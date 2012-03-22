
# Desaster Job Queue Manager

_desaster_ is a job queue manager and primarily inspired by _resque_ and it's web frontend _resque-web_.

## Requirements

- queue management
  - queues should be managed via the UI
  - dynamically attach/detach queues from/to workers
  - true and pluggable queueing system (SFQ, HTB, ...)
  - optional: automatically unique / set-based queues
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

# desaster-web

_desaster-web_ is the dedicated daemon, possibly written in Ruby/Sinatra,
to provide access to the backend scheduler.

# desasterd

_desasterd_ is the dedicated daemon, written in C++11, to run on every
worker node.

This daemon may only be executed once per host. It can be started
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

## Worker Adapters

A worker can perform different tasks, such as, performing a system command
or executing a method within a Rails environment.

The latter should be pre-spawned and reused for maximum performance.

## Implementation

Single threaded scheduling system using _libev_ as event machine
(timers are implemented via `ev::timer`).

# Client Bindings

We at least provide bindings for Ruby, Ruby on Rails 3 (possibly 2.3.x too) and C.
