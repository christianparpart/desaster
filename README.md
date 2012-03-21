
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
  - searchable, sortable, filterable, paginated, groupable (by originated queues e.g.^hh)
  - automatic retry by default with exponential backoff

# desaster-web

_desaster-web_ is the dedicated daemon, possibly written in Ruby/Sinatra,
to provide access to the backend scheduler.

# desaster-worker

Every worker host has this piece tiny management daemon installed to
actually receive jobs and perform them.

We have adapters to handle different kind of jobs, such as

- native command executions
- Rails environments (w/o respawning rails on every new incoming job)
- possibly more.

The worker should be able to announce his capabilities to the scheduler when
it requests it, e.g. what adapters this worker supports / has enabled (not every
worker has a rails environment).

Each _desaster-worker_ daemon also spawns a local _desaster-scheduler_
and announces itself to it.

If you have a network with lots of worker hosts, and thus, lots of workers,
which each will spawn their own local scheduler.
The schedulers now negotiate each other to denote the master scheduler.
Any other scheduler instance may take over as soon as the current master
exits (or dies).

# desaster-scheduler

_desaster-scheduler_ is the dedicated daemon, written in C++11, to
actually schedule all incoming (and fixed-scheduled) jobs according to the
desired configuration preferences.

As each worker spawns its own scheduler automatically, the designated master
scheduler will always *execute* any jobs locally and replicate its own
database and database-updates to all its slave nodes.

All incoming jobs to any slave node will be forwarded to the designated master.
So a worker can connect to any scheduler node (e.g. its local one).

This way we ensure maximum fault tolerance as well as almost zero-configuration
in single-server setups, thus, very easy to install.

## Implementation

Single threaded scheduling system using _libev_ as event machine
(timers are implemented via `ev::timer`).

