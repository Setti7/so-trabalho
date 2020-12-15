#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

typedef int buffer_item;
#define BUFFER_SIZE 10
#define MAX_READERS 10
#define READERS 13

#define TRUE 1

int reader_count;
int current_readers[READERS];

sem_t file_sem, reader_count_sem, queue_slots_sem;

buffer_item buffer[BUFFER_SIZE];
int counter;

pthread_t tid;
pthread_attr_t attr;

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec) {
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    res = nanosleep(&ts, &ts);

    return res;
}

void print_current_readers() {
    printf("Readers: ");

    for (int i = 0; i < READERS; i++) {
        if (current_readers[i] == 1)
            printf("%d ", i);
    }

    printf("\n");
}

void add_to_current_readers(int id) {
    current_readers[id] = 1;
}

void remove_from_current_reader(int id) {
    current_readers[id] = -1;
}

void clear_current_readers() {
    for (int i = 0; i < READERS; i++) {
        current_readers[i] = -1;
    }
}

_Noreturn void *writer(void *param) {
    buffer_item item;

    sleep(1);

    while (TRUE) {

        sem_wait(&file_sem); // gain access to the database
        printf("Writing to file...\n");

        sleep(1);

        printf("Unblocking file...\n");
        sem_post(&file_sem); // release exclusive access to the database

        sleep(1);
    }
}

_Noreturn void *reader(void *param) {
    buffer_item item;
    int *consumerID = (int *) param;

    printf("consumer %d created\n", *consumerID);

    sleep(2);

    while (TRUE) {
        // sleep(1);
        // printf("Reader %d is waiting\n", *consumerID);

        // Ocupa uma vaga na fila dos leitores, se não tiver vaga, espera.
        sem_wait(&queue_slots_sem);

        sem_wait(&reader_count_sem); // gain access to reader_count

        reader_count = reader_count + 1;       // increment the reader_count
        add_to_current_readers(*consumerID);

        if (reader_count == 1) {
            // if this is the first process to read the database,
            // a down on db is executed to prevent access to the
            // database by a writing process
            sem_wait(&file_sem);
            // printf("Reader %d is the first to get in. Block writing.\n", *consumerID);
        }
        print_current_readers();

        // allow other processes to access reader_count
        sem_post(&reader_count_sem);


        msleep(100);
        // printf("Reader %d is reading\n", *consumerID);


        sem_wait(&reader_count_sem); // gain access to reader_count
        reader_count = reader_count - 1; // decrement reader_count
        if (reader_count == 0) {
            // if there are no more processes reading from the
            // database, allow writing process to access the data
            // printf("Reader %d is the last reader to exit. Allow writing.\n", *consumerID);
            clear_current_readers();
            sem_post(&file_sem);
        }
        remove_from_current_reader(*consumerID);
        print_current_readers();
        // allow other processes to access reader_countuse_data();
        // use the data read from the database (non-critical)
        sem_post(&reader_count_sem);
        sem_post(&queue_slots_sem);

        msleep(2000);
    }
}

void initializeData() {

    clear_current_readers();

    sem_init(&file_sem, 0, 1);
    sem_init(&reader_count_sem, 0, 1);
    sem_init(&queue_slots_sem, 0, MAX_READERS);

    pthread_attr_init(&attr);

    counter = 0;
}

int main() {
    /* Loop counter */
    int i;

    int numCons = READERS; /* Number of consumer threads */

    /* Initialize the app */
    initializeData();

    pthread_create(&tid, &attr, writer, NULL);

    int taskIds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    /* Create the consumer threads */
    for (i = 0; i < numCons; i++) {
        /* Create the thread */
        pthread_create(&tid, &attr, reader, &taskIds[i]);
    }

    /* Sleep for the specified amount of time in milliseconds */
    /* Create the consumer threads */
    for (i = 0; i < 20; i++) {
        sleep(1);
    }

    /* Exit the program */
    printf("Exit the program\n");
    exit(0);
}