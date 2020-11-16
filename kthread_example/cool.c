#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>

#define COOL_1_THREAD_NAME  "cool_thread_1"

#define COOL_2_THREAD_NAME  "cool_thread_2"

/* A good practice to have all of these */
MODULE_DESCRIPTION("A cool little kthread example");
MODULE_AUTHOR("csirb@riverbed.com");
MODULE_LICENSE("GPL");

static struct task_struct *thread_st1;
static struct task_struct *thread_st2;

static int stop_thread = 0;
// Function executed by kernel thread

static int thread_should_stop(void)
{
    return stop_thread;
}

static int thread_fn(void *param)
{
    //allow_signal(SIGKILL); - not working
    while (!thread_should_stop())
    {
        pr_info("thread running: %s\n", (char*)param);
        ssleep(5);
        /*if (0 == strcmp(param, COOL_1_THREAD_NAME))
        {
            if (signal_pending(thread_st1)) - not working
            {
                break;
            }
        }
        else if (signal_pending(thread_st2))
        {
            break;
        }*/
    }
    pr_info("thread stopping: %s\n", (char*)param);

    do_exit(0);
    return 0;
}
// Module Initialization
static int __init cool_init(void)
{
    pr_info("cool init\n");
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

