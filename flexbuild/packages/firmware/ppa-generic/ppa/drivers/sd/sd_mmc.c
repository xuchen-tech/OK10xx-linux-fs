 //-----------------------------------------------------------------------------
 //
 // Copyright (C) 2015, 2016 Freescale Semiconductor, Inc.
 // Copyright 2017 NXP Semiconductors
 //
 // Redistribution and use in source and binary forms, with or without
 // modification, are permitted provided that the following conditions are met:
 //
 // 1. Redistributions of source code must retain the above copyright notice,
 //    this list of conditions and the following disclaimer.
 //
 // 2. Redistributions in binary form must reproduce the above copyright notice,
 //    this list of conditions and the following disclaimer in the documentation
 //    and/or other materials provided with the distribution.
 //
 // 3. Neither the name of the copyright holder nor the names of its contributors
 //    may be used to endorse or promote products derived from this software
 //    without specific prior written permission.
 //
 // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 // AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 // ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 // CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 // ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 // POSSIBILITY OF SUCH DAMAGE.
 //
 //-----------------------------------------------------------------------------

#include "types.h"
#include "lib.h"
#include "io.h"
#include "sd_mmc.h"
#include "timer.h"

 // Private structure for MMC driver data
struct mmc mmc_drv_data;

//#define SD_DEBUG_LEVEL_HIGH
#if defined(SD_DEBUG_LEVEL_HIGH)
#define debug_sd	debug
#define debug_sd_hex	debug_hex
#else
#define debug_sd
#define debug_sd_hex
#endif

 // To debug without dma comment this MACRO
#define SD_DMA_CAPABILITY

#define SD_TIMEOUT        1000 //ms
#define SD_TIMEOUT_HIGH   20000 //ms
#define SD_BLOCK_TIMEOUT  8 //ms

#define ERROR_ESDHC_CARD_DETECT_FAIL	-1
#define ERROR_ESDHC_UNUSABLE_CARD	-2
#define ERROR_ESDHC_COMMUNICATION_ERROR	-3
#define ERROR_ESDHC_BLOCK_LENGTH	-4
#define ERROR_ESDHC_DMA_ERROR		-5
#define ERROR_ESDHC_BUSY		-6


 //Function    :    set_speed
 //Arguments   :    mmc - Pointer to mmc struct
 //                 clock - Clock Value to be set
 //Return      :    void
 //Description :    Calculates the value of SDCLKFS and DVS to be set
 //                 for getting the required clock assuming the base_clk
 //                 as a fixed value (MAX_PLATFORM_CLOCK)
static void set_speed(struct mmc *mmc, uint32_t clock)
{
 // sdhc_clk = (base clock) / [(SDCLKFS × 2) × (DVS +1)]

    uint32_t dvs = 1;
    uint32_t sdclkfs = 2;
     // TBD - Change this to actual platform clock by reading via RCW
    uint32_t base_clk = MAX_PLATFORM_CLOCK;

    if (base_clk / 16 > clock) {
        for (sdclkfs = 2; sdclkfs < 256; sdclkfs *= 2) {
            if ((base_clk / sdclkfs) <= (clock * 16)) {
                break;
            }
        }
    }

    for (dvs = 1; dvs <= 16; dvs++) {
        if ((base_clk / (dvs * sdclkfs)) <= clock) {
            break;
        }
    }

    sdclkfs >>= 1;
    dvs -= 1;

    esdhc_out32(&mmc->esdhc_regs->sysctl,
        (ESDHC_SYSCTL_DTOCV(TIMEOUT_COUNTER_SDCLK_2_27) |
        ESDHC_SYSCTL_SDCLKFS(sdclkfs) | ESDHC_SYSCTL_DVS(dvs) |
        ESDHC_SYSCTL_SDCLKEN));
}

 //***************************************************************************
 // Function    :    esdhc_init
 // Arguments   :    mmc - Pointer to mmc struct
 //                  src_emmc - flag indicating if SRC is eMMC or not
 // Return      :    SUCCESS or Error Code
 // Description :    1. Set Initial Clock Speed
 //                  2. Card Detect if not eMMC
 //                  3. Enable Controller Clock
 //                  4. Send 80 ticks for card to power up
 //                  5. Set LE mode and Bus Width as 1 bit.
 //***************************************************************************
static int esdhc_init(struct mmc *mmc, uint32_t src_emmc)
{
    uint32_t val;
    unsigned long start_time, timer;

     // Reset the entire host controller
    val = esdhc_in32(&mmc->esdhc_regs->sysctl) | ESDHC_SYSCTL_RSTA;
    esdhc_out32(&mmc->esdhc_regs->sysctl, val);

     //Wait until the controller is available
    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        val = esdhc_in32(&mmc->esdhc_regs->sysctl) & ESDHC_SYSCTL_RSTA;
        if (!val) {
            break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->sysctl) &
        (ESDHC_SYSCTL_RSTA);
    if (val) {
 	debug("SD Reset failed\n");;	
        return ERROR_ESDHC_BUSY;
    }
    
     // Set initial clock speed
    set_speed(mmc, CARD_IDENTIFICATION_FREQ);

     // Check CINS in prsstat register
    val = esdhc_in32(&mmc->esdhc_regs->prsstat) & ESDHC_PRSSTAT_CINS;
    if (!val) {
        debug("CINS not set in prsstat\n");
        return ERROR_ESDHC_CARD_DETECT_FAIL;
    }

     // Enable controller clock
    val = esdhc_in32(&mmc->esdhc_regs->sysctl) | ESDHC_SYSCTL_SDCLKEN;
    esdhc_out32(&mmc->esdhc_regs->sysctl, val);

     // Send 80 clock ticks for the card to power up
    val = esdhc_in32(&mmc->esdhc_regs->sysctl) | ESDHC_SYSCTL_INITA;
    esdhc_out32(&mmc->esdhc_regs->sysctl, val);

    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT) {
        val = esdhc_in32(&mmc->esdhc_regs->sysctl) & ESDHC_SYSCTL_INITA;
        if (val) {
            break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->sysctl) & ESDHC_SYSCTL_INITA;
    if (!val) {
        debug("Failed to power up the card\n");
        return ERROR_ESDHC_CARD_DETECT_FAIL;
    }

    debug("Card detected successfully\n");

    val = esdhc_in32(&mmc->esdhc_regs->proctl);
    val = val | (ESDHC_PROCTL_EMODE_LE | ESDHC_PROCTL_DTW_1BIT);

     // Set little endian mode, set bus width as 1-bit
    esdhc_out32(&mmc->esdhc_regs->proctl, val);

     // Enable cache snooping for DMA transactions
    val = esdhc_in32(&mmc->esdhc_regs->ctl) | ESDHC_DCR_SNOOP;
    esdhc_out32(&mmc->esdhc_regs->ctl, val);

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_send_cmd
 // Arguments   :    mmc - Pointer to mmc struct
 //                  cmd - Command Number
 //                  args - Command Args
 // Return      :    SUCCESS is 0, or Error Code ( < 0)
 // Description :    Updates the eSDHC registers cmdargs and xfertype
 //***************************************************************************
static int esdhc_send_cmd(struct mmc *mmc, uint32_t cmd, uint32_t args)
{
    uint32_t val;
    unsigned long start_time, timer;
    uint32_t xfertyp = 0;

    esdhc_out32(&mmc->esdhc_regs->irqstat, ESDHC_IRQSTAT_CLEAR_ALL);

     // Wait for the command line & data line to be free
     // (poll the CIHB,CDIHB bit of the present state register)
    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        val = esdhc_in32(&mmc->esdhc_regs->prsstat) &
            (ESDHC_PRSSTAT_CIHB | ESDHC_PRSSTAT_CDIHB);
        if (!val) {
            break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->prsstat) &
        (ESDHC_PRSSTAT_CIHB | ESDHC_PRSSTAT_CDIHB);
    if (val) {
 	debug_hex("SD send cmd: Command Line or Data Line Busy\n", cmd);	
        return ERROR_ESDHC_BUSY;
    }

    if (cmd == CMD2 || cmd == CMD9) {
        xfertyp |= ESDHC_XFERTYP_RSPTYP_136;
    } else  if (cmd == CMD7 || (cmd == CMD6 && mmc->card.type == MMC_CARD)) {
        xfertyp |= ESDHC_XFERTYP_RSPTYP_48_BUSY;
    } else if (cmd != CMD0) {
        xfertyp |= ESDHC_XFERTYP_RSPTYP_48;
    }

    if (cmd == CMD2 || cmd == CMD9) {
        xfertyp |= ESDHC_XFERTYP_CCCEN; // Command index check enable
    } else if ((cmd != CMD0) && (cmd != ACMD41) && (cmd != CMD1)) {
        xfertyp = xfertyp | ESDHC_XFERTYP_CCCEN | ESDHC_XFERTYP_CICEN;
    }

    if ((cmd == CMD8 || cmd == CMD14 || cmd == CMD19) &&
            mmc->card.type == MMC_CARD) {
        xfertyp |=  ESDHC_XFERTYP_DPSEL;
        if (cmd != CMD19) {
            xfertyp |= ESDHC_XFERTYP_DTDSEL;
        }
    }

    if (cmd == CMD6 || cmd == CMD17 || cmd == CMD18 || cmd == ACMD51) {
        if (!(mmc->card.type == MMC_CARD && cmd == CMD6)) {
            xfertyp |=
                (ESDHC_XFERTYP_DPSEL | ESDHC_XFERTYP_DTDSEL);
        }
        if (cmd == CMD18) {
            xfertyp |= ESDHC_XFERTYP_BCEN;
            if (mmc->dma_support) {
                  // Set BCEN of XFERTYP
                xfertyp |= ESDHC_XFERTYP_DMAEN;
            }
	}

        if(cmd == CMD17 && mmc->dma_support) {
           xfertyp |= ESDHC_XFERTYP_DMAEN;
        }
    }

    xfertyp |= ((cmd & 0x3F) << 24);
    esdhc_out32(&mmc->esdhc_regs->cmdarg, args);
    esdhc_out32(&mmc->esdhc_regs->xfertyp, xfertyp);

    debug_sd_hex("cmd" , cmd);
    debug_sd_hex("args", args);
    debug_sd_hex("xfertyp:", xfertyp);

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_wait_response
 // Arguments   :    mmc - Pointer to mmc struct
 //                  response - Value updated
 // Return      :    SUCCESS - Response Recieved
 //                  COMMUNICATION_ERROR - Command not Complete
 //                  COMMAND_ERROR - CIE, CCE or CEBE  error
 //                  RESP_TIMEOUT - CTOE error
 // Description :    Checks for successful command completion.
 //                  Clears the CC bit at the end.
 //***************************************************************************
static int esdhc_wait_response(struct mmc *mmc, uint32_t *response)
{
    uint32_t val;
    unsigned long start_time, timer;
    uint32_t status = 0;

     // Wait for the command to complete
    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        val = esdhc_in32(&mmc->esdhc_regs->irqstat) & ESDHC_IRQSTAT_CC;
        if (val) {
            break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->irqstat) & ESDHC_IRQSTAT_CC;
    if (!val) {
	debug("SD wait_response err: IRQSTAT Command not complete (CC not set)\n");
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    status = esdhc_in32(&mmc->esdhc_regs->irqstat);

     // Check whether the interrupt is a CRC, CTOE or CIE error
    if (status & (ESDHC_IRQSTAT_CIE | ESDHC_IRQSTAT_CEBE |
            ESDHC_IRQSTAT_CCE)) {
	debug_hex("SD wait_response err: IRQSTAT CRC, CEBE or CIE error\n", status);
        return COMMAND_ERROR;
    }

    if (status & ESDHC_IRQSTAT_CTOE) {
	debug_hex("SD wait_response err: IRQSTAT CTOE set\n", status);
        return RESP_TIMEOUT;
    }

    if (status & ESDHC_IRQSTAT_DMAE) {
	debug_hex("SD wait_response err: IRQSTAT DMAE set\n", status);
        return ERROR_ESDHC_DMA_ERROR;
    }

    if (NULL != response) {
         // Get response values from eSDHC CMDRSPx registers.
        response[0] = esdhc_in32(&mmc->esdhc_regs->cmdrsp[0]);
        response[1] = esdhc_in32(&mmc->esdhc_regs->cmdrsp[1]);
        response[2] = esdhc_in32(&mmc->esdhc_regs->cmdrsp[2]);
        response[3] = esdhc_in32(&mmc->esdhc_regs->cmdrsp[3]);

        debug_sd("Resp R1 R2 R3 R4\n");
        debug_sd_hex("Resp R1", response[0]);
        debug_sd_hex("R2 ", response[1]);
        debug_sd_hex("R3  ", response[2]);
        debug_sd_hex("R4 ", response[3]);

    }

     // Clear the CC bit - w1c
    val = esdhc_in32(&mmc->esdhc_regs->irqstat) | ESDHC_IRQSTAT_CC;
    esdhc_out32(&mmc->esdhc_regs->irqstat, val);

    return 0;
}

 //***************************************************************************
 // Function    :    mmc_switch_to_high_frquency
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    mmc card bellow ver 4.0 does not support high speed
 //                  freq = 20 MHz
 //                  Send CMD6 (CMD_SWITCH_FUNC) With args 0x03B90100
 //                  Send CMD13 (CMD_SEND_STATUS)
 //                  if SWITCH Error, freq = 26 MHz
 //                  if no error, freq = 52 MHz
 //***************************************************************************
static int mmc_switch_to_high_frquency(struct mmc *mmc)
{
    int error;
    uint32_t response[4];
    unsigned long start_time, timer;

    mmc->card.bus_freq = MMC_SS_20MHZ;
     // mmc card bellow ver 4.0 does not support high speed
    if (mmc->card.version < MMC_CARD_VERSION_4_X) {
        return 0;
    }

     // send switch cmd to change the card to High speed
    error = esdhc_send_cmd(mmc, CMD_SWITCH_FUNC, SET_EXT_CSD_HS_TIMING);
    if (error) {
        return error;
    }
    error = esdhc_wait_response(mmc, response);
    if (error) {
        return error;
    }

    start_time = get_timer(0);
    do {
         // check the status for which error
        error = esdhc_send_cmd(mmc, CMD_SEND_STATUS, mmc->card.rca << 16);
    	if (error) {
            return error; 
    	}
        error = esdhc_wait_response(mmc, response);
        if (error) {
            return error; 
        }
    } while ((response[0] & SWITCH_ERROR) && (get_timer(start_time) < SD_TIMEOUT));

    // Check for the present state of card
    if (response[0] & SWITCH_ERROR) {
        mmc->card.bus_freq = MMC_HS_26MHZ;
    } else {
        mmc->card.bus_freq = MMC_HS_52MHZ;
    }

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_set_data_attributes
 // Arguments   :    mmc - Pointer to mmc struct
 //                  blkcnt
 //                  blklen
 // Return      :    SUCCESS or Error Code
 // Description :    Set block attributes and watermark level register
 //***************************************************************************
static int esdhc_set_data_attributes(struct mmc *mmc, uint32_t *dest_ptr, uint32_t blkcnt, uint32_t blklen)
{
    uint32_t val;
    unsigned long start_time, timer;
    uint32_t wml;
    uint32_t wl;
    uint32_t dst = (uint32_t)((uint64_t)(dest_ptr));

    // set blkattr when no transactions are executing

    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        val = esdhc_in32(&mmc->esdhc_regs->prsstat) & ESDHC_PRSSTAT_DLA;
        if (!val) {
            break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->prsstat) & ESDHC_PRSSTAT_DLA;
    if (val) {
	debug("esdhc_set_data_attributes: Data line active. Can't set attribute\n");
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    wml = esdhc_in32(&mmc->esdhc_regs->wml);
    wml &= ~(ESDHC_WML_WR_BRST_MASK | ESDHC_WML_RD_BRST_MASK |
            ESDHC_WML_RD_WML_MASK | ESDHC_WML_WR_WML_MASK);

    if (mmc->dma_support && dest_ptr) {
        // Set burst length to 128 bytes
        esdhc_out32(&mmc->esdhc_regs->wml, wml | ESDHC_WML_WR_BRST(BURST_128_BYTES));
        esdhc_out32(&mmc->esdhc_regs->wml, wml | ESDHC_WML_RD_BRST(BURST_128_BYTES));

        // Set DMA System Destination Address
        esdhc_out32(&mmc->esdhc_regs->dsaddr, dst);
    } else {
        wl = (blklen >= BLOCK_LEN_512) ? WML_512_BYTES : ((blklen + 3) / 4);
        // Set 'Read Water Mark Level' register
        esdhc_out32(&mmc->esdhc_regs->wml, wml | ESDHC_WML_RD_WML(wl));
    }

    // Configure block Attributes register
    esdhc_out32(&mmc->esdhc_regs->blkattr,
        ESDHC_BLKATTR_BLKCNT(blkcnt) | ESDHC_BLKATTR_BLKSZE(blklen));

    mmc->block_len = blklen;

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_read_data_nodma
 // Arguments   :    mmc - Pointer to mmc struct
 //                  dest_ptr - Bufffer where read data is to be copied
 //                  len - Length of Data to be read
 // Return      :    SUCCESS or Error Code
 // Description :    Read data from the sdhc buffer without using DMA
 //                  and using polling mode
 //***************************************************************************
static int esdhc_read_data_nodma(struct mmc *mmc, void *dest_ptr, uint32_t len)
{
    uint32_t i = 0;
    uint32_t status;
    uint32_t num_blocks;
    uint32_t *dst = (uint32_t *)dest_ptr;
    uint32_t val;
    unsigned long start_time, timer;

    num_blocks = len / mmc->block_len;

    while (num_blocks--) {

        start_time = get_timer(0);
        while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
            val = esdhc_in32(&mmc->esdhc_regs->prsstat) &
                ESDHC_PRSSTAT_BREN;
            if (val)
                break;
        }

        val = esdhc_in32(&mmc->esdhc_regs->prsstat) & ESDHC_PRSSTAT_BREN;
        if (!val)
            return ERROR_ESDHC_COMMUNICATION_ERROR;

        for (i = 0, status = esdhc_in32(&mmc->esdhc_regs->irqstat);
                i < mmc->block_len / 4;    i++, dst++) {
             // get data from data port
            val = in_le32(&mmc->esdhc_regs->datport);
            esdhc_out32(dst, val);
             // Increment destination pointer
            status = esdhc_in32(&mmc->esdhc_regs->irqstat);
        }
         // Check whether the interrupt is an DTOE/DCE/DEBE
        if (status & (ESDHC_IRQSTAT_DTOE | ESDHC_IRQSTAT_DCE |
                    ESDHC_IRQSTAT_DEBE)) {
	    debug_hex("SD read error - DTOE, DCE, DEBE bit set", status);
            return ERROR_ESDHC_COMMUNICATION_ERROR;
	}
    }

    // Wait for TC

    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        val = esdhc_in32(&mmc->esdhc_regs->irqstat) & ESDHC_IRQSTAT_TC;
        if (val) {
                break;
        }
    }

    val = esdhc_in32(&mmc->esdhc_regs->irqstat) & ESDHC_IRQSTAT_TC;
    if (!val) {
	debug("SD read timeout: Transfer bit not set in IRQSTAT\n");
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_read_data_dma
 // Arguments   :    mmc - Pointer to mmc struct
 //                  len - Length of Data to be read
 // Return      :    SUCCESS or Error Code
 // Description :    Read data from the sd card using DMA.
 //***************************************************************************
static int esdhc_read_data_dma(struct mmc *mmc, uint32_t len)
{
    unsigned long long timeout;
    uint32_t status;
    uint32_t tblk;

    uint32_t num_blocks;
    unsigned long start_time, timer;
    num_blocks = len / mmc->block_len;

    tblk = SD_BLOCK_TIMEOUT * (len / mmc->block_len);

    start_time = get_timer(0);

    // poll till TC is set
    do {
        status = esdhc_in32(&mmc->esdhc_regs->irqstat);

        if (status & (ESDHC_IRQSTAT_DEBE | ESDHC_IRQSTAT_DCE | ESDHC_IRQSTAT_DTOE)) {
	    debug_hex("SD read error - DTOE, DCE, DEBE bit set", status);
            return ERROR_ESDHC_COMMUNICATION_ERROR;
        }

	if (status & (ESDHC_IRQSTAT_DMAE)) {
	    debug_hex("SD read error - DMA error", status);
            return ERROR_ESDHC_DMA_ERROR;
	}

    } while (!(status & ESDHC_IRQSTAT_TC) && (esdhc_in32(&mmc->esdhc_regs->prsstat) & ESDHC_PRSSTAT_DLA) && (get_timer(start_time) < SD_TIMEOUT_HIGH + tblk));

    if (get_timer(start_time) > SD_TIMEOUT_HIGH + tblk) {
	debug("SD read DMA timeout\n");
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    return 0;
}

 //***************************************************************************
 // Function    :    esdhc_read_data
 // Arguments   :    mmc - Pointer to mmc struct
 //                  dest_ptr - Bufffer where read data is to be copied
 //                  len - Length of Data to be read
 // Return      :    SUCCESS or Error Code
 // Description :    Calls esdhc_read_data_nodma and clear interrupt status
 //***************************************************************************
int esdhc_read_data(struct mmc *mmc, void *dest_ptr, uint32_t len)
{
    int ret;

    if (mmc->dma_support && len > 64) {
        ret = esdhc_read_data_dma(mmc, len);
    } else {
        ret = esdhc_read_data_nodma(mmc, dest_ptr, len);
    }

    // clear interrupt status
    esdhc_out32(&mmc->esdhc_regs->irqstat, ESDHC_IRQSTAT_CLEAR_ALL);

    return ret;
}

 //***************************************************************************
 // Function    :    sd_switch_to_high_freq
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    1. Send ACMD51 (CMD_SEND_SCR)
 //                  2. Read the SCR to check if card supports higher freq
 //                  3. check version from SCR
 //                  4. If SD 1.0, return (no Switch) freq = 25 MHz.
 //                  5. Send CMD6 (CMD_SWITCH_FUNC) with args 0x00FFFFF1 to
 //                     check the status of switch func
 //                  6. Send CMD6 (CMD_SWITCH_FUNC) With args 0x80FFFFF1 to
 //                     switch to high frequency = 50 Mhz
 //***************************************************************************
static int sd_switch_to_high_freq(struct mmc *mmc)
{
    int err;
    uint8_t scr[8];
    uint8_t status[64];
    uint32_t response[4];
    uint32_t version;
    uint32_t count;
    uint32_t sd_versions[] = {SD_CARD_VERSION_1_0, SD_CARD_VERSION_1_10,
            SD_CARD_VERSION_2_0};

    mmc->card.bus_freq = SD_SS_25MHZ;
    // Send Application command
    err = esdhc_send_cmd(mmc, CMD_APP_CMD, mmc->card.rca << 16);
    if (err) {
        return err;
    }

    err = esdhc_wait_response(mmc, response);
    if (err) {
        return err;
    }

    esdhc_set_data_attributes(mmc, NULL, 1, 8);
    // Read the SCR to find out if this card supports higher speeds
    err = esdhc_send_cmd(mmc, CMD_SEND_SCR,  mmc->card.rca << 16);
    if (err) {
        return err;
    }
    err = esdhc_wait_response(mmc, response);
    if (err) {
        return err;
    }

    // read 8 bytes of scr data
    err = esdhc_read_data(mmc, scr, 8);
    if (err) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    // check version from SCR
    version = scr[0] & 0xF;
    if (version <= 2) {
        mmc->card.version = sd_versions[version];
    } else {
        mmc->card.version = SD_CARD_VERSION_2_0;
    }

    // does not support switch func
    if (SD_CARD_VERSION_1_0 == mmc->card.version) {
        return 0;
    }

    // read 64 bytes of status
    esdhc_set_data_attributes(mmc, NULL, 1, 64);

    // check the status of switch func
    for (count = 0; count < 4; count++) {
        err = esdhc_send_cmd(mmc, CMD_SWITCH_FUNC, SD_SWITCH_FUNC_CHECK_MODE);
        if (err) {
            return err;
        }
        err = esdhc_wait_response(mmc, response);
        if (err) {
            return err;
        }
        // read 64 bytes of scr data
        err = esdhc_read_data(mmc, status, 64);
        if (err) {
            return ERROR_ESDHC_COMMUNICATION_ERROR;
        }

        if (!(status[29] & SD_SWITCH_FUNC_HIGH_SPEED)) {
            break;
        }
    }

    if (!(status[13] & SD_SWITCH_FUNC_HIGH_SPEED)) {
        return 0;
    }

    // SWITCH
    esdhc_set_data_attributes(mmc, NULL, 1, 64);
    err = esdhc_send_cmd(mmc, CMD_SWITCH_FUNC, SD_SWITCH_FUNC_SWITCH_MODE);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, response);
    if (err) {
        return err;
    }

    err = esdhc_read_data(mmc, status, 64);
    if (err) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    if ((status[16]) == 0x01) {
        mmc->card.bus_freq = SD_HS_50MHZ;
    }

    return 0;
}

 //***************************************************************************
 // Function    :    change_state_to_transfer_state
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    1. Send CMD7 (CMD_SELECT_CARD) to toggles the card
 //                     between stand-by and transfer state
 //                  2. Send CMD13 (CMD_SEND_STATUS) to check state as
 //                     Transfer State
 //***************************************************************************
static int change_state_to_transfer_state(struct mmc *mmc)
{
    int error = 0;
    uint32_t response[4];
    unsigned long start_time, timer;

    // Command CMD_SELECT_CARD/CMD7 toggles the card between stand-by
    // and transfer states
    error = esdhc_send_cmd(mmc, CMD_SELECT_CARD, mmc->card.rca << 16);
    if (error) {
        return error;
    }
    error = esdhc_wait_response(mmc, response);
    if (error) {
       return error;
    }

    start_time = get_timer(0);
    while (get_timer(start_time) < SD_TIMEOUT_HIGH) {
        // send CMD13 to check card status
        error = esdhc_send_cmd(mmc, CMD_SEND_STATUS, mmc->card.rca << 16);
    	if (error) {
            return error;
   	}
        error = esdhc_wait_response(mmc, response);
        if (error || (response[0] & R1_ERROR)) {
            return error;
        }

        // Check for the present state of card
        if (((response[0] >> 9) & 0xF) == STATE_TRAN) {
            break;
        }
    }
    if (((response[0] >> 9) & 0xF) == STATE_TRAN) {
        return 0;
    } else {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }
}

 //***************************************************************************
 // Function    :    get_cid_rca_csd
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    1. Send CMD2 (CMD_ALL_SEND_CID)
 //                  2. get RCA for SD cards, set rca for mmc cards
 //                     Send CMD3 (CMD_SEND_RELATIVE_ADDR)
 //                  3. Send CMD9 (CMD_SEND_CSD)
 //                  4. Get MMC Version from CSD
 //***************************************************************************
static int get_cid_rca_csd(struct mmc *mmc)
{
    int err;
    uint32_t version;
    uint32_t response[4];
    uint32_t mmc_version[] = {MMC_CARD_VERSION_1_2, MMC_CARD_VERSION_1_4,
        MMC_CARD_VERSION_2_X, MMC_CARD_VERSION_3_X,
        MMC_CARD_VERSION_4_X};

    err = esdhc_send_cmd(mmc, CMD_ALL_SEND_CID, 0);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, response);
    if (err) {
	return err;
    }

    // get RCA for SD cards, set rca for mmc cards
    mmc->card.rca = SD_MMC_CARD_RCA;

    // send RCA cmd
    err = esdhc_send_cmd(mmc, CMD_SEND_RELATIVE_ADDR, mmc->card.rca << 16);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, response);
    if (err) {
	return err;
    }

    // for SD, get the the RCA
    if (SD_CARD == mmc->card.type) {
        mmc->card.rca = (response[0] >> 16) & 0xFFFF;
    }

    // Get the CSD (card specific data) from card.
    err = esdhc_send_cmd(mmc, CMD_SEND_CSD, mmc->card.rca << 16);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, response);
    if (err) {
	return err;
    }

    version = (response[3] >> 18) & 0xF;
    if (MMC_CARD == mmc->card.type) {
        if (version <= MMC_CARD_VERSION_4_X) {
            mmc->card.version = mmc_version[version];
        } else {
            mmc->card.version = MMC_CARD_VERSION_4_X;
        }
    }

    mmc->card.block_len = 1 << ((response[2] >> 8) & 0xF);

    if (mmc->card.block_len > BLOCK_LEN_512) {
        mmc->card.block_len = BLOCK_LEN_512;
    }

    return 0;
}

 //***************************************************************************
 // Function    :    identify_mmc_card
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    1. Send Reset Command
 //                  2. Send CMD1 with args to set voltage range and Sector
 //                     Mode. (Voltage Args = 0xFF8)
 //                  3. Check the OCR Response
 //***************************************************************************
static int identify_mmc_card(struct mmc *mmc)
{
    unsigned long start_time, timer;
    uint32_t resp[4];
    int ret;
    uint32_t args;

    // card reset
    ret = esdhc_send_cmd(mmc, CMD_GO_IDLE_STATE, 0);
    if (ret) {
        return ret;
    }
    ret = esdhc_wait_response(mmc, resp);
    if (ret) {
        return ret;
    }

    // Send CMD1 to get the ocr value repeatedly till the card
    // busy is clear. timeout = 20sec

    start_time = get_timer(0);
    do {
        // set the bits for the voltage ranges supported by host
        args = mmc->voltages_caps | MMC_OCR_SECTOR_MODE;
        ret = esdhc_send_cmd(mmc, CMD_MMC_SEND_OP_COND, args);
        if (ret) {
            return ret;
        }
        ret = esdhc_wait_response(mmc, resp);
        if (ret) {
            return ERROR_ESDHC_UNUSABLE_CARD;
        }
    } while ((!(resp[0] & MMC_OCR_BUSY)) && (get_timer(start_time) < SD_TIMEOUT_HIGH));

    if (get_timer(start_time) > SD_TIMEOUT_HIGH) {
        return ERROR_ESDHC_UNUSABLE_CARD;
    }

    if ((resp[0] & MMC_OCR_CCS) == MMC_OCR_CCS) {
        mmc->card.is_high_capacity = 1;
    }

    return MMC_CARD;
}

 //***************************************************************************
 // Function    :    check_for_sd_card
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    1. Send Reset Command
 //                  2. Send CMD8 with pattern 0xAA (to check for SD 2.0)
 //                  3. Send ACMD41 with args to set voltage range and HCS
 //                     HCS is set only for SD Card > 2.0
 //                     Voltage Caps = 0xFF8
 //                  4. Check the OCR Response
 //***************************************************************************
static int check_for_sd_card(struct mmc *mmc)
{
    unsigned long start_time, timer;
    uint32_t args;
    int  ret;
    uint32_t resp[4];

     // Send reset command
    ret = esdhc_send_cmd(mmc, CMD_GO_IDLE_STATE, 0);
    if (ret) {
        return ret;
    }
    ret = esdhc_wait_response(mmc, resp);
    if (ret) {
        return ret;
    }

    // send CMD8 with  pattern 0xAA
    args = MMC_VDD_HIGH_VOLTAGE | 0xAA;
    ret = esdhc_send_cmd(mmc, CMD_SEND_IF_COND, args);
    if (ret) {
        return ret;
    }
    ret = esdhc_wait_response(mmc, resp);
    if (RESP_TIMEOUT == ret) { // sd ver 1.x or not sd
        mmc->card.is_high_capacity = 0;
    } else if ((0xFF & resp[0]) == 0xAA) { // ver 2.0 or later
        mmc->card.version = SD_CARD_VERSION_2_0;
    } else {
        return  NOT_SD_CARD;
    }

    // Send Application command-55 to get the ocr value repeatedly till
    // the card busy is clear. timeout = 20sec

    start_time = get_timer(0);
    do {
        ret = esdhc_send_cmd(mmc, CMD_APP_CMD, 0);
	if (ret)
		return ret;
        ret = esdhc_wait_response(mmc, resp);
        if (COMMAND_ERROR == ret) {
            return ERROR_ESDHC_UNUSABLE_CARD;
        }

        // set the bits for the voltage ranges supported by host
        args = mmc->voltages_caps;
        if (mmc->card.version == SD_CARD_VERSION_2_0) {
            args |= SD_OCR_HCS;
        }

        // Send ACMD41 to set voltage range
        ret = esdhc_send_cmd(mmc, CMD_SD_SEND_OP_COND, args);
	if (ret) {
	    return ret;
	}
        ret = esdhc_wait_response(mmc, resp);
        if (COMMAND_ERROR == ret) {
            return ERROR_ESDHC_UNUSABLE_CARD;
        } else if (RESP_TIMEOUT == ret) {
            return NOT_SD_CARD;
        }
    } while ((!(resp[0] & MMC_OCR_BUSY)) && (get_timer(start_time) < SD_TIMEOUT_HIGH));

    if (get_timer(start_time) > SD_TIMEOUT_HIGH) {
        return ERROR_ESDHC_UNUSABLE_CARD;
    }

    // bit set in card capacity status
    if ((resp[0] & MMC_OCR_CCS) == MMC_OCR_CCS) {
        mmc->card.is_high_capacity = 1;
    }

    return SD_CARD;
}

 //***************************************************************************
 // Function    :    esdhc_emmc_init
 // Arguments   :    mmc - Pointer to mmc struct
 //                  src_emmc - Flag to Indicate SRC as emmc
 // Return      :    SUCCESS or Error Code (< 0)
 // Description :    Base Function called from sd_mmc_init or emmc_init
 //***************************************************************************
int esdhc_emmc_init(struct mmc *mmc, uint32_t src_emmc)
{
    int error = 0;
    int ret = 0;
    error = esdhc_init(mmc, src_emmc);
    if (error) {
        return error;
    }

    mmc->card.bus_freq = CARD_IDENTIFICATION_FREQ;
    mmc->card.rca = 0;
    mmc->card.is_high_capacity = 0;
    mmc->card.type = ERROR_ESDHC_UNUSABLE_CARD;

     // Set Voltage caps as FF8 i.e all supported
     // high voltage bits 2.7 - 3.6
    mmc->voltages_caps = MMC_OCR_VDD_FF8;

#ifdef SD_DMA_CAPABILITY
     // Getting host DMA capabilities.
    mmc->dma_support = esdhc_in32(&mmc->esdhc_regs->hostcapblt) & ESDHC_HOSTCAPBLT_DMAS;
#else
    mmc->dma_support = 0;
#endif

    ret = NOT_SD_CARD;
     // If SRC is not EMMC, check for SD or MMC
    if (src_emmc == 0) {
        ret = check_for_sd_card(mmc);
    }
    switch (ret) {
    case SD_CARD:
        mmc->card.type = SD_CARD;
        break;

    case NOT_SD_CARD:
        // try for MMC card
        if (MMC_CARD == identify_mmc_card(mmc)) {
            mmc->card.type = MMC_CARD;
        } else {
            return ERROR_ESDHC_UNUSABLE_CARD;
        }
        break;

    default:
        return ERROR_ESDHC_UNUSABLE_CARD;
    }

    // get CID, RCA and CSD. For MMC, set the rca
    error = get_cid_rca_csd(mmc);
    if (error) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    // change state to Transfer mode
    error = change_state_to_transfer_state(mmc);
    if (error) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    // change to high frequency if supported
    if (SD_CARD == mmc->card.type) {
        error = sd_switch_to_high_freq(mmc);
    } else {
        error = mmc_switch_to_high_frquency(mmc);
    }
    if (error) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    // mmc: 20000000, 26000000, 52000000
    // sd: 25000000, 50000000
    set_speed(mmc, mmc->card.bus_freq);

    debug_sd("init done:\n");
    debug_sd_hex("ret", error);
    debug_sd_hex("bus freq ",mmc->card.bus_freq);
    debug_sd_hex("blklen ",mmc->card.block_len);
    debug_sd_hex("version ",mmc->card.version);
    debug_sd_hex("hc ",mmc->card.is_high_capacity);

    return 0;
}

 //***************************************************************************
 // Function    :    sd_mmc_init
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    Base Function called via hal_init for SD/MMC
 //                  initialization
 //***************************************************************************
int sd_mmc_init()
{
    struct mmc *mmc = NULL;
    uint32_t src_emmc = 0;
    int ret;

    mmc = &mmc_drv_data;
    memset(mmc, 0, sizeof(struct mmc));
    mmc->esdhc_regs = (struct esdhc_regs *)ESDHC_BASE_ADDR;

    ret = esdhc_emmc_init(mmc, src_emmc);
    return ret;
}

 //***************************************************************************
 // Function    :    emmc_init
 // Arguments   :    mmc - Pointer to mmc struct
 // Return      :    SUCCESS or Error Code
 // Description :    Base Function called via hal_init for eMMC
 //                  initialization
 //***************************************************************************
int emmc_init(void *b)
{
    struct mmc *mmc = NULL;
    uint32_t src_emmc = 1;
    int ret;

    mmc = (struct mmc *)b;
    memset(mmc, 0, sizeof(struct mmc));
    mmc->esdhc_regs = (struct esdhc_regs *)ESDHC_BASE_ADDR;

    ret = esdhc_emmc_init(mmc, src_emmc);
    return ret;
}

 //***************************************************************************
 // Function    :    esdhc_read_block
 // Arguments   :    mmc - Pointer to mmc struct
 //                  dst - Destination Pointer
 //                  block - Block Number
 // Return      :    SUCCESS or Error Code
 // Description :    Read a Single block to Destination Pointer
 //                  1. Send CMD16 (CMD_SET_BLOCKLEN) with args as blocklen
 //                  2. Send CMD17 (CMD_READ_SINGLE_BLOCK) with args offset
 //***************************************************************************
static int esdhc_read_block(struct mmc *mmc, void *dst, uint32_t block)
{
    uint32_t offset;
    int err;

    debug("read single            ");
    debug_hex("dst ",(long long unsigned int)dst);
    debug_hex("block ",block);

     // send cmd16 to set the block size.
    err = esdhc_send_cmd(mmc, CMD_SET_BLOCKLEN, mmc->card.block_len);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, NULL);
    if (err) {
        return ERROR_ESDHC_COMMUNICATION_ERROR;
    }

    if (mmc->card.is_high_capacity) {
        offset = block;
    } else {
        offset = block * mmc->card.block_len;
    }

    esdhc_set_data_attributes(mmc, dst, 1, mmc->card.block_len);
    err = esdhc_send_cmd(mmc, CMD_READ_SINGLE_BLOCK, offset);
    if (err) {
	return err;
    }
    err = esdhc_wait_response(mmc, NULL);
    if (err) {
	return err;
    }

    err = esdhc_read_data(mmc, dst, mmc->card.block_len);

    return err;
}

 //***************************************************************************
 // Function    :    esdhc_read
 // Arguments   :    src_offset - offset on sd/mmc to read from. Should be block
 //		     size aligned
 //                  dst - Destination Pointer
 //                  size - Length of Data ( Multiple of block size)
 // Return      :    SUCCESS or Error Code
 // Description :    Calls esdhc_read_block repeatedly for reading the
 //                  data.
 //***************************************************************************
int esdhc_read(uint32_t src_offset, uint8_t *dst, uint32_t size)
{
    int error = 0;
    uint32_t blk, num_blocks, offset, read_len;
    struct mmc *mmc = &mmc_drv_data;
    uint8_t *buff = dst;

    mmc->esdhc_regs = (struct esdhc_regs *)ESDHC_BASE_ADDR;

    debug_sd("sdmmc read\n");
    debug_sd_hex("src", src_offset);
    debug_sd_hex("dst ", (long long unsigned int)dst);
    debug_sd_hex("size", size);

     // check for size
    if (size == 0) {
        return 0;
    }

    if ((size % mmc->card.block_len) != 0) {
	debug("Size is not block aligned\n");
	return -1;
    }

    if ((src_offset % mmc->card.block_len) != 0) {
	debug("Size is not block aligned\n");
	return -1;
    }

     // start block
    blk = src_offset / mmc->card.block_len;

     // Number of blocks to be read
    num_blocks = size / mmc->card.block_len;

    while (num_blocks) {
        error = esdhc_read_block(mmc, buff, blk);
        if (error) {
	debug_hex("Read error\n", error);
            return error;
        }

	buff = buff + mmc->card.block_len ;
        blk++;
        num_blocks--;
    }

    debug("sd-mmc read done.            \n");
    return error;
}
