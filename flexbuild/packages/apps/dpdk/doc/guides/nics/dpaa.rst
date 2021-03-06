..  BSD LICENSE
    Copyright 2017 NXP

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of NXP nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

DPAA Poll Mode Driver
=====================

The DPAA NIC PMD (**librte_pmd_dpaa**) provides poll mode driver
support for the inbuilt NIC found in the **NXP DPAA** SoC family.

More information can be found at `NXP Official Website
<http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/qoriq-arm-processors:QORIQ-ARM>`_.

NXP DPAA (Data Path Acceleration Architecture - Gen 1)
------------------------------------------------------

This section provides an overview of the NXP DPAA architecture
and how it is integrated into the DPDK.

Contents summary

- DPAA overview
- DPAA driver architecture overview

.. _dpaa_overview:

DPAA Overview
~~~~~~~~~~~~~

Reference: `FSL DPAA Architecture <http://www.nxp.com/assets/documents/data/en/white-papers/QORIQDPAAWP.pdf>`_.

The QorIQ Data Path Acceleration Architecture (DPAA) is a set of hardware
components on specific QorIQ series multicore processors. This architecture
provides the infrastructure to support simplified sharing of networking
interfaces and accelerators by multiple CPU cores, and the accelerators
themselves.

DPAA includes:

- Cores
- Network and packet I/O
- Hardware offload accelerators
- Infrastructure required to facilitate flow of packets between the components above

Infrastructure components are:

- The Queue Manager (QMan) is a hardware accelerator that manages frame queues.
  It allows  CPUs and other accelerators connected to the SoC datapath to
  enqueue and dequeue ethernet frames, thus providing the infrastructure for
  data exchange among CPUs and datapath accelerators.
- The Buffer Manager (BMan) is a hardware buffer pool management block that
  allows software and accelerators on the datapath to acquire and release
  buffers in order to build frames.

Hardware accelerators are:

- SEC - Cryptographic accelerator
- PME - Pattern matching engine

The Network and packet I/O component:

- The Frame Manager (FMan) is a key component in the DPAA and makes use of the
  DPAA infrastructure (QMan and BMan). FMan  is responsible for packet
  distribution and policing. Each frame can be parsed, classified and results
  may be attached to the frame. This meta data can be used to select
  particular QMan queue, which the packet is forwarded to.


DPAA DPDK - Poll Mode Driver Overview
-------------------------------------

This section provides an overview of the drivers for DPAA:

* Bus driver and associated "DPAA infrastructure" drivers
* Functional object drivers (such as Ethernet).

Brief description of each driver is provided in layout below as well as
in the following sections.

.. code-block:: console

                                       +------------+
                                       | DPDK DPAA  |
                                       |    PMD     |
                                       +-----+------+
                                             |
                                       +-----+------+       +---------------+
                                       :  Ethernet  :.......| DPDK DPAA     |
                    . . . . . . . . .  :   (FMAN)   :       | Mempool driver|
                   .                   +---+---+----+       |  (BMAN)       |
                  .                        ^   |            +-----+---------+
                 .                         |   |<enqueue,         .
                .                          |   | dequeue>         .
               .                           |   |                  .
              .                        +---+---V----+             .
             .      . . . . . . . . . .: Portal drv :             .
            .      .                   :            :             .
           .      .                    +-----+------+             .
          .      .                     :   QMAN     :             .
         .      .                      :  Driver    :             .
    +----+------+-------+              +-----+------+             .
    |   DPDK DPAA Bus   |                    |                    .
    |   driver          |....................|.....................
    |   /bus/dpaa       |                    |
    +-------------------+                    |
                                             |
    ========================== HARDWARE =====|========================
                                            PHY
    =========================================|========================

In the above representation, solid lines represent components which interface
with DPDK RTE Framework and dotted lines represent DPAA internal components.

DPAA Bus driver
~~~~~~~~~~~~~~~

The DPAA bus driver is a ``rte_bus`` driver which scans the platform like bus.
Key functions include:

- Scanning and parsing the various objects and adding them to their respective
  device list.
- Performing probe for available drivers against each scanned device
- Creating necessary ethernet instance before passing control to the PMD

DPAA NIC Driver (PMD)
~~~~~~~~~~~~~~~~~~~~~

DPAA PMD is traditional DPDK PMD which provides necessary interface between
RTE framework and DPAA internal components/drivers.

- Once devices have been identified by DPAA Bus, each device is associated
  with the PMD
- PMD is responsible for implementing necessary glue layer between RTE APIs
  and lower level QMan and FMan blocks.
  The Ethernet driver is bound to a FMAN port and implements the interfaces
  needed to connect the DPAA network interface to the network stack.
  Each FMAN Port corresponds to a DPDK network interface.


Features
^^^^^^^^

  Features of the DPAA PMD are:

  - Multiple queues for TX and RX
  - Receive Side Scaling (RSS)
  - Packet type information
  - Checksum offload
  - Promiscuous mode

DPAA Mempool Driver
~~~~~~~~~~~~~~~~~~~

DPAA has a hardware offloaded buffer pool manager, called BMan, or Buffer
Manager.

- Using standard Mempools operations RTE API, the mempool driver interfaces
  with RTE to service each mempool creation, deletion, buffer allocation and
  deallocation requests.
- Each FMAN instance has a BMan pool attached to it during initialization.
  Each Tx frame can be automatically released by hardware, if allocated from
  this pool.


Whitelisting & Blacklisting
---------------------------

For blacklisting a DPAA device, following commands can be used.

 .. code-block:: console

    <dpdk app> <EAL args> -b "dpaa_bus.fmX-macY" -- ...
    e.g. "dpaa_bus:fm1-mac4"

Supported DPAA SoCs
-------------------

- LS1043A/LS1023A
- LS1046A/LS1026A

Prerequisites
-------------

There are three main pre-requisities for executing DPAA PMD on a DPAA
compatible board:

1. **ARM 64 Tool Chain**

   For example, the `*aarch64* Linaro Toolchain <https://releases.linaro.org/components/toolchain/binaries/6.4-2017.08/aarch64-linux-gnu/>`_.

2. **Linux Kernel**

   It can be obtained from `NXP's Github hosting <https://github.com/qoriq-open-source/linux>`_.

3. **Rootfile system**

   Any *aarch64* supporting filesystem can be used. For example,
   Ubuntu 15.10 (Wily) or 16.04 LTS (Xenial) userland which can be obtained
   from `here <http://cdimage.ubuntu.com/ubuntu-base/releases/16.04/release/ubuntu-base-16.04.1-base-arm64.tar.gz>`_.

4. **FMC Tool**

   Before any DPDK application can be executed, the Frame Manager Configuration
   Tool (FMC) need to be executed to set the configurations of the queues. This
   includes the queue state, RSS and other policies.
   This tool can be obtained from `NXP (Freescale) Public Git Repository <https://github.com/qoriq-open-source/fmc>`_.

   This tool needs configuration files which are available in the
   :ref:`DPDK Extra Scripts <extra_scripts>`, described below for DPDK usages.

As an alternative method, DPAA PMD can also be executed using images provided
as part of SDK from NXP. The SDK includes all the above prerequisites necessary
to bring up a DPAA board.

The following dependencies are not part of DPDK and must be installed
separately:

- **NXP Linux SDK**

  NXP Linux software development kit (SDK) includes support for family
  of QorIQ?? ARM-Architecture-based system on chip (SoC) processors
  and corresponding boards.

  It includes the Linux board support packages (BSPs) for NXP SoCs,
  a fully operational tool chain, kernel and board specific modules.

  SDK and related information can be obtained from:  `NXP QorIQ SDK  <http://www.nxp.com/products/software-and-tools/run-time-software/linux-sdk/linux-sdk-for-qoriq-processors:SDKLINUX>`_.


.. _extra_scripts:

- **DPDK Extra Scripts**

  DPAA based resources can be configured easily with the help of ready scripts
  as provided in the DPDK Extra repository.

  `DPDK Extras Scripts <https://github.com/qoriq-open-source/dpdk-extras>`_.

Currently supported by DPDK:

- NXP SDK **2.0+**.
- Supported architectures:  **arm64 LE**.

- Follow the DPDK :ref:`Getting Started Guide for Linux <linux_gsg>`
  to setup the basic DPDK environment.

.. note::

   Some part of dpaa bus code (qbman and fman - library) routines are
   dual licensed (BSD & GPLv2), however they are used as BSD in DPDK in userspace.

Pre-Installation Configuration
------------------------------

Config File Options
~~~~~~~~~~~~~~~~~~~

The following options can be modified in the ``config`` file.
Please note that enabling debugging options may affect system performance.

- ``CONFIG_RTE_LIBRTE_DPAA_BUS`` (default ``n``)

  By default it is enabled only for defconfig_arm64-dpaa-* config.
  Toggle compilation of the ``librte_bus_dpaa`` driver.

- ``CONFIG_RTE_LIBRTE_DPAA_PMD`` (default ``n``)

  By default it is enabled only for defconfig_arm64-dpaa-* config.
  Toggle compilation of the ``librte_pmd_dpaa`` driver.

- ``CONFIG_RTE_LIBRTE_DPAA_DEBUG_DRIVER`` (default ``n``)

  Toggles display of bus configurations and enables a debugging queue
  to fetch error (Rx/Tx) packets to driver. By default, packets with errors
  (like wrong checksum) are dropped by the hardware.

- ``CONFIG_RTE_LIBRTE_DPAA_HWDEBUG`` (default ``n``)

  Enables debugging of the Queue and Buffer Manager layer which interacts
  with the DPAA hardware.

- ``CONFIG_RTE_MBUF_DEFAULT_MEMPOOL_OPS`` (default ``dpaa``)

  This is not a DPAA specific configuration - it is a generic RTE config.
  For optimal performance and hardware utilization, it is expected that DPAA
  Mempool driver is used for mempools. For that, this configuration needs to
  enabled.

Environment Variables
~~~~~~~~~~~~~~~~~~~~~

DPAA drivers uses the following environment variables to configure its
state during application initialization:

- ``DPAA_NUM_RX_QUEUES`` (default 1)

  This defines the number of Rx queues configured for an application, per
  port. Hardware would distribute across these many number of queues on Rx
  of packets.
  In case the application is configured to use lesser number of queues than
  configured above, it might result in packet loss (because of distribution).

- ``DPAA_PUSH_QUEUES_NUMBER`` (default 4)

  This defines the number of High performance queues to be used for ethdev Rx.
  These queues use one private HW portal per queue configured, so they are
  limited in the system. The first configured ethdev queues will be
  automatically be assigned from the these high perf PUSH queues. Any queue
  configuration beyond that will be standard Rx queues. The application can
  choose to change their number if HW portals are limited.
  The valid values are from '0' to '4'. The valuse shall be set to '0' if the
  application want to use eventdev with DPAA device.


Driver compilation and testing
------------------------------

Refer to the document :ref:`compiling and testing a PMD for a NIC <pmd_build_and_test>`
for details.

#. Running testpmd:

   Follow instructions available in the document
   :ref:`compiling and testing a PMD for a NIC <pmd_build_and_test>`
   to run testpmd.

   Example output:

   .. code-block:: console

      ./arm64-dpaa-linuxapp-gcc/testpmd -c 0xff -n 1 \
        -- -i --portmask=0x3 --nb-cores=1 --no-flush-rx

      .....
      EAL: Registered [pci] bus.
      EAL: Registered [dpaa] bus.
      EAL: Detected 4 lcore(s)
      .....
      EAL: dpaa: Bus scan completed
      .....
      Configuring Port 0 (socket 0)
      Port 0: 00:00:00:00:00:01
      Configuring Port 1 (socket 0)
      Port 1: 00:00:00:00:00:02
      .....
      Checking link statuses...
      Port 0 Link Up - speed 10000 Mbps - full-duplex
      Port 1 Link Up - speed 10000 Mbps - full-duplex
      Done
      testpmd>

Limitations
-----------

Platform Requirement
~~~~~~~~~~~~~~~~~~~~

DPAA drivers for DPDK can only work on NXP SoCs as listed in the
``Supported DPAA SoCs``.

Maximum packet length
~~~~~~~~~~~~~~~~~~~~~

The DPAA SoC family support a maximum of a 10240 jumbo frame. The value
is fixed and cannot be changed. So, even when the ``rxmode.max_rx_pkt_len``
member of ``struct rte_eth_conf`` is set to a value lower than 10240, frames
up to 10240 bytes can still reach the host interface.

Multiprocess Support
~~~~~~~~~~~~~~~~~~~~

Current version of DPAA driver doesn't support multi-process applications
where I/O is performed using secondary processes. This feature would be
implemented in subsequent versions.
