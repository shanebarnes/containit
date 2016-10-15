# containit

## Synopsis

containit is a process container that waits for one or more child processes to
complete. Each child process is expected to last forever unless a fatal error
condition is encountered. Termination of a single child process will cause
containit to kill all remaining child processes and exit immediately.

## Example

./containit "cat" "sleep 3"

## Motivation

containit was designed to be run as a single Docker container process
(i.e., init process) that executes one or more child processes.
