#!/bin/bash

make PORT_CFLAGS="-O3 -funroll-all-loops --param max-inline-insns-auto=550" PORT_DIR=linux64
