#!/bin/bash

# Copyright 2008-2013 Freescale Semiconductor, Inc.
#
# There are, unfortunately, numerous files in CodeWarrior-Eclipse projects that 
# automatically undergo changes when opened locally by a developer. These 
# constitute false positives when trying to determine what changes the developer
# has made locally. The workaround is to tell git that you don't care that the
# files have changed.
# 
# The state can be reverted with 
#		git update-index --no-assume-unchanged

if [ "$1" = off ]
then 
	prop="--no-assume-unchanged"
else 
	prop="--assume-unchanged"
fi


git update-index $prop "test/suite/project/cw/Debug_Settings/SDOS flib test Simics bootm.launch"
git update-index $prop "test/suite/project/cw/ReferencedRSESystems.xml"
