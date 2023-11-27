#include <stdint.h>
#include <stdio.h>
#include <unity.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define MY_STACK_SIZE 3000
#define THING1_PRIORITY 3
#define THING2_PRIORITY 4

char* name1 = "worker1";
char* name2 = "worker2";
char* test;

struct k_thread worker1;
struct k_thread worker2;
struct k_thread supervisor_thread;

K_THREAD_STACK_DEFINE(worker1_stack, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(worker2_stack, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(supervisor_stack, MY_STACK_SIZE);

void thingsToDo(struct k_sem *semi, char* ID){
    k_sem_take(semi, K_FOREVER);
    test = ID;
    printk("%s\n", ID);
    //while(1){}
}

k_tid_t create_thread(struct k_thread *myThread, k_thread_stack_t *stack,
                    k_thread_entry_t entry_point, void *p1, void *p2, void *p3, int priority,
                    uint32_t choices, k_timeout_t delay){
    k_tid_t num = k_thread_create(myThread, stack, MY_STACK_SIZE, entry_point, p1, p2, p3, priority, choices, delay);
    return num;
}

void nuthing(){
    printk("here");
}

void supervisor(){
    //printk("\nsupervisor start\n");
    //create semaphore to be shared
    struct k_sem my_sem;
    k_sem_init(&my_sem, 1, 1);

    // Create worker one and two
    k_tid_t worker1_tid = create_thread(&worker1, worker1_stack, (k_thread_entry_t)thingsToDo, &my_sem, name1, NULL, THING1_PRIORITY, 0, K_MSEC(20));
    k_tid_t worker2_tid = create_thread(&worker2, worker2_stack, (k_thread_entry_t)thingsToDo, &my_sem, name2, NULL, THING2_PRIORITY, 0, K_NO_WAIT);
    k_sleep(K_MSEC(500));

    // Just to get rid of unused variable warning
    printk("Ignore this line %d \n", worker1_tid->errno_var + worker2_tid->errno_var);
}

void test_activity0(){
    //printk("\ntest start\n");
    //Create supervisor
    create_thread(&supervisor_thread, supervisor_stack, (k_thread_entry_t)supervisor, NULL, NULL, NULL, -CONFIG_NUM_COOP_PRIORITIES, 0, K_MSEC(1));

    k_sleep(K_MSEC(1000));

    TEST_ASSERT_EQUAL_STRING(name2, test);
}

void setUp(void) {}

void tearDown(void) {}


int main (void)
{
    UNITY_BEGIN();
    RUN_TEST(test_activity0);
    return UNITY_END();
}