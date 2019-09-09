#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>

/*
This is my producer-consumer project for CSC415 project #5.
It's messy right now but seems to work just fine on Ubuntu. 
I ran it on my mac and had a bad time. When you run the program
you have the option of seeing the contents of the bounded buffer queue
by uncommenting the function call print_buffer() in the bottom of the
producer and consumer functions. (line 219 & 256)
 */




/********* MUTEX LOCK / THREAD SYNC *******/
pthread_mutex_t lock;

/* COLORED TEXT */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[92m" // orig. 32
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[96m" // orig. 36
#define ANSI_COLOR_RESET   "\x1b[0m"
/*------------------*/

/*************** STRUCTS *************/
struct 
data
{
    int N;
    int P;
    int C;
    int X;
    int consume_amt;
    int over_consume_on;
    int over_consume_amt;
    int PTime;
    int CTime;
    int * buffer;
    int counter;
    int total_threads;
    int item_id;
    int item_quantity;

} data;

struct 
thread_info {
    int id;
    int type;
    pthread_t tid;
};

struct 
test_counter
{
    int p_counter;
    int c_counter;
} test_counter;


struct
test_arrays {
    int item_produced;
    int item_consumed;
} * test;


/*************** TIMESTAMP **************/

time_t 
timestamp()
{
    time_t curr_time = time(NULL);
    printf("\nCurrent time: %s \n",asctime( localtime(&curr_time) ) );
    return curr_time;
}

/*************** PRINT HELPERS *****************/

/* These methods just print information to the console, 
Some methods will be used for the actual program output,
some are just used for debugging and will not be called 
in the final implimentation */

void 
print_run_info()
{
    printf("                        Number of Buffers : %d \n", data.N);
    printf("                      Number of Producers : %d \n", data.P);
    printf("                      Number of Consumers : %d \n", data.C);
    printf("Number of items Produced by each producer : %d \n", data.X);
    printf("Number of items consumed by each consumer : %d \n", data.consume_amt);
    printf("                         Over consume on? : %d \n", data.over_consume_on);
    printf("                      Over consume amount : %d \n", data.over_consume_amt);
    printf("      Time each Producer Sleeps (seconds) : %d \n", data.PTime);
    printf("      Time each Consumer Sleeps (seconds) : %d \n\n", data.CTime);
}

void 
print_buffer()
{
    for(int i = 0; i < data.N; i++) {
        printf(ANSI_COLOR_YELLOW "[%d] ", data.buffer[i]);
    }
    printf("  counter : %d \n" ANSI_COLOR_RESET, data.counter);
}

/********* OVER CONSUMPTION CHECK *********/
void 
over_consume_check()
{
    data.over_consume_amt = (data.P * data.X) % data.C;
    data.consume_amt = data.P * data.X / data.C;
    if(data.over_consume_amt > 0) {
        data.over_consume_on = 1;
    }
}

/*********** QUEUE FUNCTIONS ***************/

/* 
 * Function to remove item.
 * Item removed is returned
 */
int 
dequeue_item()
{
    if(data.buffer[0] == 0) {
        return -1;
    }
    int removed_item = data.buffer[0];
    for(int i = 1; i <= data.N; i++)
    {
        data.buffer[i-1] = data.buffer[i];
    }
    data.buffer[data.N-1] = 0;
    data.counter--;
    return removed_item;
}

/* 
 * Function to add item.
 * Item added is returned.
 * It is up to you to determine
 * how to use the return value.
 * If you decide to not use it, then ignore
 * the return value, do not change the
 * return type to void. 
 */
int 
enqueue_item(int item)
{
    int i = 0;
    while(data.buffer[i] !=  0) {
        i++;
    }
    if(i >= data.N) {
        return -1;
    } else {
        data.buffer[i] = item;
        data.counter++; 
        return item;
    }
}

/************** CONSUMER THREAD ************/

void *
consumer_thread(void * arg)
{
    /* Get thread ID */
    int id = *((int*) arg);

    int consume = data.consume_amt;
    if(data.over_consume_on) {
        if(data.over_consume_amt / data.C > 1) {
            consume = consume + (data.over_consume_amt / data.C);
            if(data.over_consume_amt % data.C > 0) {
                if(id == 1) {
                    consume++;
                }
            }
        } else {
            if(id <= data.over_consume_amt) {
                consume++;
            }
        }
    }

    for(int i = 0; i < consume; i++) {
        
        /* wait for next item */
        while (data.counter == 0) {
            ; /* do nothing */
        }
        /* CRITICAL SECTION BEGIN */
        pthread_mutex_lock(&lock);
        int removed_item = dequeue_item();
        if(removed_item == -1) {
            pthread_mutex_unlock(&lock);
            i--;
            continue;
        }
        /* DEQUEUE SUCCESS */
        //add to test array
        test[test_counter.c_counter++].item_consumed = removed_item;
        printf(ANSI_COLOR_RED "%d    was consumed by consumer->    %d \n" ANSI_COLOR_RESET, removed_item, id);
        //print_buffer(); // THIS IS FOR DEBUGGING, IT DUMPS CONTENTS OF BUFFER
        pthread_mutex_unlock(&lock);
        /* CRITICAL SECTION END SUCCESS */
        sleep(data.CTime);
        
        
        
    }
}

/************** PRODUCER THREAD *************/

void *
producer_thread(void * arg)
{
    /* Get thread ID */
    int id = *((int*) arg);
    for(int i = 0; i < data.X; i++) {
        
        /* while the buffer is full, wait */
        while (data.counter == data.N) {
            ; /* do nothing */
        }
        /* CRITICAL SECTION BEGIN */
        pthread_mutex_lock(&lock);

        /* add item to queue */
        int added_item = enqueue_item(data.item_id);
        if(added_item == -1) {
            pthread_mutex_unlock(&lock);
            i--;
            continue;
        }
        /* ENQUEUE SUCCESS */
        test[test_counter.p_counter++].item_produced = added_item;
        printf(ANSI_COLOR_GREEN "%d    was produced by producer->    %d \n" ANSI_COLOR_RESET , data.item_id, id);
        data.item_id++;
        //print_buffer(); // THIS IS FOR DEBUGGING, IT DUMPS CONTENTS OF BUFFER
        pthread_mutex_unlock(&lock);
        /* CRITICAL SECTION END */
        sleep(data.PTime);
        

    }
}

/*********** TEST ***********/

/* This will test if the producer array matches the consumer array.
If they match, the return value will be the number of items. If
they don't match, it will return -1 */

int
integrity_check()
{
    int flag = 0;
    printf("Producer array     | Consumer array \n----------------------------------- \n");
    for(int i = 0; i < data.item_quantity; i++) {
        if(test[i].item_produced != test[i].item_consumed) {
            flag = 1;
        }
        printf("  %d                |  %d \n", test[i].item_produced, test[i].item_consumed);
    }
    if(flag) {
        return -1;
    } else {
        return data.item_quantity;
    }
}


/******************** MAIN ********************/

int 
main(int argc, char** argv) 
{
    time_t start_time, end_time, delta_time;
    data.item_id = 1;

    /* Get args from user */
    data.N = atoi(argv[1]);
    data.P = atoi(argv[2]);
    data.C = atoi(argv[3]);
    data.X = atoi(argv[4]);
    data.PTime = atoi(argv[5]);
    data.CTime = atoi(argv[6]);

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("Mutex initialization failed.\n");
        return 1;
    }
    /* Make an array of int's that will be our buffers */
    data.total_threads = data.P + data.C;
    data.buffer = malloc( sizeof( int[data.N] ) );
    struct thread_info * thread_args = malloc(data.total_threads*sizeof(struct thread_info));
    data.item_quantity = data.P * data.X;

    // init test struct
    test = malloc(data.item_quantity*sizeof(struct test_arrays));
    test_counter.p_counter = 0;
    test_counter.c_counter = 0;

    /* Print Timestamp */
    start_time = timestamp();

    /* Find out if you need to over consume some items */
    over_consume_check();

    /* Print parameters of this instance of the program */
    print_run_info();

    /* Spawn P Producer threads */
    int p_id = 0;
    int c_id = 0;
    for(int i = 0; i < data.total_threads; i++)
    {
        if(i < data.P) 
        {
            thread_args[i].type = 1;
            thread_args[i].id = p_id+1;
            pthread_create( &thread_args[i].tid, NULL, producer_thread, &thread_args[i] );
            p_id++;
        } 
        else 
        {
            thread_args[i].type = 0;
            thread_args[i].id = c_id+1;
            pthread_create( &thread_args[i].tid, NULL, consumer_thread, &thread_args[i] );
            c_id++;
        }
        
    }

    /* Wait for threads to finish */
    for(int i = 0; i < data.total_threads; i++)
    {
        if(thread_args[i].type == 1) {
            pthread_join(thread_args[i].tid, NULL);
            printf("Producer Thread joined: %d \n", thread_args[i].id);
            
        } else {
            pthread_join(thread_args[i].tid, NULL);
            printf("Consumer Thread joined: %d \n", thread_args[i].id);
        }
        
    }

    /* Print timestamp & calculate total time */
    end_time = timestamp();
    delta_time = end_time-start_time;
    

    /* Test and print results */
    if(integrity_check() < 0) { // TEST FAIL
        printf( ANSI_COLOR_RED " ERROR \n" ANSI_COLOR_RESET);
    } else { // TEST SUCCESS
        printf( ANSI_COLOR_GREEN "\n    Consume and Produce Arrays Match! \n\n" ANSI_COLOR_RESET);
    }

    /* Print total time */
    printf("Total Runtime: %ld secs\n\n", (long)delta_time);


    return 0;
}
