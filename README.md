# BatServer

Telnet Usage:
info:  display info of tasks, processes and workers.
task:  display info of tasks, include task id, task processes, owner, name, and recently finished tasks will be displayed.
process:  display the number of processes in queue, and the first process info.
computer:  display the info of all workers, include ip, cores, and unregistered computers.
acctask +id:  put the task %%id processes in front of the queue.
killtask +id:  remove the task %%id processes.
settask +id +num:  set the task %%id use %%num cores in the left processes.
removecomputer +ip:  remove the computer %%ip, stop all the tasks in the computer.
setcomputer +ip +num: set the computer %%ip using %%num cores, processes under processing may be restarted.
lazysetcomputer +ip +num: set the computer %%ip using %%num cores, and it will not effect the process under processing.