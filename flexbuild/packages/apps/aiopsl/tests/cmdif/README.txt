=================================
CMDIF files to copy from <ldpaa-aiop-sl-tag>:
=================================
cmdif.h              aiopsl\src\arch\ppc\platform\ls2085a_gpp
fsl_cmdif_client.h   aiopsl\src\include\kernel
fsl_cmdif_server.h   aiopsl\src\include\kernel
fsl_cmdif_flib_fd.h  aiopsl\src\kernel\inc
fsl_cmdif_flib_c.h   aiopsl\src\kernel\inc
fsl_cmdif_flib_s.h   aiopsl\src\kernel\inc
cmdif_client_flib.h  aiopsl\src\kernel
cmdif_srv_flib.h     aiopsl\src\kernel
cmdif_client_flib.c  aiopsl\src\kernel
cmdif_srv_flib.c     aiopsl\src\kernel
fsl_shbp_flib.h      aiopsl\src\lib
fsl_shbp_host.h      aiopsl\src\lib
shbp_flib.h          aiopsl\src\lib
shbp_flib.c          aiopsl\src\lib

=================================
Test Files:
=================================
mc\tests\cmdif_gpp\srv\cmdif_srv_test.c           - Example for Server test (based MC).
mc\tests\cmdif_gpp\shbp\shbp_host.c               - Example for SHBP host implementation (based MC).
nadk_develop\apps\cmdif_demo\cmdif_server_demo.c  - Example for GPP Server test. To be tested with cmdif_integ_dbg.elf AIOP test.
nadk_develop\apps\cmdif_demo\cmdif_client_demo.c  - Example for GPP Client test. To be tested with cmdif_integ_dbg.elf AIOP test.
aiopsl\build\aiop_sim\tests\cmdif_test\integ_out\cmdif_integ_dbg.elf - This is AIOP elf to be used for CMDIF tests.
aiopsl\tests\cmdif\cmdif_integration_test.c                          - Source code of cmdif_integ_dbg.elf.
aiopsl\tests\cmdif\nadk\cmdif_client_demo.c	                     - Source code of nadk based test used for testing fsl_shbp_flib.h
aiopsl\tests\cmdif\cmdif_performance.c                               - Source code of cmdif_performance.elf.

What's new:
------------
1. Added fsl_shbp_flib.h, fsl_shbp_host.h
2. Added flib postfix to all flib files, see the new naming above.
3. Remove aiop postfix at shbp API for AIOP.
4. Updated fsl_shbp.h AIOP API.
5. Added cmdif_performace target

=================================
GPP client side:
=================================

How to test:
------------
Run cmdif_integ_dbg.elf. This elf includes AIOP server & client.
Use mc.itb for MC firmaware.
Use module name "TEST0", like in the example below:
err = cmdif_open(cidesc, "TEST0", 0, async_cb, (void *)ind, command_buffer, command_buffer_phys_addr, size);
Once commands have reached AIOP server you'll see some debug information and at the end you should see "PASSED open command".

Use cmdif_send() to test regular commands.
Once commands reached AIOP server you'll see some debug information and at the end you should see
"PASSED Synchronous Command"/"PASSED Asynchronous Command" and etc.

=================================
GPP server side:
=================================

How to test:
------------
Run cmdif_integ_dbg.elf. This elf includes AIOP server & client.
Use AIOP server to trigger AIOP client as shown at cmdif_srv_test.c and cmdif_integration_test.c.
"TEST0" control callback will trigger AIOP client according to command id that is sent to it.

------------------------------------------------------------------------------------
These are the commands that can be sent to AIOP module "TEST0" - cmdif_integ_dbg.elf
------------------------------------------------------------------------------------
#define OPEN_CMD	0x100
/*!< Will trigger cmdif_open("IRA") on AIOP, first byte of the data should be GPP DPCI id.
     If there is no data DPCI id 4 will be used as default. */
#define NORESP_CMD	0x101
/*!< Will trigger cmdif_send() on AIOP  which should reach registered module "IRA" on GPP server */
#define ASYNC_CMD	0x102
/*!< Will trigger cmdif_send() on AIOP  which should reach registered module "IRA" on GPP server
     which will send response to AIOP and call asyncronous cb on AIOP.
     The activation of this command is a bit tricky as AIOP client needs a buffer for sending async commands.
     cmdif_send(&cidesc, ASYNC_CMD, size, CMDIF_PRI_LOW, (uint64_t)async_buff,
                      NULL, NULL);
     It is important that async_buff should be of size (size + AIOP_SYNC_BUFF_SIZE).
     AIOP will use (async_buff + size) as a buffer and AIOP_SYNC_BUFF_SIZE as a size for async command.

              async_buff
     ------------------------------------
    |    size      | AIOP_SYNC_BUFF_SIZE |
     ------------------------------------
*/

#define IC_TEST		0x106
/*!< Will trigger fsl_icontext.h test on AIOP. Data size must be at least 10 bytes.
     Byte 0 will have dpci id, byte 1 will have buffer pool id  */

#define CLOSE_CMD	0x107
/*!< Will trigger cmdif_close() for cidesc that was created during last OPEN_CMD */

#define TMAN_TEST	0x108
/*!< This test will create a one-shot timer on AIOP. AIOP will print "PASSED verif_tman_cb" */

#define SHBP_TEST	0x109
/*!< Test SHBP with AIOP as allocation master.
        The data of this command have struct shbp_test structure (see cmdif_integration_test.c)
        shbp_test.shbp is the pointer to struct shbp that was created on GPP
        shbp.dpci_id is the dpci_id where this shbp belongs to.
        AIOP will acquire, release  and write to all the buffers in the pool.
        Each buffer inside this shbp should have the shbp address/handle written to the first 8 bytes,
        shbp should be written by GPP in LE without byte swaps.
        See shbp_test() at mc\tests\cmdif_gpp\srv\cmdif_srv_test.c as an example for GPP side testing */

#define SHBP_TEST_GPP	0x110
/*!< Test SHBP with GPP as allocation master.
        The data of this command have struct shbp_test structure (see cmdif_integration_test.c)
        shbp_test.shbp is the pointer to struct shbp that was created on GPP
        shbp.dpci_id is the dpci_id where this shbp belongs to.
        AIOP will check that it can't acquire from this shbp, it will write shbp_test->shbp = 0 and
        then it will release the buffer that was sent with this command into this shared buffer pool.
        See shbp_test_gpp() at mc\tests\cmdif_gpp\srv\cmdif_srv_test.c as an example for GPP side testing */

#define SHBP_TEST_AIOP	0x111
/*!< Test SHBP with AIOP as allocation master.
        The data of this command have struct shbp_test structure (see cmdif_integration_test.c)
        shbp_test.shbp is the pointer to struct shbp that was created on GPP
        shbp.dpci_id is the dpci_id where this shbp belongs to.
        AIOP will acquire buffer from this shbp, will write shbp and dpci to the buffer and will send
        no response command to GPP with this buffer.
        The GPP is expected to release this buffer into the shbp that is written to the buffer.
        See shbp_test_aiop() at mc\tests\cmdif_gpp\srv\cmdif_srv_test.c as an example for GPP side testing */

Server side test should have this code prior to sending commands to module TEST0:
        err |= cmdif_register("IRA", &ops);
        err |= cmdif_session_open(&cidesc[0], "IRA", 0, 30,
                                  buff, &auth_id);

I also share the code for cmdif_integ_dbg.elf, look at cmdif_integration_test.c on ctrl_cb0().
Here you'll see the activation of AIOP client.

------------------------------------------------------------------------------------
These are the commands that can be sent to AIOP module "TEST0" - cmdif_performance.elf
------------------------------------------------------------------------------------
NOTE : pay attention to the warnings while compiling this target. DEBUG_LEVEL must be 0.

#define PERF_TEST_START	(TMAN_TEST | CMDIF_NORESP_CMD)
/*!< Trigger for the AIOP to start 60 sec timer and to start counting the received commands. 
     Upon timeout the number of commands is printed on the screen. 
     The sequence for sending commands to AIOP should be
      1. Send low priority no response PERF_TEST_START command,
      2. Send high priority commands,
      3. TIMEOUT - print counter since PERF_TEST_START
     */
