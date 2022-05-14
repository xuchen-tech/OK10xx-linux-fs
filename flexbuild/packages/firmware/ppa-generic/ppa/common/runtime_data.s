//-----------------------------------------------------------------------------
// 
// Copyright (c) 2013-2016 Freescale Semiconductor, Inc.
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
// Author Rod Dorris <rod.dorris@nxp.com>
// 
//-----------------------------------------------------------------------------

  .section .text, "ax"

//-----------------------------------------------------------------------------

#include "psci.h"

#if (LSCH == 3)
#include "lsch3.h"
#elif (LSCH == 2)
#include "lsch2.h"
#endif

//-----------------------------------------------------------------------------

.global _getCoreData
.global _setCoreData
.global _get_task1_start
.global _set_task1_start
.global _get_task1_done
.global _set_task1_done
.global _get_task1_core
.global _set_task1_core
.global _get_task2_start
.global _set_task2_start
.global _get_task2_done
.global _set_task2_done
.global _get_task2_core
.global _set_task2_core
.global _initialize_memory
.global _init_global_data
.global _get_global_data
.global _set_global_data
.global _initialize_psci
.global _init_stack_percpu
.global _init_task_flags
.global _init_membank_data
#if (PSCI_TEST)
.global _populate_membank_data
#endif

#if (DATA_LOC == DATA_IN_DDR)
.global _getBaseAddrNS
#endif

//-----------------------------------------------------------------------------

#if (DATA_LOC == DATA_IN_DDR)

 // Note: this is a non-std function. It does not pass parms in x0, and it
 //       uses only registers x13, x14, and x15. Be careful when inserting
 //       a call to this function in an existing call chain.
 // this function returns the requested base address (requested base address
 //  is identified by enum type in runtime_data.h
 // in:  x13 = base addr enum type
 // out: x13 = base addr requested
 //      x13 = 0 if enum type undefined
 // uses x13, x15, x14
_getBaseAddrNS:

    ldr  x14, =__PPA_PROG_START__
    ldr  x15, =__BSS_END__
    sub  x15, x15, x14

     // x15 = size of executable image

    adr  x14, _start_monitor_el3
    add  x14, x14, x15

     // x14 = raw addr

     // move the addr to the next 8KB boundary
    mov  x15, #INCREMENT_8KB
    add  x14, x14, x15

     // trim the lo-order bits to achieve alignment
    ldr  x15, =ALIGN_MASK_8KB
    and  x15, x14, x15

     // x13 = base address requested (enum)
     // x15 = base address of secure data area

    cmp  x13, #SECURE_DATA_BASE_QUERY
    b.ne 1f
    mov  x13, x15
    b    13f
1:   // EL3_PAGE_TABLE_BASE
    cmp  x13, #EL3_PAGE_TABLE_BASE_QUERY
    b.ne 2f
    mov  x13, x15
    b    13f
2:   // EL3_VECTOR_BASE
    cmp  x13, #EL3_VECTOR_BASE_QUERY
    b.ne 3f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    mov  x14, #EL3_VECTOR_SIZE
    sub  x13, x15, x14
    b    13f
3:   // SMC_GLBL_BASE_QUERY
    cmp  x13, #SMC_GLBL_BASE_QUERY
    b.ne 4f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x13, x15, x14
    b    13f
4:   // SMC_TASK1_BASE_QUERY
    cmp  x13, #SMC_TASK1_BASE_QUERY
    b.ne 5f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    add  x13, x14, #SMC_MISC_OFFSET
    b    13f
5:   // SMC_TASK2_BASE_QUERY
    cmp  x13, #SMC_TASK2_BASE_QUERY
    b.ne 6f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    add  x15, x14, #SMC_MISC_OFFSET
    add  x13, x15, #SMC_TASK_OFFSET
    b    13f
6:   // SMC_TASK3_BASE_QUERY
    cmp  x13, #SMC_TASK3_BASE_QUERY
    b.ne 7f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    add  x15, x14, #SMC_MISC_OFFSET
    mov  x14, #SMC_TASK_OFFSET
    lsr  x14, x14, #1
    add  x13, x15, x14
    b    13f
7:   // SMC_TASK4_BASE_QUERY
    cmp  x13, #SMC_TASK4_BASE_QUERY
    b.ne 8f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    add  x15, x14, #SMC_MISC_OFFSET
    mov  x14, #SMC_TASK_OFFSET
    lsr  x13, x14, #1
    add  x14, x13, x14
    add  x13, x15, x14
    b    13f
8:   // MEMBANK_BASE_QUERY
    cmp  x13, #MEMBANK_BASE_QUERY
    b.ne 9f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    ldr  x15, =MEMBANK_REGION_SIZE
    sub  x13, x14, x15
    b    13f
9:   // BC_PSCI_BASE_QUERY
    cmp  x13, #BC_PSCI_BASE_QUERY
    b.ne 10f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    ldr  x15, =MEMBANK_REGION_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_PSCI_DATA_SIZE
    sub  x13, x14, x15
    b    13f
10:  // BC_STACK_TOP_QUERY
    cmp  x13, #BC_STACK_TOP_QUERY
    b.ne 11f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    ldr  x15, =MEMBANK_REGION_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_PSCI_DATA_SIZE
    sub  x13, x14, x15
    b    13f
11:  // SECONDARY_TOP_QUERY
    cmp  x13, #SECONDARY_TOP_QUERY
    mov  x13, xzr
    b.ne 12f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    ldr  x15, =MEMBANK_REGION_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_PSCI_DATA_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_STACK_SIZE
    add  x15, x15, #INTEGRITY_BUFF_SIZE
    sub  x13, x14, x15
    b    13f
12:  // STACK_BASE_ADDR_QUERY
    cmp  x13, #STACK_BASEADDR_QUERY
    mov  x13, xzr
    b.ne 13f
    ldr  x14, =SECURE_DATA_SIZE
    add  x15, x15, x14
    ldr  x14, =SMC_REGION_OFFSET
    sub  x14, x15, x14
    ldr  x15, =MEMBANK_REGION_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_PSCI_DATA_SIZE
    sub  x14, x14, x15
    mov  x15, #BC_STACK_SIZE
    add  x15, x15, #INTEGRITY_BUFF_SIZE
    sub  x14, x14, x15
     // x14 = secondary top
    ldr  x15, =SEC_REGION_SIZE
    mov  x13, #SECONDARY_CORE_CNT
    mul  x15, x15, x13
    sub  x13, x14, x15
13:
    ret

#endif

//-----------------------------------------------------------------------------

 // this function returns the specified data field value from the specified cpu
 // core data area
 // in:  x0 = core mask lsb
 //      x1 = data field name/offset
 // out: x0 = data value
 // uses x0, x1, x2, [x13, x14, x15]
_getCoreData:
     // x0 = core mask
     // x1 = field offset

     // generate a 0-based core number from the input mask
    clz   x2, x0
    mov   x0, #63
    sub   x0, x0, x2

     // x0 = core number (0-based)
     // x1 = field offset

     // determine if this is bootcore or secondary core
    cbnz  x0, 1f

     // get base address for bootcore data
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x2, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x2, =BC_PSCI_BASE
#endif
    add  x2, x2, x1
    b    2f

1:   // get base address for secondary core data

     // x0 = core number (0-based)
     // x1 = field offset

     // generate number of regions to offset
    sub   x0, x0, #1
    mov   x2, #SEC_REGION_OFFSET
    mul   x2, x2, x0

     // x1 = field offset
     // x2 = region offset

     // generate the total offset to data element
    add   x2, x2, #SEC_DATA_OFFSET
    sub   x1, x2, x1

     // x1 = total offset to data element

     // get the base address
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SECONDARY_TOP_QUERY
    bl   _getBaseAddrNS
    mov  x2, x13
     // restore LR
    mov  x30, x0
#else
    ldr   x2, =SECONDARY_TOP
#endif

     // apply offset to base addr
    sub   x2, x2, x1
2: 
     // x2 = data element address
   
    dc   ivac, x2
    dsb  sy
    isb 
     // read data
    ldr  x0, [x2]

    ret 

//-----------------------------------------------------------------------------

 // this function writes the specified data value into the specified cpu
 // core data area
 // in:  x0 = core mask lsb
 //      x1 = data field offset
 //      x2 = data value to write/store
 // out: none
 // uses x0, x1, x2, x3, [x13, x14, x15]
_setCoreData:
     // x0 = core mask
     // x1 = field offset
     // x2 = data value

    clz   x3, x0
    mov   x0, #63
    sub   x0, x0, x3

     // x0 = core number (0-based)
     // x1 = field offset
     // x2 = data value

     // determine if this is bootcore or secondary core
    cbnz  x0, 1f

     // get base address for bootcore data
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x3, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x3, =BC_PSCI_BASE
#endif
    add  x3, x3, x1
    b    2f

1:   // get base address for secondary core data

     // x0 = core number (0-based)
     // x1 = field offset
     // x2 = data value

     // generate number of regions to offset
    sub   x0, x0, #1
    mov   x3, #SEC_REGION_OFFSET
    mul   x3, x3, x0

     // x1 = field offset
     // x2 = data value
     // x3 = region offset

     // generate the total offset to data element
    add   x3, x3, #SEC_DATA_OFFSET
    sub   x1, x3, x1

     // x1 = total offset to data element
     // x2 = data value

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SECONDARY_TOP_QUERY
    bl   _getBaseAddrNS
    mov  x3, x13
     // restore LR
    mov  x30, x0
#else
    ldr   x3, =SECONDARY_TOP
#endif

     // apply offset to base addr
    sub   x3, x3, x1

2: 
     // x2 = data value
     // x3 = data element address
   
    str   x2, [x3]

    dc    cvac, x3
    dsb   sy
    isb  
    ret

//-----------------------------------------------------------------------------

 // this function returns the state of the task 1 start flag
 // in:  none
 // out: w0 = value of task1 start flag
 // uses x0, x1, [x13, x14, x15]
_get_task1_start:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    add  x1, x1, #TSK_START_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function sets the task1 start 
 // in:  w0 = value to set flag to
 // out: none
 // uses x0, x1
_set_task1_start:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    add  x1, x1, #TSK_START_OFFSET
    str  w0, [x1]
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function returns the state of the task 1 done flag
 // in:  none
 // out: x0 = value of task1 done flag
 // uses x0, x1
_get_task1_done:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    add  x1, x1, #TSK_DONE_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function sets the state of the task 1 done flag
 // in:  w0 = value to set flag to
 // out: none
 // uses x0, x1
_set_task1_done:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    add  x1, x1, #TSK_DONE_OFFSET
    str  w0, [x1]
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function returns the core mask of the core performing task 1
 // in:  
 // out: x0 = core mask lsb of the task 1 core
 // uses x0, x1
_get_task1_core:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    add  x1, x1, #TSK_CORE_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function saves the core mask of the core performing task 1
 // in:  x0 = core mask lsb of the task 1 core
 // out: none
 // uses x0, x1
_set_task1_core:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK1_BASE
#endif

    str  w0, [x1, #TSK_CORE_OFFSET]!
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function returns the state of the task 2 start flag
 // in:  none
 // out: w0 = value of task2 start flag
 // uses x0, x1
_get_task2_start:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    add  x1, x1, #TSK_START_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function sets the task2 start flag
 // in:  w0 = value to set flag to
 // out: none
 // uses x0, x1
_set_task2_start:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    add  x1, x1, #TSK_START_OFFSET
    str  w0, [x1]
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function returns the state of the task 2 done flag
 // in:  none
 // out: x0 = value of task2 done flag
 // uses x0, x1
_get_task2_done:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    add  x1, x1, #TSK_DONE_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function sets the state of the task 2 done flag
 // in:  w0 = value to set flag to
 // out: none
 // uses x0, x1
_set_task2_done:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    add  x1, x1, #TSK_DONE_OFFSET
    str  w0, [x1]
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function returns the core mask of the core performing task 2
 // in:  none
 // out: x0 = core mask lsb of the task 2 core
 // uses x0, x1
_get_task2_core:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    add  x1, x1, #TSK_CORE_OFFSET
    dc   ivac, x1
    isb
    ldr  w0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function saves the core mask of the core performing task 2
 // in:  x0 = core mask lsb of the task 2 core
 // out: none
 // uses x0, x1
_set_task2_core:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK2_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_TASK2_BASE
#endif

    str  w0, [x1, #TSK_CORE_OFFSET]!
    dc   cvac, x1
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function initializes a memory region to zero using 64-bit writes
 // the memory size must be a multiple of 8-bytes, and start on a 
 //  64-bit boundary
 // in:  x0 = base address
 //      x1 = size in bytes
 // out: none
 // uses x0, x1, x2
_initialize_memory:

    cbz   x1, 9f

     // determine what stride thru memory we can use
    ands  x2, x1, #0x3F
    b.eq  1f
    ands  x2, x1, #0x1F
    b.eq  3f
    ands  x2, x1, #0xF
    b.eq  5f
    b     7f


1:   // initialize using 64-byte strides

     // convert size to 64-byte chunks
    lsr   x1, x1, #6

     // x0 = start address
     // x1 = size in 64-byte chunks

    mov  x2, xzr
2:
    stp  x2, xzr, [x0]
    stp  x2, xzr, [x0, #16]
    stp  x2, xzr, [x0, #32]
    stp  x2, xzr, [x0, #48]
    dc   cvac, x0
    
    sub  x1, x1, #1
    cbz  x1, 9f
    add  x0, x0, #64
    b    2b

3:   // initialize using 32-byte strides

     // convert size to 32-byte chunks
    lsr   x1, x1, #5

     // x0 = start address
     // x1 = size in 32-byte chunks

    mov  x2, xzr
4:
    stp  x2, xzr, [x0]
    stp  x2, xzr, [x0, #16]
    dc   cvac, x0
    
    sub  x1, x1, #1
    cbz  x1, 9f
    add  x0, x0, #32
    b    4b

5:   // initialize using 16-byte strides

     // convert size to 16-byte chunks
    lsr   x1, x1, #4

     // x0 = start address
     // x1 = size in 16-byte chunks

    mov  x2, xzr
6:
    stp  x2, xzr, [x0]
    dc   cvac, x0
    
    sub  x1, x1, #1
    cbz  x1, 9f
    add  x0, x0, #16
    b    6b

7:   // initialize using 8-byte strides

     // convert size to 8-byte chunks
    lsr   x1, x1, #3

     // x0 = start address
     // x1 = size in 8-byte chunks
8:
    str  xzr, [x0]
    dc   cvac, x0
    
    sub  x1, x1, #1
    cbz  x1, 9f
    add  x0, x0, #8
    b    8b

9:
    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function initializes the smc global data entries
 // Note: the constant LAST_SMC_GLBL_OFFSET must reference the last entry in the
 //       smc global region
 // in:  none
 // out: none
 // uses x0, x1, x2
_init_global_data:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_GLBL_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_GLBL_BASE
#endif

     // x1 = SMC_GLBL_BASE

    mov x2, #LAST_SMC_GLBL_OFFSET
    add x2, x2, x1
1:
    str  xzr, [x1]
    dc   cvac, x1
    cmp  x2, x1
    add  x1, x1, #8
    b.hi 1b

    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function gets the value of the specified global data element
 // in:  x0 = offset of data element
 // out: x0 = requested data element
 // uses x0, x1
_get_global_data:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_GLBL_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x1
    mov  x1, x13
#else
    ldr  x1, =SMC_GLBL_BASE
#endif
    add  x1, x1, x0
    dc   ivac, x1
    isb

    ldr  x0, [x1]
    ret

//-----------------------------------------------------------------------------

 // this function sets the value of the specified global data element
 // in:  x0 = offset of data element
 //      x1 = value to write
 // out: none
 // uses x0, x1, x2
_set_global_data:

#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x2, x30
     // request base address
    mov  x13, #SMC_GLBL_BASE_QUERY
    bl   _getBaseAddrNS
     // restore LR
    mov  x30, x2
    mov  x2, x13
#else
    ldr  x2, =SMC_GLBL_BASE
#endif
    add  x0, x0, x2
    str  x1, [x0]
    dc   cvac, x0

    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function initializes the per-core stack area, and sets SP_EL3 to
 // point to the start of this area
 // Note: stack is full-descending
 // in:  x0 = core mask lsb
 // out: none
 // uses x0, x1, x2, [x13, x14, x15]
_init_stack_percpu:

     // x0 = core mask

     // generate a 0-based core number from the input mask
    clz   x1, x0
    mov   x0, #63
    sub   x0, x0, x1

     // x0 = current core number (0-based)

     // determine if this is bootcore or secondary core
    cbnz  x0, 1f

     // get stack address & size for bootcore
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #BC_STACK_TOP_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr   x1, =BC_STACK_TOP
#endif
    mov   x2, #BC_STACK_SIZE
    b     2f

1:   // get stack address & size for secondary core

     // x0 = current core number (0-based)

     // offset past the other cores stack space
    sub   x0, x0, #1
    mov   x2, #SEC_REGION_OFFSET
    mul   x2, x2, x0
    
     // add in offset to this cores stack to get a total offset
    add   x2, x2, #SEC_DATA_OFFSET

     // get base address
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SECONDARY_TOP_QUERY
    bl   _getBaseAddrNS
    mov  x1, x13
     // restore LR
    mov  x30, x0
#else
    ldr   x1, =SECONDARY_TOP
#endif

     // get address to top of stack
    sub   x1, x1, x2
    mov   x2, #SEC_STACK_SIZE

2:   // initialize the stack area

     // x1 = address of top-of-stack
     // x2 = stack size in bytes

     // align address to a quad-word boundary
    bic   x1, x1, #0xf

     // x2 = size of stack area in bytes
     // convert bytes to 16-byte chunks
    lsr   x2, x2, #4

     // set the stack pointer
    mov   sp, x1

3:
     // descending-full stack - use pre-indexed addressing
    stp   xzr, xzr, [x1, #-16]!
    dc    cvac, x1
    subs  x2, x2, #1
    b.gt  3b

     // the sixteen bytes following the bottom of the stack are reserved
     // for an integrity buffer, used to help determine if the stack has
     // grown beyond its lower boundary - initialize this buffer
     // 
     //    .8byte 0x0000000000000000
     //    .8byte 0xFFFFFFFFFFFFFFFF
     //
    str   xzr, [x1, #-8]!
    dc    cvac, x1
    mvn   x0, xzr
    str   x0,  [x1, #-8]!
    dc    cvac, x1

    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

 // this function sets up the memory bank info data areas in memory
 // Note: this function MUST be called before initializing ddr
 // in:  none
 // out: none
 // uses x0, x1, x2, x3
_init_membank_data:

 // only valid if ddr is being initialized
#if (DDR_INIT)
    mov   x3, x30

     // get base address and size of memory bank data area
#if (DATA_LOC == DATA_IN_DDR)
     // request base address
    mov  x13, #MEMBANK_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x0, x13
#else
    ldr  x0, =MEMBANK_BASE
#endif
    mov  x1, #MEMBANK_REGION_SIZE
    bl   _initialize_memory

    mov  x30, x3
#endif

    ret

//-----------------------------------------------------------------------------

#if DDR_INIT
#if PSCI_TEST

#define  MEMBANK_COUNT  4
#define  BANK1_ADDR     0x0080000000
#define  BANK2_ADDR     0x8000000000
#define  BANK3_ADDR     OCRAM_BASE_ADDR
#define  BANK4_ADDR     0x00F0000000
#define  BANK1_SIZE     0x080000000       // 2GB
#define  BANK2_SIZE     0x200000000       // 8GB
#define  BANK3_SIZE     OCRAM_SIZE_IN_BYTES
#define  BANK4_SIZE     0x080000000       // 2GB

 // in:  none
 // out: none
 // uses x0, x1, x2, x3
_populate_membank_data:

     // get the address of the bank count var
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #MEMBANK_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x2, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x2, =MEMBANK_BASE
#endif
     // store the number of memory banks
    mov  x1, #MEMBANK_COUNT
    str  x1, [x2]
    dc    cvac, x2

     // move address to first data struc
    add  x2, x2, #MEMBANK_CNT_SIZE

     // x2 = address of first membank info struc

     // populate the first structure
    mov   w1, #MEMBANK_STATE_VALID
    str   w1, [x2, #MEMDATA_STATE_OFFSET]
    mov   w3, #MEMBANK_TYPE_DDR
    str   w3, [x2, #MEMDATA_TYPE_OFFSET]
    mov   x0, #BANK1_ADDR
    str   x0, [x2, #MEMDATA_ADDR_OFFSET]
    mov   x1, #BANK1_SIZE
    str   x1, [x2, #MEMDATA_SIZE_OFFSET]
    dc    cvac, x2
   
     // move to the next data structure
    add  x2, x2, #MEMBANK_DATA_OFFSET

     // populate the second structure
    mov   w1, #MEMBANK_STATE_VALID
    str   w1, [x2, #MEMDATA_STATE_OFFSET]
    mov   w3, #MEMBANK_TYPE_DDR
    str   w3, [x2, #MEMDATA_TYPE_OFFSET]
    mov   x0, #BANK2_ADDR
    str   x0, [x2, #MEMDATA_ADDR_OFFSET]
    mov   x1, #BANK2_SIZE
    str   x1, [x2, #MEMDATA_SIZE_OFFSET]
    dc    cvac, x2
   
     // move to the next data structure
    add  x2, x2, #MEMBANK_DATA_OFFSET

     // populate the third structure
    mov   w1, #MEMBANK_STATE_VALID
    str   w1, [x2, #MEMDATA_STATE_OFFSET]
    mov   w3, #MEMBANK_TYPE_DDR
    str   w3, [x2, #MEMDATA_TYPE_OFFSET]
    mov   x0, #BANK3_ADDR
    str   x0, [x2, #MEMDATA_ADDR_OFFSET]
    mov   x1, #BANK3_SIZE
    str   x1, [x2, #MEMDATA_SIZE_OFFSET]
    dc    cvac, x2
   
     // move to the next data structure
    add  x2, x2, #MEMBANK_DATA_OFFSET

     // populate the fourth structure
    mov   w1, #MEMBANK_STATE_INVALID
    str   w1, [x2, #MEMDATA_STATE_OFFSET]
    dc    cvac, x2

    dsb  sy
    isb
    ret

#endif
#endif

//-----------------------------------------------------------------------------

 // this function initializes the core data areas
 // only executed by the boot core
 // in:   none
 // out:  none
 // uses: x0, x1, x2, x3, x4, x5, x6, x7, [x13, x14, x15]
_initialize_psci:
    mov   x7, x30

     // initialize the bootcore psci data
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #BC_PSCI_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x5, x13
     // restore LR
    mov  x30, x0
#else
    ldr   x5, =BC_PSCI_BASE
#endif
    mov   x6, #CORE_RELEASED

    str   x6,  [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5], #8
    str   xzr, [x5]

     // see if we have any secondary cores
    mov   x4, #CPU_MAX_COUNT
    sub   x4, x4, #1
    cbz   x4, 3f

     // initialize the secondary core's psci data
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x0, x30
     // request base address
    mov  x13, #SECONDARY_TOP_QUERY
    bl   _getBaseAddrNS
    mov  x5, x13
     // restore LR
    mov  x30, x0
#else
    ldr  x5, =SECONDARY_TOP
#endif
     // core mask lsb for core 1
    mov  x3, #2
    sub  x5, x5, #SEC_DATA_OFFSET

     // x3 = core1 mask lsb
     // x4 = number of secondary cores
     // x5 = core1 psci data base address
2:
     // set core state in x6
    mov  x0, x3
    mov  x6, #CORE_IN_RESET
    bl   _soc_ck_disabled
    cbz  x0, 1f
    mov  x6, #CORE_DISABLED
1:
    str   x6,  [x5, #CORE_STATE_DATA]
    str   xzr, [x5, #SPSR_EL3_DATA]
    str   xzr, [x5, #CNTXT_ID_DATA]
    str   xzr, [x5, #START_ADDR_DATA]
    str   xzr, [x5, #LINK_REG_DATA]
    str   xzr, [x5, #GICC_CTLR_DATA]
    str   xzr, [x5, #ABORT_FLAG_DATA]
    str   xzr, [x5, #SCTLR_DATA]
    str   xzr, [x5, #CPUECTLR_DATA]
    str   xzr, [x5, #AUX_01_DATA]
    str   xzr, [x5, #AUX_02_DATA]
    str   xzr, [x5, #AUX_03_DATA]
    str   xzr, [x5, #AUX_04_DATA]
    str   xzr, [x5, #AUX_05_DATA]
    str   xzr, [x5, #SCR_EL3_DATA]
    str   xzr, [x5, #HCR_EL2_DATA]

    sub   x4, x4, #1
    cbz   x4, 3f

     // generate next core mask
    lsl  x3, x3, #1
    
     // decrement base address to next data area
    sub  x5, x5, #SEC_REGION_OFFSET
    b    2b
3:
    mov   x30, x7
    ret

//-----------------------------------------------------------------------------

 // this function initializes the soc init task flags
 // in:  none
 // out: none
 // uses x0, x1, [x13, x14, x15]
_init_task_flags:

     // get the base address of the first task structure
#if (DATA_LOC == DATA_IN_DDR)
     // save LR
    mov  x1, x30
     // request base address
    mov  x13, #SMC_TASK1_BASE_QUERY
    bl   _getBaseAddrNS
    mov  x0, x13
     // restore LR
    mov  x30, x1
#else
    ldr  x0, =SMC_TASK1_BASE
#endif

     // x0 = task1 base address

    str  wzr, [x0, #TSK_START_OFFSET]
    str  wzr, [x0, #TSK_DONE_OFFSET]
    str  wzr, [x0, #TSK_CORE_OFFSET]
    dc   cvac, x0

     // move to task2 structure
    add  x0, x0, #SMC_TASK_OFFSET

    str  wzr, [x0, #TSK_START_OFFSET]
    str  wzr, [x0, #TSK_DONE_OFFSET]
    str  wzr, [x0, #TSK_CORE_OFFSET]
    dc   cvac, x0

     // move to task3 structure
    add  x0, x0, #SMC_TASK_OFFSET

    str  wzr, [x0, #TSK_START_OFFSET]
    str  wzr, [x0, #TSK_DONE_OFFSET]
    str  wzr, [x0, #TSK_CORE_OFFSET]
    dc   cvac, x0

     // move to task4 structure
    add  x0, x0, #SMC_TASK_OFFSET

    str  wzr, [x0, #TSK_START_OFFSET]
    str  wzr, [x0, #TSK_DONE_OFFSET]
    str  wzr, [x0, #TSK_CORE_OFFSET]
    dc   cvac, x0

    dsb  sy
    isb
    ret

//-----------------------------------------------------------------------------

