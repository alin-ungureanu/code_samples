#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

/* A good practice to have all of these */
MODULE_DESCRIPTION("A cool little kthread example");
MODULE_AUTHOR("aungureanu@riverbed.com");
MODULE_LICENSE("GPL");

#define COOL_1_THREAD_NAME  "cool_thread_1"

#define COOL_2_THREAD_NAME  "cool_thread_2"


typedef enum ThreadID
{
    Thread0 = 0,
    Thread1,
    Thread2,
    Thread3,
    Thread4,
    Thread5,
    Thread6,
    Thread7,
    Thread8,
    Thread9,
} ThreadID;

//creates and initializes the lock
DEFINE_SPINLOCK(cool_spinlock);


static struct task_struct *thread_st1;
static struct task_struct *thread_st2;

static int stop_thread = 0;
static int count = 0;

static int thread1 = Thread1;
static int thread2 = Thread2;
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

        spin_lock(&cool_spinlock);
        i = count;
        count++;
        spin_unlock(&cool_spinlock);
        pr_info("kthread '%s' running on %d\n", current->comm, smp_processor_id());

        ssleep(5);
        //determining if is the SIGKILL sent
        if (signal_pending(current))
        {
            //assigning NULL to the task_struct, to not wait for the threads with kthread_stop()
            if (*(int *)param == Thread1)
            {
                thread_st1 = NULL;
            }
            else if (*(int *)param == Thread2)
            {
                thread_st2 = NULL;
            }
            break;
        }
    }
    pr_info("thread stopping: %s\n", current->comm);

    do_exit(0);

    return 0;
}


static int thread_fn_6sec_run(void *param)
{

    int i;
    struct completion *c = (struct completion*)param;
    for (i = 0; i < 3; ++i)
    {
        pr_info("kthread '%s' running on [%d]\n", current->comm, smp_processor_id());

        ssleep(2);
    }    
    pr_info("thread stopping: %s\n", current->comm);

    complete(c);

    do_exit(0);

    return 0;
}



// Module Initialization
static int __init cool_init(void)
{
    int cpu;
    struct completion *completions;

    pr_info("cool init\n");
    //Create the kernel thread with name 'cool thread'
    thread_st1 = kthread_create(thread_fn, &thread1, COOL_1_THREAD_NAME);
    if (thread_st1)
    {
        pr_info("%s created successfully\n", COOL_1_THREAD_NAME);
        //bind thread to CPU 0
        kthread_bind(thread_st1, 0);
        //thread is created and needs to be awaken
        wake_up_process(thread_st1);
    }
    else
    {
        pr_err("%s creation failed\n", COOL_1_THREAD_NAME);
    }
    //kthread_run is a macro which contains kthread_create and wake_up_process
    thread_st2 = kthread_run(thread_fn, &thread2, COOL_2_THREAD_NAME);
    if (thread_st2)
    {
        pr_info("%s created successfully\n", COOL_2_THREAD_NAME);
    }
    else
    {
        pr_err("%s creation failed\n", COOL_1_THREAD_NAME);
    }
    /* We must allocate them because we don't know how many of them are. */
    completions = (struct completion *)kcalloc(num_online_cpus(), sizeof(*completions), GFP_KERNEL);

    for_each_online_cpu(cpu) {
        struct task_struct *thread;
        /* It's not global, so we must initialize it onstack. See completion.h on why. */
        completions[cpu] = COMPLETION_INITIALIZER_ONSTACK(completions[cpu]);
        thread = kthread_create(thread_fn_6sec_run, &completions[cpu], "cool_thread[%d]", cpu);
        if (IS_ERR(thread)) {
                pr_err("Failed to create thread on cpu %d: %ld\n", cpu, (long)thread);
                continue;
        }
            
        kthread_bind(thread, cpu);
        wake_up_process(thread);
    }
    /* Will wait for each thread to finish. If it's already finished, it will just return.
        * Basically, it's like pthread_join. */
    for_each_online_cpu(cpu) {
            wait_for_completion(&completions[cpu]);
    }

    kfree(completions);

    pr_info("cool: init done only after threads\n");

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

