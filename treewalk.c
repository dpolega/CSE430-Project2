/*   -- Start of treewalk.c --   */

#include <linux/module.h>		/* Needed by all modules */
#include <linux/kernel.h>		/* Needed for KERN_INFO */
#include <linux/init.h>			/* Needed for the macros */
#include <linux/delay.h>	// allows use of msleep()
#include <linux/kthread.h>	// attempt to avoid implicit macro errors
#include <linux/sched.h>	// allows access to task_struct
#include <linux/list.h>		// used for struct list_head
#include <linux/kfifo.h>	// added for kfifo
#include <linux/slab.h>		// added for kmalloc

MODULE_LICENSE("GPL");

DEFINE_KFIFO(fifo, int, PAGE_SIZE);

// declare global variable to allow start and stop kthread
struct task_struct *my_task;

// declare int for count of elements in a tree
int tcount = 0;

int tree_queue(struct task_struct *p) {

	// declare unsigned int for enqueuing pids
	unsigned int pid2q;

	// declare list for iterating thru children
	struct list_head *pos;

	// iterate through each child and traverse their children
	list_for_each(pos, &p->children) {
		tree_queue(list_entry(pos, struct task_struct, sibling));
	} 

	// print PID of process being queued to kernel log
	printk(KERN_INFO "%i\n", (int) p->pid);
	
	pid2q = (int) p->pid;
	kfifo_in(&fifo, &pid2q, sizeof(pid2q));

	return 0;
}

int tree_count(struct task_struct *p) {

	// declare list for iterating thru children
	struct list_head *pos;

	// iterate through each child and traverse their children
	list_for_each(pos, &p->children) {
		tree_count(list_entry(pos, struct task_struct, sibling));
	} 

	tcount++;

	return 0;
}

int my_kthread_function(void *data) {
	
	int threshold = 80;
	int fifo_val = 0;
	struct task_struct *current_task;	

	// allocate string to store task_struct comm value
	char * cname = (char *) kmalloc(64, GFP_KERNEL);

	/*----Debugging Statement----*/
	printk(KERN_INFO "Entering kthread function\n");

	// repeat my_kthread_function until stop signal is detected
	while(!kthread_should_stop()) {
		
		for_each_process(current_task) {
			tcount = 0;
			sprintf(cname, "%s", current_task->parent->comm);

			// traverse if task has children and the task's parent is bash
			if (!list_empty(&current_task->children) 
				&& (int) current_task->pid > 100 
				&& cname[0] == 'b' && cname[1] == 'a'
				&& cname[2] == 's' && cname[3] == 'h') {

				/*----Debugging Statement----*/
				printk(KERN_INFO "Possible fork bomb: %i\n", current_task->pid); 

				// begin tree count of current task
				tree_count(current_task);

				if (tcount > threshold) {
					
					// begin queuing current task tree
					tree_queue(current_task);

					/*----Debugging Statement----*/
					kfifo_peek(&fifo, &fifo_val);
					printk(KERN_INFO "First fork node queued: %i Length: %i\n", fifo_val,  tcount);
					printk(KERN_INFO "Parent PID: %i\n", (int) current_task->pid);

					// break from for_each_process
					break;
				}
				else {

					/*----Debugging Statement----*/
					printk(KERN_INFO "False alarm");

					// reset fifo if threshold is not met
					kfifo_reset(&fifo);
				}
			}	
		} // end for_each_process

		msleep(10000);
	}

	// free cname allocated from kmalloc
	kfree(cname);

	return 0;
} // end my_kthread_function

static int __init treewalk_init(void) {
	
	int data = 20;

	printk(KERN_INFO "----Module loaded----\n");

	// create new thread
	my_task = kthread_run(&my_kthread_function,(void *) &data,"my_module");
	return 0;
}

static void __exit treewalk_exit(void) {
	kthread_stop(my_task);
	printk(KERN_INFO "----Module removed----\n");
}

module_init(treewalk_init);
module_exit(treewalk_exit);

/*   -- End of my_module.c --   */
