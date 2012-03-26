
# ROADMAP-STYLE TODO

## Milestone 1

- standalone-worker mode (no cluster support)
- HTB-style scheduling for local workers
- support worker-adapter: shell

## Milestone 2
- cluster support
  - nodes announce on startup
  - designated master replies by tcp connect to announcing node
  - designated master schedules and balances jobs to worker(s)
  - workers frequently ping the master as a health assurance
- support worker-adapter: ruby (rails)

## Milestone 3
- worker process/host resource monitoring
- worker load auto-scaling
- cron scheduling
