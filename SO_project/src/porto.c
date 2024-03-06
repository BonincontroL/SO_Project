#define _GNU_SOURCE

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <fcntl.h>
#include "porto.h"

#define DEFAULT_VAL -1

/* variabili globali di configurazione simulazione */
int SO_BANCHINE = DEFAULT_VAL;
int SO_LOADSPEED = DEFAULT_VAL;
int SO_FILL = DEFAULT_VAL;
int SO_MERCI = DEFAULT_VAL;
int SO_LATO = DEFAULT_VAL;
int SO_PORTI = DEFAULT_VAL;
int SO_MIN_VITA = DEFAULT_VAL;
int SO_MAX_VITA = DEFAULT_VAL;

Porto new_porto(double px, double py, int id);
double tempo_scambio_merci(int ton);
void genera_domanda_offerta(Porto p);
void stampa_domanda(Porto p);
void stampa_offerta(Porto p);
int ispresent_offerta(int *id_offerta, int N, int elem);
int ispresent_domanda(int *vett, int N, int elem);
int esiste_banchina_libera(Porto p);
int porto_get_id(Porto p);
int porto_ha_bisogno_di_id(Porto p, int id);
void load_config();
Coordinates get_corner_coordinates(int max_X, int max_Y, int num_angolo);
Coordinates get_random_coordinates(int max_X, int max_Y);
void genera_domanda_offerta(Porto p);

int main(int argc, char *argv[])
{

#pragma region init_var
    /* init var */
    Coordinates c;
    Porto p = NULL;
    int i;

    /* var creazione_memoria_condivisa_per_avvio_e_terminazione_programma */
    int shmid;
    key_t key;
    int *shared_memory;
    /* var creazione_semafori_avvio_e_terminazione */
    sem_t *sem_memoria;

    /* var creazione_memoria_condivisa_per_porti */
    int shared_mem_porto_id;
    size_t spazio_alloca;
    key_t shared_mem_porto_key;
    Porto shared_mem_porto;

    /* var creazione_sem_ports_initialized */
    sem_t *sem_ports_initialized;

    int Continua; /* var Avvio - Terminazione */

    load_config(argv);
#pragma endregion

#pragma region crea_processi_porti
    for (i = 0; i < SO_PORTI; i++)
    {
        if (fork() == 0)
        {
            if (i < 4)
                c = get_corner_coordinates(SO_LATO, SO_LATO, i);
            else
                c = get_random_coordinates(SO_LATO, SO_LATO);

            p = new_porto(c.x, c.y, i);

            break;
        }
    }

    if (i == SO_PORTI)
        exit(0);

    genera_domanda_offerta(p);
    /* printf("id: %d\n",p->id);
    stampa_offerta(p);
    stampa_domanda(p); */
#pragma endregion

#pragma region sem_ports_initialized

    sem_ports_initialized = sem_open("ports_initialized", 0666, 0);
    if (sem_ports_initialized == SEM_FAILED)
    {
        perror("sem_open for ports_initialized porto ");
        exit(1);
    }
#pragma endregion

#pragma region creazione_memoria_condivisa_per_avvio_e_terminazione_programma
    key = ftok("/tmp", 65);
    shmid = shmget(key, sizeof(int), 0666);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }

    shared_memory = shmat(shmid, NULL, 0);
    if (shared_memory == (int *)-1)
    {
        perror("shmat");
        exit(1);
    }
#pragma endregion

#pragma region creazione_semafori_avvio_e_terminazione
    sem_memoria = sem_open("/avvio", 0);
    if (sem_memoria == SEM_FAILED)
    {
        perror("sem_open");

        exit(1);
    }
#pragma endregion

#pragma region creazione_memoria_condivisa_per_porti
    /* carico porti in memoria condivisa */

    shared_mem_porto_key = ftok("/etc", 1);
    spazio_alloca = sizeof(struct porto) * SO_PORTI;
    shared_mem_porto_id = shmget(shared_mem_porto_key, spazio_alloca, IPC_CREAT | 0666);
    if (shared_mem_porto_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    shared_mem_porto = (Porto)shmat(shared_mem_porto_id, NULL, 0);

    if (shared_mem_porto == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    shared_mem_porto[p->id] = *p;

#pragma endregion

    sem_post(sem_ports_initialized);

    Continua = 1;
    while (Continua)
    {
        sem_wait(sem_memoria);
        Continua = *shared_memory;
        sem_post(sem_memoria);
    }

    /* printf("PROCESSO porto FINE\n"); */
    return 0;
}

Porto new_porto(double px, double py, int id)
{
    Porto val;
    srand(getpid());
    val = malloc(sizeof(*val));

    val->c.x = px;
    val->c.y = py;
    val->banchine_occupate = 0;
    val->num_banchine = rand() % (SO_BANCHINE) + 1;
    val->id = id;
    val->merce_ricevuta = 0;
    val->merce_spedita = 0;

    return val;
}

double tempo_scambio_merci(int ton)
{
    return ton / SO_LOADSPEED;
}

void genera_domanda_offerta(Porto p)
{
    int i, k;

    do
    {
        p->offerta_N = rand() % SO_MERCI + 1;
    } while (p->offerta_N == SO_MERCI);

    for (i = 0; i < p->offerta_N; i++)
    {
        *(p->id_offerta + i) = -1;
        *(p->qt_offerta + i) = -1;
        *(p->temp_vita_offerta + i) = -1;
    }

    for (i = 0; i < p->offerta_N; i++)
    {
        do
        {
            p->id_offerta[i] = rand() % SO_MERCI + 1;
        } while (ispresent_offerta(p->id_offerta, i, p->id_offerta[i]));

        p->qt_offerta[i] = SO_FILL;
        p->temp_vita_offerta[i] = rand() % (SO_MAX_VITA - SO_MIN_VITA + 1) + SO_MIN_VITA;
    }

    do
    {
        p->domanda_N = rand() % SO_MERCI + 1;
    } while (p->domanda_N == SO_MERCI || p->domanda_N > SO_MERCI - p->offerta_N);

    for (i = 0; i < p->domanda_N; i++)
    {
        do
        {
            p->domanda_id[i] = rand() % SO_MERCI + 1;
        } while (ispresent_offerta(p->id_offerta, p->offerta_N, p->domanda_id[i]) ||
                 ispresent_domanda(p->domanda_id, i, p->domanda_id[i]));

        p->domanda_qt[i] = SO_FILL;
    }
}

void stampa_domanda(Porto p)
{
    int i;
    for (i = 0; i < p->domanda_N; i++)
    {
        printf("domanda: id: %d\tqt:%d\n", p->domanda_id[i], p->domanda_qt[i]);
    }
}

void stampa_offerta(Porto p)
{
    int i;
    for (i = 0; i < p->offerta_N; i++)
    {
        printf("offerta: id: %d\tqt:%d\ttemp_vita:%d\n", p->id_offerta[i], p->qt_offerta[i], p->temp_vita_offerta[i]);
    }
}

/*bool è presente offerta nel vettore?*/
int ispresent_offerta(int *id_offerta, int N, int elem)
{
    int k;
    for (k = 0; k < N; k++)
    {
        if (id_offerta[k] == elem)
            return 1;
    }
    return 0;
}
/*bool è presente domanda nel vettore?*/
int ispresent_domanda(int *vett, int N, int elem)
{
    int k;
    for (k = 0; k < N; k++)
    {

        if (vett[k] == elem)
            return 1;
    }
    return 0;
}

int esiste_banchina_libera(Porto p)
{
    if (p->banchine_occupate < p->num_banchine)
        return 1;
    else
        return 0;
}

int porto_get_id(Porto p)
{
    return p->id;
}

/* Parametri di configurazione simulazione */
void load_config(char *argv[])
{
    SO_MERCI = atoi(argv[3]);
    SO_LOADSPEED = atoi(argv[11]);
    SO_FILL = atoi(argv[12]);
    SO_BANCHINE = atoi(argv[10]);
    SO_LATO = atoi(argv[7]);
    SO_PORTI = atoi(argv[2]);
    SO_MIN_VITA = atoi(argv[5]);
    SO_MAX_VITA = atoi(argv[6]);
}

/* Genera le coordinate date le dimensioni di un piano. */
Coordinates get_random_coordinates(int maxX, int maxY)
{

    Coordinates coordinates;
    srand(time(NULL));
    coordinates.x = (double)rand() / (RAND_MAX / maxX);
    coordinates.y = (double)rand() / (RAND_MAX / maxY);
    return coordinates;
}

/* Genera le coordinate dell'angolo date le dimensioni di un piano. */
Coordinates get_corner_coordinates(int max_X, int max_Y, int num_angolo)
{

    Coordinates coordinates;
    switch (num_angolo)
    {
    case 0:
        coordinates.x = 0;
        coordinates.y = 0;
        break;
    case 1:
        coordinates.x = max_X;
        coordinates.y = 0;
        break;
    case 2:
        coordinates.x = 0;
        coordinates.y = max_Y;
        break;
    case 3:
        coordinates.x = max_X;
        coordinates.y = max_Y;
        break;
    default: /* ERROR !!!  */
        coordinates.x = -1;
        coordinates.y = -1;
        break;
    }

    return coordinates;
}

/* ritorna 1 se il porto ha bisogno di una merce con determinato id (soddisfo la domanda di porto) */
int porto_ha_bisogno_di_id(Porto p, int id)
{
    int i;
    for (i = 0; i < p->domanda_N; i++)
    {
        if (p->domanda_id[i] == id)
            return 1;
    }
    return 0;
}
