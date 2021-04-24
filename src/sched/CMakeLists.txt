add_library(sched OBJECT tss.c tss.s kthread.c switch.s klock.c kthread.s klock.s timer.c user.c sleeplock.c schedule.c)
