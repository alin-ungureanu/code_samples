#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/mutex.h>

/* A good practice to have all of these */
MODULE_DESCRIPTION("A cool little kthread example");
MODULE_AUTHOR("aungureanu@riverbed.com");
MODULE_LICENSE("GPL");

#define COOL_1_THREAD_NAME  "cool_thread_1"

#define COOL_2_THREAD_NAME  "cool_thread_2"

DEFINE_MUTEX(cool_mutex);



static struct task_struct *thread_st1;
static struct task_struct *thread_st2;

static int stop_thread = 0;
static int count = 0;
// Function executed by kernel thread

static int thread_should_stop(void)
{
    return stop_thread;
}

static int thread_fn(void *param)
{
    int i;
    allow_signal(SIGKILL);
    while (!thread_should_stop())
    {

        mutex_lock(&cool_mutex);
        i = count;
        count++;
        mutex_unlock(&cool_mutex);
        pr_info("thread running: %s, count %d\n", (char*)param, i);

        ssleep(5);
        //determining for which thread is the SIGKILL sent
        if (0 == strcmp(param, COOL_1_THREAD_NAME))
        {
            if (signal_pending(thread_st1))
            {
                //assigning NULL to the task_struct, to not wait for the threads with kthread_stop()
                thread_st1 = NULL;
                break;
            }
        }
        else if (signal_pending(thread_st2))
        {
            //assigning NULL to the task_struct, to not wait for the threads with kthread_stop()
            thread_st2 = NULL;
            break;
        }
    }
    pr_info("thread stopping: %s\n", (char*)param);

    do_exit(0);

    return 0;
}
// Module Initialization
static int __init cool_init(void)
{
    pr_info("cool init\n");
    mutex_init(&cool_mutex);
    //Create the kernel thread with name 'cool thread'
    thread_st1 = kthread_create(thread_fn, COOL_1_THREAD_NAME, COOL_1_THREAD_NAME);
    if (thread_st1)
    {
        pr_info("%s created successfully\n", COOL_1_THREAD_NAME);
        //thread is created and needs to be awaken
        wake_up_process(thread_st1);
    }
    else
    {
        pr_err("%s creation failed\n", COOL_1_THREAD_NAME);
    }
    //kthread_run is a macro which contains kthread_create and wake_up_process
    thread_st2 = kthread_run(thread_fn, COOL_2_THREAD_NAME, COOL_2_THREAD_NAME);
    if (thread_st2)
    {
        pr_info("%s created successfully\n", COOL_2_THREAD_NAME);
    }
    else
    {
        pr_err("%s creation failed\n", COOL_1_THREAD_NAME);
    }


    return 0;
}
// Module Exit
static void __exit cool_uninit(void)
{
    pr_info("cool uninitialising\n");
    stop_thread = 1;
    if (thread_st1)
    {
        kthread_stop(thread_st1);
        pr_info("%s stopped\n", COOL_1_THREAD_NAME);
    }
    if (thread_st2)
    {
        kthread_stop(thread_st2);
        pr_info("%s stopped\n", COOL_2_THREAD_NAME);
    }
}


module_init(cool_init);
module_exit(cool_uninit);

