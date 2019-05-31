#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define ARRAY_SIZE 1000000
#define N ARRAY_SIZE
#define THRESHOLD 10

typedef enum messageType {
    WORK, SHUTDOWN, FINISH
}MessageType;

typedef struct message {
    MessageType type;
    int start;
    int n;
}Message;

Message messagesQueue[N] = {};
int messageOut = 0, messageIn = 0;
int numberOfMessages = 0;
double *a;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msgIn = PTHREAD_COND_INITIALIZER;
pthread_cond_t msgOut = PTHREAD_COND_INITIALIZER;



void send(MessageType type, int start, int end) {
    pthread_mutex_lock(&mutex);
    while (numberOfMessages >= N) {
        printf("\nSend Message Was Locked\n");
        pthread_cond_wait(&msgOut, &mutex);
    }
    
    // Enqueue message
    messagesQueue[messageIn].type = type;
    messagesQueue[messageIn].start = start;
    messagesQueue[messageIn].n = end;
    messageIn = (messageIn + 1) % N;
    numberOfMessages++;

    pthread_cond_signal(&msgIn);
    pthread_mutex_unlock(&mutex);
}

void receive(MessageType *type, int *start, int *end) {
    pthread_mutex_lock(&mutex);
    while (numberOfMessages < 1) {
        printf("\nReceive Message Was Locked\n");
        pthread_cond_wait(&msgIn, &mutex);
    }

    // Dequeue message
    *type = messagesQueue[messageOut].type;
    *start = messagesQueue[messageOut].start;
    *end = messagesQueue[messageOut].n;
    messageOut = (messageOut + 1) % N;
    numberOfMessages--;

    pthread_cond_signal(&msgOut);
    pthread_mutex_unlock(&mutex);
}


//Swaps place in two items
void swap(double *one, double *two){
    double t = 0;

    t = *one;
    *one = *two;
    *two = t;
}

//Handles the sorting
void insort(double *a, int n) {
    int i,j;
    double t;
    for(int i = 0; i < n; i++) {
        j = i;
        while((j>0) && (a[j-1] > a[j])){
            swap(a + (j-1), a + j);
            j--;
        }
    }
}


//Splits the array as needed, returns the pivot
int partition(double *a, int n){
    int first = 0;
    int middle = n/2;
    int last = n-1;
    double p = a[middle];
    int i,j;

    if (a[first] > a[middle]) {
        swap(a + first,a + middle);
    }
    if (a[middle] > a[last]) {
        swap(a + middle,a + last);
    }
    if (a[first] > a[middle]) {
        swap(a + first,a + middle);
    }

    for(i = 1, j=n-2 ;; i++, j--) {
        while(a[i] < p){
            i++;
        }
        while(a[j] > p){
            j--;
        }
        if (i >= j){break;}
        
        swap(a + i, a + j);
    }
    return i;
}

void *threadFunc(void *params){
    int start = 0;
    int end = 0;
    MessageType type = WORK;

    while (1){
        receive(&type, &start, &end);

        if (type == WORK){
            // printf("Working...\n");
            if (end - start <= THRESHOLD) {
                insort(a + start, end - start);
                // printf("Sending finish\n");
                send(FINISH, start, end);
            } else {
                int pivot = partition(a + start, end - start);
                // printf("Sending finish\n");
                send(WORK, start, start + pivot);
                send(WORK, start + pivot, end);
            }
        } else if(type == SHUTDOWN){
            // printf("SHUTDOWN\n");
            send(SHUTDOWN, start, end);
            break;
        } else {
            // printf("FINISH\n");
            send(type, start, end);

        }
        // printf("type: %d, start: %d\n", type, start);
    }
    pthread_exit(NULL);
}



int main(int argc, char *argv[]){
    
    pthread_t thread[THREADS] = {};

    int completed = 0;
    MessageType type = WORK;
    int start, end;

    int errorsFound = 0;


    printf("Allocating a\n");

    //Allocating memory
    a = (double *) malloc(sizeof(double) * ARRAY_SIZE);
    if (!a) {
        exit(-1);
    }

    srand(time(NULL));
    for(int i = 0; i < N; i++) {
        a[i] = (double) rand()/RAND_MAX;
    }

    send(WORK, 0, ARRAY_SIZE);
    
    for (int i = 0; i < THREADS; i++) {
        if (pthread_create(&thread[i], NULL, threadFunc, NULL) != 0) {
            printf("Thread creation error\n");
            exit(1);
        }
    }

    while (completed < ARRAY_SIZE) {
        receive(&type, &start, &end);
        if (type == FINISH){
            completed += end - start;
            // printf("completed: %d\n", completed);
        } else {
            send(type, start, end);
        }
    }
    send(SHUTDOWN, 0, 0);

    for (int i = 0; i < THREADS; i++) {
        pthread_join(thread[i], NULL);
    }

    for(int i = 0; i < (N-1); i++) {
        if (a[i] > a[i+1]) {
            errorsFound++;
        }
    }

    if (errorsFound > 0){
        printf("Sort Process Exited With Errors.\n");
    } else {
        printf("Array Was Sorted Correctly.\n");
    }

    printf("Destroying Mutex\n");
    pthread_mutex_destroy(&mutex);
    printf("Destroying Con V msgIn\n");
    pthread_cond_destroy(&msgIn);
    printf("Destroying Con V msgOut\n");
    pthread_cond_destroy(&msgOut);

    printf("Dealocating a\n");
    free(a);

    return 0;
}
