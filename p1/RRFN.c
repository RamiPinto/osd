#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

#include "mythread.h"
#include "interrupt.h"

#include "queue.h"

TCB* scheduler();
void activator();
void timer_interrupt(int sig);
void network_interrupt(int sig);

/* Array of state thread control blocks: the process allows a maximum of N threads */
static TCB t_state[N];

/* Current running thread */
static TCB* running;
static int current = 0;

/* Variable indicating if the library is initialized (init == 1) or not (init == 0) */
static int init=0;

/* Thread control block for the idle thread */
static TCB idle;
static void idle_function(){
	while(1);
}

//Declare high priority and low priority queues
struct queue * hp_q;
struct queue * lp_q;
struct queue * w_q;

/* Initialize the thread library */
void init_mythreadlib() {
	int i;

	//Initialize queues
	hp_q = queue_new();
	lp_q = queue_new();
	w_q = queue_new();

	/* Create context for the idle thread */
	if(getcontext(&idle.run_env) == -1){
		perror("*** ERROR: getcontext in init_thread_lib");
		exit(-1);
	}
	idle.state = IDLE;
	idle.priority = SYSTEM;
	idle.function = idle_function;
	idle.run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
	idle.tid = -1;
	if(idle.run_env.uc_stack.ss_sp == NULL){
		printf("*** ERROR: thread failed to get stack space\n");
		exit(-1);
	}
	idle.run_env.uc_stack.ss_size = STACKSIZE;
	idle.run_env.uc_stack.ss_flags = 0;
	idle.ticks = QUANTUM_TICKS;
	makecontext(&idle.run_env, idle_function, 1);

	t_state[0].state = INIT;
	t_state[0].priority = LOW_PRIORITY;
	t_state[0].ticks = QUANTUM_TICKS;
	if(getcontext(&t_state[0].run_env) == -1){
		perror("*** ERROR: getcontext in init_thread_lib");
		exit(5);
	}

	if(QUANTUM_TICKS < 0){
		printf("*** ERROR: QUANTUM TICKS must not be lower than 1\n");
		exit(-1);
	}

	for(i=1; i<N; i++){
		t_state[i].state = FREE;
	}

	t_state[0].tid = 0;
	running = &t_state[0];

	/* Initialize network and clock interrupts */
	init_network_interrupt();
	init_interrupt();
}


/* Create and intialize a new thread with body fun_addr and one integer argument */
int mythread_create (void (*fun_addr)(),int priority)
{
	int i;

	if (!init) { init_mythreadlib(); init=1;}
	for (i=0; i<N; i++)
		if (t_state[i].state == FREE) break;
	if (i == N) return(-1);
	if(getcontext(&t_state[i].run_env) == -1){
		perror("*** ERROR: getcontext in my_thread_create\n");
		exit(-1);
	}
	t_state[i].state = INIT;
	t_state[i].priority = priority;
	t_state[i].function = fun_addr;
	t_state[i].run_env.uc_stack.ss_sp = (void *)(malloc(STACKSIZE));
	if(t_state[i].run_env.uc_stack.ss_sp == NULL){
		printf("*** ERROR: thread failed to get stack space\n");
		exit(-1);
	}
	t_state[i].tid = i;
	t_state[i].run_env.uc_stack.ss_size = STACKSIZE;
	t_state[i].run_env.uc_stack.ss_flags = 0;
	t_state[i].ticks = QUANTUM_TICKS;

	makecontext(&t_state[i].run_env, fun_addr, 1);

	//Insert process into its corresponding queue
	if(t_state[i].priority == HIGH_PRIORITY){
		enqueue(hp_q, &t_state[i]);
	}
	else{
		enqueue(lp_q, &t_state[i]);
	}

	printf("*** THREAD %d READY\n", t_state[i].tid);

	/* If low priority process is running and a high priority process arrives,
	we stop the low priority process execution to execute the high priority one*/
	if(running->priority == LOW_PRIORITY && t_state[i].priority == HIGH_PRIORITY) {
		disable_interrupt();
		disable_network_interrupt();
		activator(scheduler());
	}

	return i;
} /****** End my_thread_create() ******/

/* Read network syscall */
int read_network()
{
	if (running->state != WAITING) {
		running->state = WAITING;
		enqueue(w_q, running);
		printf("*** THREAD %d READ FROM NETWORK\n", current);
	}

	return 1;
}

/* Network interrupt  */
void network_interrupt(int sig)
{
	if(queue_empty(w_q) == 0){
		TCB* d = dequeue(w_q);

		if (d->priority == HIGH_PRIORITY) {
			enqueue(hp_q, d);
		} else {
			enqueue(lp_q, d);
		}

		printf("*** THREAD %d READY\n", d->tid);

		if(running->priority == LOW_PRIORITY && running->ticks == 0){
			running->ticks = QUANTUM_TICKS;
			disable_interrupt();
			disable_network_interrupt();
			TCB* next = scheduler();
			activator(next);
		}
	}

	//Reset quantum ticks to low priority porcesses
//	if(running->priority == LOW_PRIORITY && running->ticks == 0){
//		running->ticks = QUANTUM_TICKS;
//		disable_interrupt();
//		disable_network_interrupt();
//		TCB* next = scheduler();
//		activator(next);
//	}
}


/* Free terminated thread and exits */
void mythread_exit() {
	int tid = mythread_gettid();

	printf("*** THREAD %d FINISHED\n", tid);
	t_state[tid].state = FREE;
	free(t_state[tid].run_env.uc_stack.ss_sp);

	//If there are still processes in any queue, we select the next process to execute
	if(queue_empty(hp_q) == 0 || queue_empty(lp_q) == 0){
		disable_interrupt();
		disable_network_interrupt();
		TCB* next = scheduler();
		activator(next);
	}

	printf("FINISH\n");
	exit(0);
}

/* Sets the priority of the calling thread */
void mythread_setpriority(int priority) {
	int tid = mythread_gettid();
	t_state[tid].priority = priority;
}

/* Returns the priority of the calling thread */
int mythread_getpriority(int priority) {
	int tid = mythread_gettid();
	return t_state[tid].priority;
}


/* Get the current thread id.  */
int mythread_gettid(){
	if (!init) { init_mythreadlib(); init=1;}
	return current;
}


/* FIFO para alta prioridad, RR para baja*/
TCB* scheduler(){

	//If running process has not ended, we insert it at the end of the corresponding queue
	if(running->state!=FREE){
		if(running->priority == HIGH_PRIORITY){
			enqueue(hp_q, running);
		}
		else{
			enqueue(lp_q, running);
		}
	}

	if(queue_empty(hp_q) == 0){
		return dequeue(hp_q);
	}
	else{
		if(queue_empty(lp_q) == 0){
			return dequeue(lp_q);
		}
		else{
			return &idle;
		}
	}
}

/* Timer interrupt  */
void timer_interrupt(int sig)
{
	//Reduce ticks remaining to finish the process in each clock interrupt
	running->ticks--;

	//Reset quantum ticks to low priority porcesses
	if(running->priority == LOW_PRIORITY && running->ticks == 0){
		running->ticks = QUANTUM_TICKS;
		disable_interrupt();
		TCB* next = scheduler();
		activator(next);
	}
}

/* Activator */
void activator(TCB* next){

	TCB * temp = running;

	//Update process tid
	current = next->tid;
	running = next;

	//Running process finished
	if(temp->state == FREE){
		printf("*** THREAD %d FINISHED: SET CONTEXT OF %d \n", temp->tid, next->tid);
		if(next->priority == LOW_PRIORITY){
			enable_interrupt();
			enable_network_interrupt();
		}
		setcontext(&(next->run_env));
		printf("mythread_free: After setcontext, should never get here!!...\n");
	}
	else{
		//Swap from low priority process to high priority one
		if(running->priority == HIGH_PRIORITY && temp->priority == LOW_PRIORITY){
			printf("*** THREAD %d PREEMPTED: SET CONTEXT OF %d\n", temp->tid, running->tid);

//			TODO: remove after tests
//			if(next->priority == LOW_PRIORITY){
//				enable_interrupt();
//				enable_network_interrupt();
//			}
			swapcontext(&(temp->run_env),&(running->run_env));
		}
			//Standard not finished process
		else{
			if(temp->tid != next->tid){	//Avoid context swaping of same process
				if(temp->state == IDLE){
					printf("*** THREAD READY: SET CONTEXT TO %d\n", next->tid);
				}
				else{
					printf("*** SWAPCONTEXT FROM %d TO %d\n", temp->tid, next->tid);
				}

				if(next->priority == LOW_PRIORITY){
					enable_interrupt();
					enable_network_interrupt();
				}
				swapcontext(&(temp->run_env),&(next->run_env));
			}
		}
	}
}