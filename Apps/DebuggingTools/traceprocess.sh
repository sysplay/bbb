#!/bin/bash
 
DPATH="/sys/kernel/debug/tracing"
PID=$$

## Quick basic checks
[ `id -u` -ne 0  ]  &&  { echo "Needs to be root"; exit 1; }  # check for root permissions
mount | grep -i debugfs &> /dev/null
[ $? -ne 0 ] && { echo "debugfs not mounted, mount it first"; exit 1; } # check for debugfs mount
[ -z $1 ] && { echo "Usage: $0 <process_name>"; exit 1; } # check for args to this function
 
# flush existing trace data
echo nop > $DPATH/current_tracer
 
# set function tracer
echo function > $DPATH/current_tracer
 
# write current process id to set_ftrace_pid file
echo $PID > $DPATH/set_ftrace_pid
 
# start the tracing
echo 1 > $DPATH/tracing_on

# execute the process
exec $*
