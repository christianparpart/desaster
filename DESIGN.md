
# Example Queue Diagram

    ---------------------------------------------------------------------------------------------
    |                                        all cluster nodes                                  |
    ---------------------------------------------------------------------------------------------
    ---------------------- -------------------------- -------------------------------------------
    | shell worker group | |    ruby worker group   | |  HTTP worker group                      |
    ---------------------- -------------------------- -------------------------------------------
            |              ---------- --------------- ------------- ----------- -----------------
            |              | purges | |   general   | | checkouts | | uploads | |    general    |
            |              ---------- --------------- ------------- ----------- -----------------
            |                  |             |                |             |              |
          fifo                set          fifo             fifo          fifo           fifo

In this cluster we're having a worker group for shell scripts, ruby calls, and
a dedicated HTTP worker group.

The ruby workers handle purges seperately to the other _general_ traffic in order to ensure
that purges are always going with with at least N% throughput.

HTTP requests are devided into 3 groups aswell, a general and 2 more important ones.

The "purge" ruby queue is queuing no duplicated jobs for at least N seconds (configurable)
while all other queues are first-in-first-out.

## Receiving jobs

- shell queue: cronjob'ed jobs or via native network protocol
- ruby worker queues: via the official ruby gem to pass jobs through the network protocol
- HTTP worker queues: via HTTP protocol on a distinct port (e.g. 8080) received via x0's XzeroHttp API

## possible HTTP request flow

    CLIENT -> nginx (ssl termination/gzip) [-> varnish] -> Desaster Scheduler -> Desaster Worker (with Rails loaded)
                                     \_____________________/

