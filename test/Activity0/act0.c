#include <stdint.h>
#include <stdio.h>
#include <unity.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define MY_STACK_SIZE 500
#define THING1_PRIORITY 4
#define THING2_PRIORITY 5

struct k_thread worker1;
struct k_thread worker2;

K_THREAD_STACK_DEFINE(worker1_stack, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(worker2_stack, MY_STACK_SIZE);

void thingsToDo(struct k_sem *semi, char* ID){
    k_sem_take(semi, K_FOREVER);
    while(1){
                printk("hello world from %s!\n", ID);
    }
}


void supervisor(){

    //create semaphore to be shared
    struct k_sem my_sem;
    k_sem_init(&my_sem, 0 ,1);

    // Create worker one and two
    k_tid_t worker1_tid = create_thread(&worker1, worker1_stack, (k_thread_entry_t)thingsToDo, &my_sem, "worker1", NULL, THING1_PRIORITY, 0, K_MSEC(20));
    k_tid_t worker2_tid = create_thread(&worker2, worker2_stack, (k_thread_entry_t)thingsToDo, &my_sem, "worker2", NULL, THING2_PRIORITY, 0, K_NO_WAIT);
}

k_tid_t create_thread(struct k_thread *myThread, k_thread_stack_t *stack,
                    k_thread_entry_t entry_point, void *p1, void *p2, void *p3, int priority,
                    uint32_t choices, k_timeout_t delay){
    return k_thread_create(myThread, stack, K_THREAD_STACK_SIZEOF(stack), entry_point, p1, p2, p3, priority, choices, delay);
}

void setUp(void) {}

void tearDown(void) {}


int main (void)
{
    UNITY_BEGIN();
    supervisor();
    return UNITY_END();
}