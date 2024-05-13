# You Spin Me Round Robin

Simulates Round-Robin scheduling of given simple arrival and burst times for hypothetical processes and a quantum time. Outputs the average wait time and average response time of the scheduling situation.

## Building

```shell
make
```

## Running

The first argument supplied to the program should be a txt file containing the process count and information, in the following format:
```shell
[# of processes]
[process 1], [arrival time], [burst time]
[process 2], [arrival time], [burst time]
...
```
The second argument should be an integer, and will be the quantum time for the simulated scheduler.
```shell
./rr processes.txt 3
```

The results will be the average wait time and response time for the processes.
```shell
Average wait time: x.x
Average response time: x.x
```

## Cleaning up

```shell
make clean
```
