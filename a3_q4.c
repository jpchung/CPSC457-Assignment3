#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <semaphore.h>

/**
*CPSC 457 - Assignment 3
* Part A - Coffee Shop Simulation
* Johnny Phuong Chung
* 10036448
* Code modified from skeleton by: Tyson Kendon
* Refer to README for additional cited resources
*/


/****
 *
 * This is a skeleton for assignment 3. It includes the code to handle the 
 * production and clean up of customers. Your job, is to write the barista
 * threads and manage the buffers (pthread_mutexes will be your friend).
 * 
 *
 */

/**
 * Definition of the maximum buffer size
 */
#define MAX_BUFFER_SIZE 10

/**
 * Definition of how many customers we will processes and how many
 * baristas we'll have serve them
 */
#define TOTAL_CUSTOMERS 15
#define TOTAL_BARISTAS 2



/* Data structure to pass data to the customer thread */
typedef struct _customer_data_t {
  int tid;
} customer_data_t; 

/* Data structure to pass data to the cleaner thread */
typedef struct _cleaner_data_t {
  int tid;
} cleaner_data_t; 

/* Data structures to organize information about the order */
typedef struct _item_t {
	int production_time_ms;
} item_t;

typedef struct _order_t {
	int customer;
	int size;
	item_t items [];
} order_t;





/* Data structure to pass data to train model barista thread*/
typedef struct _bar_train_data_t {
	int tid;
} bar_train_data_t;

/* Data structure to pass data to space model barista thread*/
typedef struct _bar_space_data_t {
	int tid;
} bar_space_data_t;

/*barista model variable*/
int bar_model;



/*Mutexes to lock critical sections*/
pthread_mutex_t order_mutex;
pthread_mutex_t bar_mutex;
pthread_mutex_t cash_mutex;
pthread_mutex_t prod_mutex;

/*semaphores to manage control of buffers*/
sem_t order_empty_sem;
sem_t order_full_sem;
sem_t bar_empty_sem;
sem_t bar_full_sem;
sem_t prod_empty_sem;
sem_t prod_full_sem;


/**
* Intermediate buffer for space model
*/
order_t ** prod_buffer;
int prod_index;


/**
 * Buffer to manage the order data
 */
order_t ** order_buffer;
int order_index;

/**
 * Buffer to manage the returned "products"
 */

int * bar_buffer;
int bar_index;

/**
 * Integer to play the "cash register"
 */

int cash;

/* Counter for how many customers yet to processes */
int customer_total = TOTAL_CUSTOMERS;

/*
 * Calculate the amount for the customer thread to sleep 
 */
void decide_time_to_sleep(struct timespec *st) {

	int adjust_ms, base_ms;

	/* Assume we want to wait about half a second */
	base_ms = 500;
	adjust_ms = (rand() % 500) - 250;

	/* Set adjusted time in timespec */
	st->tv_sec = 0;
	st->tv_nsec = (base_ms + adjust_ms) * 1000000L;

}

/*
 * Generate how long a barista must "work" to make the order
 */
int generate_prduction_time_ms() {


	/* 2 new possible orders, one larger production time, one smaller*/
	int c = (rand() % 5);


	int retVal;
	switch(c) {
		case 0 : retVal = 200; break;
		case 1 : retVal = 400; break;
		case 2 : retVal = 800; break;
		case 3 : retVal = 2000; break;
		case 4 : retVal = 10; break;
	}

	return retVal;	

}

/*
 * For a given order size, generate items to be produced
 */
void generate_order (order_t *  order) {

	int i;
	for (i = 0; i < order->size; i++) {

		item_t order_part;
		order_part.production_time_ms = generate_prduction_time_ms();
		order->items[i] = order_part;
	}

}

/*
 * Generate a size for an order
 */ 
int generate_order_size() { 

		int o_size; 
		o_size = (rand() % 4) + 1;
		return(o_size);

}

/*Print customer orders */
void print_order(order_t * order) {

		printf("Order %d (%d)\n", order->customer, order->size);
		for(int i = 0; i < order->size; i++) {
			printf("\tItem %d: %d\n", i + 1, order->items[i].production_time_ms);

		}

}

 
/*
 * Main Customer Thread function.
 *
 * Generate customers and orders over time, clean up as customers are
 * served.
 */
void *customer_run(void *arg) {
	//customer_data_t *data = (customer_data_t *)arg;

	int customer_id = 0;
	int order_size = 0;
	int remaining_customers = customer_total;
	
	/* Run for as long as we have customers incoming */
	while (remaining_customers > 0) {
		sem_wait(&order_empty_sem);


		/* Create customer and generate order */
		printf("------------------------------\n");
		printf("Created Customer %d\n", ++customer_id); 
		order_size = generate_order_size();	

		order_t * new_order = malloc(sizeof (order_t) + sizeof (item_t [order_size]));
		new_order->customer=customer_id;
		new_order->size=order_size;
		printf("Ordering %d items\n", new_order->size);
		generate_order(new_order);

		/*add to order buffer - lock until complete*/
		pthread_mutex_lock(&order_mutex);

		order_buffer[order_index] = new_order;	
    		order_index++;

		pthread_mutex_unlock(&order_mutex);


		/*signal to threads waiting for order buffer*/
		sem_post(&order_full_sem);

		/* Put the thread to sleep until the next customer comes */
		struct timespec c;
		decide_time_to_sleep(&c);
		printf("Sleeping Until Next Customer in: %lld.%.3ld\n", (long long)c.tv_sec, c.tv_nsec);
		struct timespec r;
		nanosleep(&c, &r);
		
		remaining_customers--;
	}

	pthread_exit(NULL);
}


/*
 * Train Model Barista Thread function.
 *
 * Each barista will receive orders from buffer, take cash, and make product for bar buffer
 */
void *barista_train_run(void *arg) {
	
	while(1) {

		int *tid = (int*) arg;

		/*wait for signal to proceed into order buffer*/
		sem_wait(&order_full_sem);


		/*remove and print order from buffer for processing - lock until complete*/
		pthread_mutex_lock(&order_mutex);

		order_index--;
		order_t *order_pending = order_buffer[order_index];
		print_order(order_pending);
		printf("Barista: %d working on order %d\n", *tid, order_pending->customer);

		pthread_mutex_unlock(&order_mutex);


		/*signal threads waiting on order buffer*/
		sem_post(&order_empty_sem);


		/*put thread to sleep to simulate order processing*/
		for(int i = 0;i < order_pending->size; i++) {
			usleep((order_pending->items[i].production_time_ms)*1000);
		}
	
		/*wait ofr signal to proceed into bar buffer*/
		sem_wait(&bar_empty_sem);

		/*create product to place into bar buffer - lock until complete*/
		pthread_mutex_lock(&bar_mutex);

		bar_buffer[bar_index] = order_pending->customer;
		bar_index++;

		cash += 10;

		pthread_mutex_unlock(&bar_mutex);

	
		/*signal to threads waiting on bar buffer*/
		sem_post(&bar_full_sem);
	}
	pthread_exit(NULL);
}


void *barista_space_run(void *arg) {
	while(1) {

		int *tid = (int*) arg;

		/*check if cashier barista*/
		if(*tid == 2002) {
			/*wait for signal to proceed into order buffer*/
			sem_wait(&order_full_sem);


			/*remove and print order from buffer for processing - lock until complete*/
			pthread_mutex_lock(&order_mutex);

			order_index--;
			order_t *order_pending = order_buffer[order_index];
			print_order(order_pending);

			
			printf("Barista: %d is working on order %d\n", *tid, order_pending->customer);

			pthread_mutex_unlock(&order_mutex);


			/*signal threads waiting on order buffer*/
			sem_post(&order_empty_sem);



			/*wait for empty slots in production buffer*/
			sem_wait(&prod_empty_sem);

			/*add to production buffer - lock until complete*/
			pthread_mutex_lock(&prod_mutex);

			prod_buffer[prod_index] = order_pending;
			prod_index++;

			pthread_mutex_unlock(&prod_mutex);

			/*signal to producer threads waiting for prod buffer*/
			sem_post(&prod_full_sem);

		}
		/*otherwise, producer barista*/
		else if (*tid >= 2003) {
			/*wait for signal that cashier has placed an order in prod buffer*/
			sem_wait(&prod_full_sem);

			/*remove from prod buffer, to add to bar buffer - lock until complete*/
			pthread_mutex_lock(&prod_mutex);
			


			prod_index--;
			order_t* prod_to_bar = prod_buffer[prod_index];

			printf("Barista: %d is working on order %d\n", *tid, prod_to_bar->customer);

			pthread_mutex_unlock(&prod_mutex);

			/*signal to threads waiting that production filled, should be put in bar buffer*/
			sem_post(&prod_empty_sem);


			/*put thread to sleep to simulate order processing*/
			for(int i = 0;i < prod_to_bar->size; i++) {
				usleep((prod_to_bar->items[i].production_time_ms)*1000);
			}
	
			/*wait ofr signal to proceed into bar buffer*/
			sem_wait(&bar_empty_sem);

			/*create product to place into bar buffer - lock until complete*/
			pthread_mutex_lock(&bar_mutex);

			//bar_buffer[bar_index] = order_pending->customer;
			bar_buffer[bar_index] = prod_to_bar->customer;
			bar_index++;

			cash += 10;

			pthread_mutex_unlock(&bar_mutex);

	
			/*signal to threads waiting on bar buffer*/
			sem_post(&bar_full_sem);
		}

		


		
	}

	pthread_exit(NULL);
}

/*
 * Cleanup thread function 
 * Sum cash total and clean up bar buffer
 */ 
void *cleanup_run(void *arg) {

	int customers_cleared = 0;
	int cash_total;

	//int bar;

	while (customers_cleared < customer_total) {
		
		/*wait for signal to proceed into bar buffer*/
		sem_wait(&bar_full_sem);

		/* Take the next bar out of the buffer - lock until complete*/
		pthread_mutex_lock(&bar_mutex);

		bar_index--;
		//bar = bar_buffer[bar_index];

		customers_cleared++;
		cash_total = cash;

		pthread_mutex_unlock(&bar_mutex);

		printf("*** Cleared %d customers - Cash (%d) ***\n", customers_cleared, cash_total);

		/*signal for*/
		sem_post(&bar_empty_sem);

	}

	pthread_exit(NULL);
}
 
/*
 * Run the main simulation
 */
int main(int argc, char **argv) {

	int i, rc;

	/*time structures to measure completion time*/
	time_t simStart, simEnd;

	/*check correct number of command line args*/
	if(argc <= 3) {
		printf("Usage: ./a3 [-bar_model] [bar_num] [cust_num]\n");
		return EXIT_FAILURE;
	}

	/*check character option input from command line for barista model*/
	int opt = getopt(argc, argv, "st");
	switch(opt) {
		case('s'):
			bar_model = 0;
			break;
		case('t'):
			bar_model = 1;
			break;
		default:
			printf("Usage: ./a3 [-bar_model] [bar_num] [cust_num]\n");
			return EXIT_FAILURE;
			break;
	}

	/*check number of baristas specified by command line*/
	int barNum = atoi(argv[2]);

	if((barNum) == 0) {
		printf("Not enough baristas\n");
		printf("Usage: ./a3 [-bar_model] [bar_num] [cust_num]\n");
		return EXIT_FAILURE;
	}

	/*check number of customers specified by command line*/
	int custNum = atoi(argv[3]);
	if((custNum) == 0) {
		printf("Not enough customers\n");
		printf("Usage: ./a3 [-bar_model] [bar_num] [cust_num]\n");
		return EXIT_FAILURE;
	}
	customer_total = custNum;

	

	/* Intializes random number generator */
	time_t t;
	srand((unsigned) time(&t));

	/* Initalize buffers */
	order_buffer = malloc (sizeof(order_t) * MAX_BUFFER_SIZE);
	order_index = 0;
	bar_buffer = malloc (sizeof(int) * MAX_BUFFER_SIZE);
	bar_index = 0;
	cash=0;

	prod_buffer = malloc (sizeof(int) * MAX_BUFFER_SIZE);
	prod_index = 0;	
	

	/*Initialize mutex locks*/
	if((pthread_mutex_init(&order_mutex, NULL)) != 0) {
		printf("error: pthread_mutex_init\n");	
		return EXIT_FAILURE;
	}

	if((pthread_mutex_init(&bar_mutex, NULL)) != 0) {
		printf("error: pthread_mutex_init\n");	
		return EXIT_FAILURE;
	}

	if((pthread_mutex_init(&cash_mutex, NULL)) != 0) {
		printf("error: pthread_mutex_init\n");	
		return EXIT_FAILURE;
	}

	if((pthread_mutex_init(&prod_mutex, NULL))!= 0) {
		printf("error: pthread_mutex_init\n");	
		return EXIT_FAILURE;
	}

	/*Initialize order buffer semaphores*/
	if((sem_init(&order_empty_sem, 0, MAX_BUFFER_SIZE)) != 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}
	if((sem_init(&order_full_sem, 0, 0)) != 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}

	/*Initialize bar buffer semaphores*/
	if((sem_init(&bar_empty_sem, 0, MAX_BUFFER_SIZE)) != 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}

	if((sem_init(&bar_full_sem, 0, 0)) != 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}

	/*Initialize product buffer semaphores*/
	if((sem_init(&prod_empty_sem, 0, MAX_BUFFER_SIZE))!= 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}

	if((sem_init(&prod_full_sem, 0, 0))!= 0) {
		printf("error: sem_init\n");
		return EXIT_FAILURE;
	}




	/*get start time */
	time(&simStart);

	/* Create Customer Thread */
	pthread_t cust_t;
	customer_data_t cust_data;
	cust_data.tid=1000;
	if ((rc = pthread_create(&cust_t, NULL, customer_run, &cust_data))) {
		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		return EXIT_FAILURE;
	}

	/* Create Cleaner Thread */
	pthread_t cleaner_t;
	cleaner_data_t cleaner_data;
	cleaner_data.tid=1001;
	if ((rc = pthread_create(&cleaner_t, NULL, cleanup_run, &cleaner_data))) {
		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		return EXIT_FAILURE;
	}
 


	/*baristas multithread array*/
	pthread_t bar_threads[barNum];
	bar_train_data_t bar_train_data[barNum];
	bar_space_data_t bar_space_data[barNum];

	/* Create Barista Threads - depends on barista model specified */
	if(bar_model == 1) {
		/*train model*/
		for(i = 0; i < barNum; i++){
			/*array for data*/

			bar_train_data[i].tid = 1002 + i;

			/*create thread*/
			rc = pthread_create(&bar_threads[i], NULL, barista_train_run, &bar_train_data[i]);
			if(rc != 0) {
				fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
				return EXIT_FAILURE;

			}
		}

	}
	else if(bar_model == 0) {
		/*space model*/
		for(i = 0; i < barNum; i++) {
			/*array for data*/

			bar_space_data[i].tid = 2002 + i;
		
			/*create thread*/
			rc = pthread_create(&bar_threads[i], NULL, barista_space_run, &bar_space_data[i]);
			if(rc != 0) {
				fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
				return EXIT_FAILURE;

			}
		}
	}


	/*wait for thread termination*/
	pthread_join(cust_t, NULL);
	pthread_join(cleaner_t, NULL);
	if(bar_model == 1) {

		for(i = 0; i < barNum; i++) {
			pthread_cancel(bar_threads[i]);
			pthread_join(bar_threads[i], NULL);
		}

	}
	else if(bar_model == 0) {

		for(i = 0; i < barNum; i++) {
			pthread_cancel(bar_threads[i]);
			pthread_join(bar_threads[i], NULL);
		}
	}


	/*destroy mutex locks once done process*/
	pthread_mutex_destroy(&order_mutex);
	pthread_mutex_destroy(&bar_mutex);
	pthread_mutex_destroy(&cash_mutex);
	pthread_mutex_destroy(&prod_mutex);
 
	/*get end time*/
	time(&simEnd);

	double elapsed = difftime(simEnd, simStart);
	printf("\n>>>>> ELAPSED SIMULATION TIME: %.2lf SECONDS <<<<<\n", elapsed);

	return EXIT_SUCCESS;
}
