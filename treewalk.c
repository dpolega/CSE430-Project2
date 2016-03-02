/*   -- Start of treewalk.c --   */

#include <linux/module.h>		/* Needed by all modules */
#include <linux/kernel.h>		/* Needed for KERN_INFO */
#include <linux/init.h>			/* Needed for the macros */
#include <linux/delay.h>	// allows use of msleep()
#include <linux/kthread.h>	// attempt to avoid implicit macro errors
#include <linux/sched.h>	// allows access to task_struct
#include <linux/list.h>		// used for struct list_head
#include <linux/kfifo.h>	// added for kfifo

MODULE_LICENSE("GPL");

//DEFINE_KFIFO(fifo, int, PAGE_SIZE);

// declare global variable to allow start and stop kthread
struct task_struct *my_task;

// declare int for count of children in a task
int count = 0;

int tree_walk(struct task_struct *p) {

	// declare unsigned int for enqueuing pids
	//unsigned int pid2q;

	// declare list for iterating thru children
	struct list_head *pos;

	// iterate through each child and traverse their children
	list_for_each(pos, &p->children) {
		tree_walk(list_entry(pos, struct task_struct, sibling));
	} 

	/*----Debugging Statement----*/
	printk(KERN_INFO "Queuing: %i\n", (int) p->pid);
	
	//check that current task is not shell process
	//if ( (int) p->parent->pid > 1) {
	//	pid2q = (int) p->pid;
	//	kfifo_in(&fifo, &pid2q, sizeof(pid2q));
	//}

	count++;

	return 0;
}

int my_kthread_function(void *data) {
	
	//int threshold = 10;
	struct task_struct *current_task;	

	/*----Debugging Statement----*/
	printk(KERN_INFO "Entering kthread function\n");

	// repeat my_kthread_function until stop signal is detected
	while(!kthread_should_stop()) {
		
		for_each_process(current_task) {

			// not to be used on consumer producer code
			//kfifo_reset(&fifo);

			// check if grandparent is init // && (int) current_task->parent->parent->pid == 1
			if ((int) current_task->pid > 100 ) {
				count = 0;

				// begin recursive traveral of current task
				tree_walk(current_task);

				/*----Debugging Statement----*/
				if (count > 1)
					printk(KERN_INFO "PID: %i #Children: %i\n", (int) current_task->pid, count-1);
	
			}	
		}	
		msleep(10000);
	}
	return 0;
}

static int __init my_module_init(void) {
	
	int data = 20;

	printk(KERN_INFO "----Module loaded----\n");

	// create new thread
	my_task = kthread_run(&my_kthread_function,(void *) &data,"my_module");
	return 0;
}
