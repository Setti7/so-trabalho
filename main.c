#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

typedef int buffer_item;
#define MAX_READERS 10
#define READERS 13

#define TRUE 1

// Número de leitores atuais
int reader_count;

// Contêm 1 na posição x se o leitor x está lendo o arquivo, caso contrário,
// possui -1.
int current_readers[READERS];

// Semáforo do arquivo.
// O file_sem controla o acesso ao arquivo. Ele tem valor inicial 1.
sem_t file_sem;

// reader_count_sem é o semáforo que controla o acesso à varíavel reader_count,
// que possui o número atual de leitores do arquivo. Tem valor inicial 1.
sem_t reader_count_sem;

// queue_slots_sem é o semáforo que permite o acesso concorrente de até no máximo
// MAX_READERS ao arquivo.
sem_t queue_slots_sem;

pthread_t tid;
pthread_attr_t attr;

// Função para fazer sleep de milisegundos.
int msleep(long msec) {
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    res = nanosleep(&ts, &ts);

    return res;
}

void print_current_readers() {
    printf("Leitores: ");

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

    // sleep para dar um tempo de startup para ser mais fácil de testar.
    // Pode ser removido na versão final.
    sleep(1);

    while (TRUE) {

        /*
         * Aqui deve ser criado o item que for escrever no arquivo, antes de
         * bloquear a execução dos outros processos.
         * */

        // Ganha acesso ao arquivo
        sem_wait(&file_sem);
        printf("Escrevendo no arquivo...\n");

        sleep(1);
        /*
         * Aqui deve ser feito a operação com o arquivo.
         * */

        // Desbloqueia acesso ao arquivo
        printf("Liberando arquivo...\n");
        sem_post(&file_sem);

        // sleep apenas para testar. Pode ser removido na versão final.
        sleep(1);
    }
}

_Noreturn void *reader(void *param) {
    buffer_item item;
    int *consumerID = (int *) param;

    printf("Leitor %d criado\n", *consumerID);

    sleep(2);

    while (TRUE) {

        // Ocupa uma vaga na fila dos leitores, se não tiver vaga, espera.
        sem_wait(&queue_slots_sem);

        // Ganha acesso ao reader_count e o incrementa, já que o processo
        // atual vai acessar o arquivo. Além disso, adiciona o processo
        // atual à lista dos leitores.
        sem_wait(&reader_count_sem);
        reader_count++;
        add_to_current_readers(*consumerID);

        // Caso esse processo seja o primeiro a ler o arquivo, damos um down
        // do semáforo do arquivo para previnir que o escritor ganhe acesso.
        // Isso só é feito para o primeiro leitor pois os outros leitores
        // poderão acessar o arquivo simultanêamente.
        if (reader_count == 1) {
            sem_wait(&file_sem);
        }

        // Para debug apenas. Talvez seja interessante deixar para mostrar
        // que no máximo MAX_READERS estão tendo acesso ao arquivo e quais
        // são eles.
        print_current_readers();

        // desbloqueia o acesso ao reader_count
        sem_post(&reader_count_sem);


        msleep(100);
        /*
         * Aqui deve ser feito a operação com o arquivo.
         * */

        // Ganha acesso ao reader_count e o decrementa, já que o processo
        // atual vai para de acessar o arquivo. Além disso, remove o processo
        // atual da lista dos leitores.
        sem_wait(&reader_count_sem);
        reader_count--;
        remove_from_current_reader(*consumerID);


        // Caso esse processo seja o último a para de ler o arquivo, damos um up
        // no semáforo do arquivo para permitir que o escritor ganhe acesso.
        if (reader_count == 0) {
            sem_post(&file_sem);
        }

        // Para debug apenas. Talvez seja interessante deixar para mostrar
        // que no máximo MAX_READERS estão tendo acesso ao arquivo e quais
        // são eles.
        print_current_readers();

        // Permitir que outro processo acesse reader_count e libera uma vaga para
        // leitura.
        sem_post(&reader_count_sem);
        sem_post(&queue_slots_sem);

        // sleep para debug apenas
        msleep(2000);
    }
}


int main() {
    int i;

    // Inicializa os semáforos, vetores e os atributos das threads.
    clear_current_readers();

    sem_init(&file_sem, 0, 1);
    sem_init(&reader_count_sem, 0, 1);
    sem_init(&queue_slots_sem, 0, MAX_READERS);

    pthread_attr_init(&attr);

    // Cria apenas uma thread de escritor.
    pthread_create(&tid, &attr, writer, NULL);

    int taskIds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    // Cria as threads dos leitores
    for (i = 0; i < READERS; i++) {
        pthread_create(&tid, &attr, reader, &taskIds[i]);
    }

    // Ao invés de um sleep longo é usado vários pequenos para facilitar o debug.
    for (i = 0; i < 20; i++) {
        sleep(1);
    }

    exit(0);
}