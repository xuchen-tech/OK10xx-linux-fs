#!/bin/bash

BIN=""
COMPILE_OUTPUT="./compile_output"

DEF_DPRC="dprc.2"
DEF_AIOP_IMAGE="./something"

function clean_and_compile() {
	make clean
	make
	return $?
}

function test_help() {
	echo "Executing: $BIN help"
	echo
	$BIN help
}

function test_load() {
	echo "Executing: $BIN load \"$@\""
	echo
	$BIN load $@
}

function test_reset() {
	echo "Executing: $BIN reset \"$@\""
	echo
	$BIN reset $@
}

function test_gettod() {
	echo "Executing: $BIN gettod \"$@\""
	echo
	$BIN gettod $@
}

function test_settod() {
	echo "Executing: $BIN settod \"$@\""
	echo
	$BIN settod $@
}

function test_status() {
	echo "Executing: $BIN status \"$@\""
	echo
	$BIN "status" $@
}
#===============================================================

TEST_COUNTER=0
function run_test() {
	echo "========================================================"
	echo "        Running Test: ID:$1: Name:$2 COUNT=$TEST_COUNTER"
	echo "----- $4 ----"
	shift
	$1 $2
	echo "========================================================"
	TEST_COUNTER=$(($TEST_COUNTER + 1))
}

function usage() {
	echo "<Test Script> <Path to the AIOP Tool>"
	echo
}

function test_init() {
	TEST_COUNTER=0
	if [ "$DPRC" == "" ]

	then
		DPRC=$DEF_DPRC
	fi

	if [ "$AIOP_FILE" == "" ]
	then
		AIOP_FILE=$DEF_AIOP_IMAGE
	fi

}

function test_summary() {
	echo
	echo "==== Executed: $TEST_COUNTER; Pass: <Not Implemented Yet> ==="
	echo
}

if [ "$1" == "" ]
then
	usage
	exit 1
else
	BIN=$1
fi

clean_and_compile 2>$COMPILE_OUTPUT 1>&2
if [ $? != 0 ]
then
	echo "Unable to compile. Check logs"
	exit 1
fi

test_init

####### All Test Cases are above ########

### Help Test
### ID Range: 1 - 9
run_test 1 test_help

### Load Test
### ID Range: 10 - 30

run_test 10 test_load " " 0
run_test 11 test_load "-g" 0
run_test 12 test_load "-g $DPRC" 0
run_test 13 test_load "-f $AIOP_FILE" 1
run_test 14 test_load "-f $AIOP_FILE -g $DPRC" 1
run_test 15 test_load "-f $AIOP_FILE -g $DPRC -r" 1
run_test 16 test_load "-r -d" 0
run_test 17 test_load "-g $DPRC -t 1 -r -f $AIOPF_FILE" 0
run_test 18 test_load "-g $DPRC -t 1 -r -f $AIOPF_FILE -d" 0
run_test 19 test_load "--container" 0
run_test 20 test_load "--container $DPRC" 0
run_test 21 test_load "--file $AIOP_FILE" 1
run_test 22 test_load "--file $AIOP_FILE -g $DPRC" 1
run_test 23 test_load "--file $AIOP_FILE --container $DPRC -r" 1
run_test 24 test_load "--reset --debug" 0
run_test 25 test_load "-g $DPRC --time 1 --reset --file $AIOPF_FILE" 0
run_test 26 test_load "-g $DPRC --time 1 -r -f $AIOPF_FILE --debug" 0
run_test 27 test_load "-g $DPRC -c 16 -d" 0
run_test 28 test_load "-g $DPRC -f $AIOP_FILE -c 16 -d" 1
run_test 29 test_load "-g $DPRC -f $AIOP_FILE -c 0 -d" 1
run_test 30 test_load "-g $DPRC -f $AIOP_FILE --threadpercore 8 -d" 1


### Reset Test
### ID Range: 31 - 60
run_test 31 test_reset " " 1
run_test 32 test_reset "-g" 0
run_test 33 test_reset "-g $DPRC" 1
run_test 34 test_reset "-f $AIOP_FILE" 0
run_test 35 test_reset "-f $AIOP_FILE -g $DPRC" 0
run_test 36 test_reset "-r" 1
run_test 37 test_reset "-g $DPRC -t 1 -r -f $AIOPF_FILE" 0
run_test 38 test_reset "--container $DPRC" 1
run_test 39 test_reset "--file $AIOP_FILE" 0
run_test 40 test_reset "-f $AIOP_FILE --container $DPRC" 0
run_test 41 test_reset "--reset" 1
run_test 42 test_reset "--container $DPRC --time 1 --reset --file $AIOPF_FILE" 0

### Gettod Test
### ID Range: 61 - 80
run_test 61 test_gettod "" 1
run_test 62 test_gettod "-g" 0
run_test 63 test_gettod "-g $DPRC" 1
run_test 64 test_gettod "-f $AIOP_FILE" 0
run_test 65 test_gettod "-f $AIOP_FILE -g $DPRC" 0
run_test 66 test_gettod "-r" 0
run_test 66 test_gettod "--debug" 1
run_test 67 test_gettod "--container $DPRC --time 1 --reset --file $AIOPF_FILE" 0
run_test 68 test_gettod "--container $DPRC --debug" 1

### Settod Test
### ID Range: 81 - 110
run_test 81 test_settod " " 0
run_test 82 test_settod "-g" 0
run_test 83 test_settod "-g $DPRC" 0
run_test 84 test_settod "-f $AIOP_FILE" 0
run_test 85 test_settod "-f $AIOP_FILE -g $DPRC" 0
run_test 86 test_settod "-r" 0
run_test 87 test_settod "-g DPRC.3 -t 10992092" 1
run_test 88 test_settod "-t" 0
run_test 89 test_settod "-t 1" 1
run_test 90 test_settod "-t 1099283022" 1
run_test 91 test_settod "-g DPRC.3 -t 10992830A" 0
run_test 92 test_settod "-g DPRC.3 -t A10992830" 0
run_test 93 test_settod "-g DPRC.3 -t 11:11:11" 0
run_test 94 test_settod "-g DPRC.3 -t 1110290199092090909229919" 0
run_test 95 test_settod "-g DPRC.3 -t -10029" 0
run_test 95 test_settod "-g DPRC.3 -t 0x10029" 1
run_test 95 test_settod "-g DPRC.3 -t 010029" 1
run_test 96 test_settod "-g $DPRC -t 1 -r -f $AIOPF_FILE" 0
run_test 97 test_settod "--container $DPRC" 0
run_test 98 test_settod "--file $AIOP_FILE" 0
run_test 97 test_settod "--container $DPRC --time 1090919 --debug" 1

### Status Test
### ID Range: 111 - 130
run_test 111 test_status " " 1
run_test 112 test_status "-g" 0
run_test 113 test_status "-g $DPRC" 1
run_test 114 test_status "-f $AIOP_FILE" 0
run_test 115 test_status "-f $AIOP_FILE -g $DPRC" 0
run_test 116 test_status "-r" 0
run_test 117 test_status "-t 1" 0
run_test 118 test_status "-g $DPRC -t 1 -r -f $AIOPF_FILE" 0
run_test 119 test_status "--container $DPRC" 1
run_test 120 test_status "--container $DPRC --debug" 1

####### All Test Cases are above ########

test_summary
