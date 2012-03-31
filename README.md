
# Desaster Job Queue Manager

_desaster_ is a job queue manager and primarily inspired by _resque_ and it's web frontend _resque-web_.

# Command-line Syntax

    desaster, Job Queueing Manager, version 0.1.0 [http://github.com/trapni/desaster/]
    Copyright (c) 2012 by Christian Parpart <trapni@gentoo.org>
    Licensed under GPL-3 [http://gplv3.fsf.org/]

     usage: desaster [-a BIND_ADDR] [-b BROADCAST_ADDR] [-p PORT] [-k GROUP_KEY]

     -?, -h, --help                 prints this help
     -a, --bind-address=IPADDR      local IP address to bind to [0.0.0.0]
     -b, --broadcast-address=IPADDR remote IP/multicast/broadcast address to announce to [255.255.255.255]
     -p, --port=NUMBER              port number for receiving/sending packets [2691]
     -k, --key=GROUP_KEY            cluster-group shared key [default]
     -s, --standalone               do not broadcast for peering with cluster

To rn _desaster_ in standard cluster-mode, just run without any arguments: `desaster`.
This will auto-peer with other _desaster_-nodes within the same subnet.

To run _desaster_ in standalone-mode, just run `desaster -s`.

# Software Implementation

## Dependencies

- build-time dependencies:
  - C++11 compiler, such as GCC 4.6 (or higher, not below)
  - cmake - http://cmake.org (our build system of choice)
  - pkg-config - http://www.freedesktop.org/wiki/Software/pkg-config (helper, used by the build-system)
- build- and runtime dependencies:
  - libev - http://libev.schmorp.de/ (for I/O event dispatching)
  - x0 - http://xzero.io/ (its base library, later also its HTTP library for HTTP dispatching)
  - hiredis
- runtime dependencies:
  - redis - http://redis.io/ (you *possibly* will ned Redis to run Desaster for shell/ruby jobs, not decided yet).

## UI Requirements

- queue management
  - queues should be managed via the UI
  - dynamically attach/detach queues from/to workers
  - true and pluggable queueing system (SFQ, HTB, ...)
  - timed job-requeuing blacklist (rejecting every job identity that
    has been enqueued/processed just recently)
  - list queue entries
    - searchable, sortable, filterable, paginated
    - perform actions on selected jobs
- worker management:
  - assign jobs to different workers/queues as needed
- job management:
  - ETA property (r/o) for queued jobs
  - kill/relocate/re-role jobs
- dashboard:
  - live update feature (button on-top, or even by default)
- failed queue: 
  - searchable, sortable, filterable, paginated, groupable (by originated queues e.g.)
  - automatic retry by default with exponential backoff

## Backend Requirements

- *easily scalable* by just adding new nodes that will auto-negotiate with the existing cluster.
- *maximum fault tolerance* of the designated scheduler master. any worker node can
  take over the scheduling for the cluster if the current master becomes unreachable.
- *on-demand spawning* from within the Rails environment but also be possible to
  start it dedicated, e.g. via _systemd_ or _SysV init script_.
- *binary process upgrades*
- *zero configuration*, at least the (1..n) cluster should be able to run without configuration.
- *worker* CPU/memory resource monitoring


# desaster-web

_desaster-web_ is the dedicated daemon, possibly written in Ruby/Sinatra,
to provide access to the backend scheduler and all worker nodes.

# desaster

_desaster_ is the dedicated daemon, written in C++11, to run on every
worker node.

This daemon shall be only executed once per host. It can be started
as a system service or in some rare cases on demand by (e.g.) your
rails application that needs this functionality.

This service will:

- if this node is the designated scheduler master:
  - it will schedule all tasks and pass them to all workers.
  - it will propagate any scheduler state changes to all other nodes in the cluster.
- if this node is NOT the designated scheduler master:
  - it will just receive scheduler state changes by the designated master scheduler.
  - forward incoming tasks to the designated master
- receive incoming tasks by the designated scheduler master perform them locally.
- start multiple auto-scaling child worker processes as a child process.
  - guards over resource usage of their worker processes (CPU and memory usage).
- should log worker resource usage and load distribution to allow the web frontend
  to generate nice looking graphs.

## Implementation

Single threaded scheduling system using _libev_ as event machine
(timers are implemented via `ev::timer`).

# Modules

## Shell

Executes shell commands, forking on demand.

## Cron

Executes shell commands on any shell worker node at a given time/period, just like UNIX cronjobs
but enqueued and scheduled within the _Desaster_ cluster.

This module rarely depends on the `shell`-module as it just schedules tasks to be performed
by the `shell` module.

At a later stage, we might also want to schedule ruby method invokations, which should be kept in mind
when implementing this module.

## Ruby

executes ruby methods, pre-forking and communicating over shared file descriptors
(pipes / unnamed sockets) to pass jobs and their response status.

This module should effectively be able to spawn Rack, and thus Rails, applications.

## HTTP

For _Desaster_, an HTTP request is nothing else than a Job to be executed, so this framework just
fits the best when it comes to fair load balancing your incoming HTTP requests over a large cluster.

This worker implements receiving HTTP requests, possibly terminating SSL, possibly passing
it to the designated master scheduler, to be then scheduled and passed to the actual backend
worker.

Speaking of our particular use-case, on a backend node, we might even optimize the communication path
a little further to combine it with the ruby glue code to actually handle our Rack/Passenger Rails requests.

# API Bindings

You communicate via TCP/IP to the Desaster cluster, however, we at least provide
an access API for Ruby (1.8 and 1.9 compatible) and C++.
