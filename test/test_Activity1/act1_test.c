#include <stdint.h>
#include <stdio.h>
#include <unity.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define MY_STACK_SIZE 3000

char* name1 = "worker1";
char* name2 = "worker2";
char* test;

struct k_thread worker1;
struct k_thread worker2;
struct k_thread supervisor_thread;

K_THREAD_STACK_DEFINE(worker1_stack, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(worker2_stack, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(supervisor_stack, MY_STACK_SIZE);


k_tid_t create_thread(struct k_thread *myThread, k_thread_stack_t *stack,
                    k_thread_entry_t entry_point, void *p1, void *p2, void *p3, int priority,
                    uint32_t choices, k_timeout_t delay){
    k_tid_t num = k_thread_create(myThread, stack, MY_STACK_SIZE, entry_point, p1, p2, p3, priority, choices, delay);
    return num;
}

void supervisorTerminator(struct k_thread *first, struct k_thread *second){
    // Suspends workers 1 and 2
    k_thread_suspend(first);
    k_thread_suspend(second);
}

void supervisor_execution_analyzer(k_thread_entry_t entryPoint1, k_thread_entry_t entryPoint2, int worker1Priority, int worker2Prioriyt,
                k_timeout_t worker1Delay, k_timeout_t worker2Delay, uint64_t *worker1Duaration, uint64_t *worker2Duration, uint64_t *totalDuartion){

    uint64_t startTime, worker1Time, worker2Time, endTime, elapsedTime;
    k_thread_runtime_stats_t worker1Stats, worker2Stats, startStats, endStats;

    // Create reference point for execution time
    k_thread_runtime_stats_all_get(&startStats);

    // Create a thread whos purpose is to stop worker 1 and worker 2 given totalDuration given by user
    k_tid_t sup = create_thread(supervisor_thread, supervisor_stack, (k_thread_entry_t)supervisorTerminator, NULL, NULL, NULL, -CONFIG_NUM_COOP_PRIORITIES, 0, K_MSEC(totalDuartion));

    // Create worker 1 and worker 2 threads
    k_tid_t num1 = create_thread(worker1, worker1_stack, entryPoint1, NULL, NULL, NULL, worker1Priority, 0, worker1Delay);
    k_tid_t num2 = create_thread(worker2, worker2_stack, entryPoint2, NULL, NULL, NULL, worker2Prioriyt, 0, worker2Delay);

    // Block until supervisor suspends workers or until timeout
    k_thread_join(&supervisor_thread, K_MSEC(5500));
}

void setUp(void) {}

void tearDown(void) {}


int main (void)
{
    UNITY_BEGIN();
    return UNITY_END();
}