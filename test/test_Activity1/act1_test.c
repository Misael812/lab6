#include <stdint.h>
#include <stdio.h>
#include <unity.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "act1.h"

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
    k_tid_t sup = create_thread(&supervisor_thread, supervisor_stack, (k_thread_entry_t)supervisorTerminator, &worker1, &worker2, NULL, -CONFIG_NUM_COOP_PRIORITIES, 0, K_MSEC(5000));

    // Create worker 1 and worker 2 threads
    k_tid_t num1 = create_thread(&worker1, worker1_stack, entryPoint1, NULL, NULL, NULL, worker1Priority, 0, worker1Delay);
    k_tid_t num2 = create_thread(&worker2, worker2_stack, entryPoint2, NULL, NULL, NULL, worker2Prioriyt, 0, worker2Delay);
    printk("\n Made it here\n");
    // Block until supervisor suspends workers or until timeout
    k_thread_join(&supervisor_thread, K_MSEC(5500));
    
    // Get stats for execution time for worker threads and end time
    k_thread_runtime_stats_get(&worker1, &worker1Stats);
    k_thread_runtime_stats_get(&worker2, &worker2Stats);
    k_thread_runtime_stats_all_get(&endStats);

    // Use the number of cycles to approximate the time it took to execute (converted to milliseconds)
    startTime = timing_cycles_to_ns(startStats.execution_cycles) / 1000;
    worker1Time = timing_cycles_to_ns(worker1Stats.execution_cycles) / 1000;
    worker2Time = timing_cycles_to_ns(worker2Stats.execution_cycles) / 1000;
    endTime = timing_cycles_to_ns(endStats.execution_cycles) / 1000;
    elapsedTime = endTime - startTime;

    // Return time it took to execute for the worker and total execution
    *worker1Duaration =  worker1Time;
    *worker2Duration =  worker2Time;
    *totalDuartion = elapsedTime;

    k_thread_abort(&worker1);
    k_thread_abort(&worker2);
    
    // To get rid of un used variable warning
    printk("Ignore this line %d \n", sup->errno_var + num1->errno_var + num2->errno_var);
}

void setUp(void) {}

void tearDown(void) {}

//////////////////// Threads with Same Priority ///////////////////

// Coop Section
void test_coop_same_priority_busy_busy(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_COOP(4), K_PRIO_COOP(4),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_EQUAL_UINT64(0, workerTwoTime);
}

void test_coop_same_priority_busy_yeild(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_COOP(4), K_PRIO_COOP(4),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 2500000, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(100000, 2500000, workerTwoTime);
}

// Preempt Section
void test_preempt_same_priority_busy_busy(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_PREEMPT(4), K_PRIO_PREEMPT(4),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo (ask about this one, why worker two does not run even though its preempt)
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_EQUAL_UINT64(0, workerTwoTime);
}

void test_preempt_same_priority_busy_yeild(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_PREEMPT(4), K_PRIO_PREEMPT(4),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
}

//////////////////// Threads with Different Priority //////////////

// Coop Section
void test_coop_different_priority_busy_busy_A(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime; //should i delay the higher priority more or less
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_COOP(3), K_PRIO_COOP(4),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_EQUAL_UINT64(0, workerTwoTime);
}

void test_coop_different_priority_busy_busy_B(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_COOP(4), K_PRIO_COOP(3),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    // Even though worker two has a higher priority, Worker one gets there first and will 
    // keep control of the main execution
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_EQUAL_UINT64(0, workerTwoTime);
}

void test_coop_different_priority_busy_yield_A(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_COOP(3), K_PRIO_COOP(4),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerTwoTime);
}

void test_coop_different_priority_busy_yield_B(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_COOP(4), K_PRIO_COOP(3),
                                    K_MSEC(10), K_MSEC(11), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerTwoTime);
}

// Preempt Section
void test_preempt_different_priority_busy_busy_A(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_PREEMPT(3), K_PRIO_PREEMPT(4),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerTwoTime);
}

void test_preempt_different_priority_busy_busy_B(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_busy, (k_thread_entry_t)busy_busy, K_PRIO_PREEMPT(4), K_PRIO_PREEMPT(3),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerTwoTime);
}

void test_preempt_different_priority_busy_yield_A(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_PREEMPT(3), K_PRIO_PREEMPT(4),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerTwoTime);
}

void test_preempt_different_priority_busy_yield_B(){
    uint64_t workerOneTime, workerTwoTime, elapsedTime;
    supervisor_execution_analyzer((k_thread_entry_t)busy_yield, (k_thread_entry_t)busy_yield, K_PRIO_PREEMPT(4), K_PRIO_PREEMPT(3),
                                    K_MSEC(10), K_MSEC(10), &workerOneTime, &workerTwoTime, &elapsedTime);
    //Todo
    TEST_ASSERT_UINT64_WITHIN(5000, 0, workerOneTime);
    TEST_ASSERT_UINT64_WITHIN(100000, 5000000, workerTwoTime);
}

int main (void)
{
    UNITY_BEGIN();
    RUN_TEST(test_coop_same_priority_busy_busy);
    RUN_TEST(test_coop_same_priority_busy_yeild);
    RUN_TEST(test_preempt_same_priority_busy_busy);
    RUN_TEST(test_coop_different_priority_busy_busy_A);
    RUN_TEST(test_coop_different_priority_busy_busy_B);
    RUN_TEST(test_coop_different_priority_busy_yield_A);
    RUN_TEST(test_coop_different_priority_busy_yield_B);
    RUN_TEST(test_preempt_different_priority_busy_busy_A);
    RUN_TEST(test_preempt_different_priority_busy_busy_B);
    RUN_TEST(test_preempt_different_priority_busy_yield_A);
    RUN_TEST(test_preempt_different_priority_busy_yield_B);
    return UNITY_END();
}