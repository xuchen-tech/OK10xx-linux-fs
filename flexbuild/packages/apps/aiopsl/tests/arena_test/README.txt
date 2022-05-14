The following file includes the instructions for arena_test demo.

===========================================
Demo possible modifications:
===========================================
1. The user may add application additional initialization inside app_init()
2. The user may add packet processing code inside app_process_packet_flow0()
===========================================
Setup
===========================================
1. Install Code Warrior (see Release Note for the compatible CW version).
2. Download the linux version of the simulator (see Release Note for the compatible LS_SIM version).
3. Copy the files ls2085a_sim_init_params.cfg and ls2085a_sys_test.cfg
   from the source tree at: aiopsl/build/aiop_sim/sim_files.
   into the simulator folder at: dtsim_release/linux64/
4. Update the �LD_LIBRARY_PATH� variable to point to the simulator folder.
   setenv LD_LIBRARY_PATH {$LD_LIBRARY_PATH}:/home/user/LS_SIM_<version>/dtsim_release/linux64
5. Copy the layout file (dpl.dtb) from aiopsl/misc/setup/ to the simulator folder.
6. Copy �arena_test_40.pcap� from the source tree at: aiopsl/misc/setup/ into the simulator folder

===========================================
Execution flow
===========================================
1. Import the MC and AIOP projects into CodeWarrior:
   mc/build/mc_sim/mc_app/.project
   aiopsl/build/aiop_sim/apps/app_process_packet/.project
2. Build both projects in CW.
3. Copy the resulting ELF file from the build project folder(aiop_app.elf)
   to the simulator folder (same location as cfg files).
4. Run the simulator:
   ./ccssim2 -port 42333
             -imodel "ls_sim_init_file=ls2085a_sim_init_params.cfg"
             -smodel "ls_sim_config_file=ls2085a_sys_test.cfg"
5. Launch mc_app using AFM connection.
   Don't forget to update simulator server IP and port in debug configuration - 42333.
6. Attach app_process_packet (make sure to un-mark initialization files).
7. After MC reaches main(), run tio console:
   ./bin/tio_console -hub localhost:42975 -ser duart2_1 duart2_0
8. Run mc_app.
9. Run �tio capture�:
   ./fm_tio_capture -hub localhost:42975 -ser w0_m1 -verbose_level 2
   Here you'll be able to capture sent and received packets.
10. Run �tio inject�:
   ./fm_tio_inject -hub localhost:42975 -ser w0_m1 -file arena_test_40.pcap -verbose_level 2
   This will send packets to the AIOP.
11. Set break point inside app_process_packet_flow0() and push "Resume / Multi core Resume" button to run and see that
    it's activated on each packet.
12. The packet will also be captured by the tio_capture

===========================================
Possible modifications:
===========================================
1. The user may add application additional initialization inside app_init()
2. The user may add packet processing code inside app_process_packet()
3. The user may use different tio port and update it inside ls2085a_sys_test.cfg
4. The user may use different simulator port
5. The demo runs in MC integrated mode. MC loads AIOP and kicks the AIOP cores.
   Please note that the standalone mode is being phased out and is no longer verified.

===========================================
ARENA sets the following default values for every NI:
===========================================
MTU = maximal value
Same parser profile id for all NIs



