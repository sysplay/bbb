#!/bin/bash
 
DPATH="/sys/kernel/debug/tracing"
PID=$$

## Quick basic checks
[ `id -u` -ne 0  ]  &&  { echo "needs to be root" ; exit 1; }  # check for root permissions
[ $# -ne 0 ] && { echo "Usage: $0"; exit 1; } # check for no args to this function
 
# stop the tracing
echo 0 > $DPATH/tracing_on

# clear existing process id
echo > $DPATH/set_ftrace_pid
 
# flush existing trace data
echo nop > $DPATH/current_tracer
