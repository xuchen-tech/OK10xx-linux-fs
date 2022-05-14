#!/bin/bash

aarch64-linux-gnu-gcc -O3 -funroll-all-loops --param max-inline-insns-auto=550 -static dhry21a.c dhry21b.c timers.c -o dhrystone
