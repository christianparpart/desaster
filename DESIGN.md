
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

# Job Flow

- job description is passed to scheduler (MODULE CMD [ARGS ...])
- `scheduler->enqueue(module(moduleName)->createJob(cmd, args));`
  - createJob: `new ShellJob(this, cmdline, args)`
  - enqueue: `queue(job->queueName())->enqueue(job);`
- scheduler: `job = dequeue();`
- scheduler: `nextWorker().pass(job);`

- worker-node:
  - `while (true) getJob()->perform();`


## Job created

- enqueue job J to target queue Q
- trigger check for new jobs

## Job completed

- we've been notified by a worker node (e.g. self) that a job (J) of queue Q completed.

## Job scheduling

the job scheduler is iterating flat over all queues (ordered by ascending priority)
and looks for the next queue having something it can dequeue.
if something found, it'll safe this dequeue-offset and start searching from next
to this position when the next worker slot becomes available.

Job -> Queue (qdisc) -> Server

Server manages Queue
  ~Queue() { server().unlink(this); }
Queue manages Job
  ~Job() { queue().unlink(this); }

## Network Commands

### Syntax

    queue create $NAME
    queue rate $NAME $VALUE
    queue ceil $NAME $VALUE
    queue rename $NAME $NEW_NAME
    queue push $NAME $MODULE $JOB
    queue destroy $NAME $TAKE_OVER_QUEUE
    queue list

### Examples

    queue push test-queue shell /usr/bin/find / -name '*e*'
    +OK 1234

    queue list
    +MULTI 3
    test-queue
    other-queue
    special-queue

    queue contents test-queue
    +MULTI 2
    1234 shell /usr/bin/find / -name '*e*'
    1235 shell /usr/local/bin/backup.sh --mysql

    queue destroy other-queue
    +OK








