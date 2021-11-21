#include <zephyr.h>
#include <drivers/entropy.h>
#include <stdio.h>
#include <kernel.h>
#include <stdlib.h>
#include <time.h>
#include <device.h>
#include <syscalls/rand32.h>
#include <drivers/entropy.h>

#define MAX_WAIT_TIME	K_MSEC(30)
#define TRUE 1
#define FALSE 0

/*Defining stacksize used by each thread*/
#define STACKSIZE 1024

/*scheduling priority used by each thread*/
#define PRIORITY 5
  
K_MBOX_DEFINE(mailbox);
static void expiry_function(struct k_timer *timer);
static void work_handler(struct k_work *work);
static void stop_work_handler(struct k_work *stop_work);
static void stop_function(struct k_timer *timer);
int entropy_get_entropy(const struct device* dev, uint8_t* buffer, uint16_t length);
   
K_TIMER_DEFINE(timer,expiry_function,stop_function);

K_WORK_DEFINE(work,work_handler);

K_WORK_DEFINE(stop_work,stop_work_handler);

static struct k_mbox_msg recv_msg;
uint32_t random;

static uint32_t buffer;
static uint32_t key[20];
static bool received;
static int i;

void producer_thread(void)
{
    printk("Running producer thread\n");

    const struct device *dev;
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	if (!device_is_ready(dev)) {
		printf("error: no entropy device\n");
		return;
	}

	printf("entropy device is %p, name is %s\n",
	       dev, dev->name);

    struct k_mbox_msg send_msg;
    uint32_t buffer_msg;
        /*generate random value to send*/
        //srand(time(NULL));
        entropy_get_entropy(dev, &buffer_msg, sizeof(buffer_msg));
        
        /* prepare to send message */
        uint32_t buffer_msg_binary = buffer_msg%2;
        printf("The random number generated is %d\n", buffer_msg_binary);
        send_msg.info=10;
        send_msg.size=1;
        send_msg.tx_data = &buffer_msg_binary;
        send_msg.tx_block.data=0;
        send_msg.tx_target_thread= K_ANY;
        /* send message and wait until a consumer receives it */

        k_mbox_put(&mailbox, &send_msg, K_MSEC(30));
        
}

void consumer_thread(void)
{
        printk("Running consumer thread\n");
        /* prepare to receive message */
        //recv_msg.info;
        recv_msg.size=1;
        recv_msg.rx_source_thread = K_ANY;

        /* get a data item, waiting as long as needed */
        k_mbox_get(&mailbox, &recv_msg, &buffer, K_MSEC(30));

        /* info, size, and rx_source_thread fields have been updated */

         if(recv_msg.info==10){
            received=TRUE;
        }
}
    
static void expiry_function(struct k_timer *timer)
{
    printk("Running expiry function\n");
	k_work_submit(&work);
}
static void stop_function(struct k_timer *timer)
{
    printk("Running stop function\n");
	k_work_submit(&stop_work);
}    

	
static void work_handler(struct k_work *work)
{
    printk("Running work handler\n");
	producer_thread();/*do the processing that needs to be done periodically, send the random bit if random wait time is elapsed,	start the session again*/
     key[i] = random;

}

static void stop_work_handler(struct k_work *stop_work)
{
    printk("Running stop work handler\n");
    key[i] = buffer;
}


void ALICE(void)
{
    int wait_time_alice;

    printk("Starting the loop for alice \n");
    for (i=0;i<20;i++){
        printk("\n-----------------------\n");
        printk("\n [ALICE] \n");
        // transmit timer start

        printk("The Timer has started for ALICE\n");
        wait_time_alice=rand()%30;
        printk("The random wait time for ALICE is %d\n",wait_time_alice);
        
        k_timer_start(&timer, K_MSEC(wait_time_alice), K_MSEC(30));

        //scanning for bits 
        printk("ALICE is  Scanning for the bits\n");
        consumer_thread();

        if((received=TRUE)){
            printk("Received by ALICE\n");  
            k_timer_stop(&timer);
            printk("The value received by ALICE is %d\n",key[i]);
            key[i] = 1-key[i];
            printk("The value stored by ALICE is %d\n", key[i]);
        }
       else{
        printk("The value is not received by ALICE, so the transmitted value is %d\n", key[i]);
        }
    }
    printk("The shared secret key for ALICE is\n");
    for (int j = 0; j<20; j++)
    {
        printk("%d ", key[j]);
    }
    printk("\n\n");

}
void BOB(void)
{
    int wait_time;

    printk("Starting the loop for BOB\n");
    for (i=0;i<20;i++){
        printk("\n [BOB] \n");
        // transmit timer start 
        printk("The Timer has started for BOB\n");
        wait_time=rand()%30;
        printk("The random wait time for BOB is %d\n",wait_time);
        
        k_timer_start(&timer, K_MSEC(wait_time), K_MSEC(30));

        //scanning for bits 
        printk("BOB is Scanning for the bits\n");
        consumer_thread();

        if((received=TRUE)){
            printk("Received by BOB\n");  
            k_timer_stop(&timer);
            printk("The value received by BOB is %d\n",key[i]);
        }
       else{
        printk("The value is not received by BOB, so the value transmitted is %d\n", key[i]);
        key[i] = 1-key[i];
        printk("The value stored by BOB is %d", key[i]);
        }
    }
    printk("The Shared secret key for BOB is \n");
    for (int j = 0; j<20; j++)
    {
        printk("%d ", key[j]);
    }
    printk("\n");
    printk("\n-----------------------\n");

}

K_THREAD_DEFINE(A,STACKSIZE,ALICE,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(B,STACKSIZE,BOB,NULL,NULL,NULL,PRIORITY,0,0);
