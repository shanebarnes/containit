# containit

## Synopsis

containit is a process container that waits for one or more child processes to
complete. Each child process is not expected to exit unless a fatal error
condition is encountered. Termination of a single child process will cause
containit to kill all remaining child processes and stop or restart immediately.

## Example

./containit stop-on-term "cat" "sleep 3"

./containit restart-on-term "cat" "sleep 3" "date"

## Motivation

containit was designed to be run as a single Docker container process
(i.e., init process) that executes one or more child processes.
