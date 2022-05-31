#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>

void * barber_function(void *idp);
void * customer_function(void *idp);
void service_customer();
void * make_customer_function();

pthread_mutex_t srvCust;

sem_t barber_ready; 
sem_t customer_ready;
sem_t modifySeats;

const int chair_count = 3;
const int total_custs = 3;
int available_seats = chair_count;
int leaved_customers = 0;
time_t waiting_time_sum;
int result[total_custs] = {0};

int service_times[total_custs] = {3, 4, 1};
int arrival_times[total_custs] = {3, 4, 5};

typedef struct {
    int counter;
} compute_prime_struct;

void * barber_function(void *idp)
{    
    int counter = 0;
    
    while(1)
    {
        sem_wait(&customer_ready);
        sem_wait(&modifySeats);
        available_seats++;
        sem_post(&modifySeats);
        sem_post(&barber_ready);

        pthread_mutex_lock(&srvCust);
        service_customer(service_times[counter]);
        pthread_mutex_unlock(&srvCust);
        
        printf("Customer was served.\n");
        counter++; 
        if(counter == (total_custs - leaved_customers))
            break;

    }
    pthread_exit(NULL);    
}

void * customer_function(void* idp)
{
    compute_prime_struct *actual_args = idp;
    int counter = actual_args->counter;
    struct timeval start, stop;

    sem_wait(&modifySeats); 

    if(available_seats >= 1)
    {
        available_seats--;
        printf("Customer[pid = %lu] is waiting.\n", pthread_self());
        printf("Available seats: %d\n", available_seats);
   
        gettimeofday(&start, NULL);
        sem_post(&customer_ready);
        sem_post(&modifySeats);
        sem_wait(&barber_ready);
        gettimeofday(&stop, NULL);

        printf("customer %lu, %ld, %ld\n", pthread_self(), start.tv_sec, stop.tv_sec);
        
        double sec = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
        waiting_time_sum += 1000 * sec;
        printf("Customer[pid = %lu] is being served. \n", pthread_self());
        result[counter] += stop.tv_sec;
    }
    else
    {
        sem_post(&modifySeats);
        leaved_customers++;
        printf("A Customer left.\n");
    }
    pthread_exit(NULL);
}

void service_customer(int service_time){
    usleep(service_time * 1000 * 1000);
    printf("End of service time\n");
}

void * make_customer_function(){
    int tmp;   
    int counter = 0;

    while(counter < total_custs)
    {
        struct timeval start, stop;
        gettimeofday(&start, NULL);
        usleep((arrival_times[counter] - (counter ? arrival_times[counter - 1] : 0)) * 1000 * 1000);
        gettimeofday(&stop, NULL);
        result[counter] = -stop.tv_sec;
        pthread_t customer_thread;
        compute_prime_struct *args = malloc(sizeof *args);
        args -> counter = counter;
        tmp = pthread_create(&customer_thread, NULL, (void *)customer_function, args);  
        if(tmp)
            printf("Failed to create thread.");
        counter++;
    }
}

int main(){
    pthread_t barber;
    pthread_t customer_maker;

    pthread_mutex_init(&srvCust, NULL);

    sem_init(&customer_ready, 0, 0);
    sem_init(&barber_ready, 0, 0);
    sem_init(&modifySeats, 0, 1);

    int tmp;
    tmp = pthread_create(&barber, NULL, (void *)barber_function, NULL);  
    if(tmp)
        printf("Failed to create thread."); 
    tmp = pthread_create(&customer_maker, NULL, (void *)make_customer_function, NULL);  
    if(tmp)
        printf("Failed to create thread."); 

    pthread_join(barber, NULL);
    pthread_join(customer_maker, NULL);
        
    printf("\n------------------------------------------------\n");
    printf("Average customers waiting time: %f ms.\n", (waiting_time_sum / (double) (total_custs - leaved_customers)));
    printf("Number of customers leaved: %d\n", leaved_customers);    	
}
