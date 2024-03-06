#define _GNU_SOURCE

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/msg.h>
#include "nave.h"

#define DEFAULT_VAL -1

#define TIPO_MESSAGGIO 1

/* variabili globali di configurazione simulazione */
double SO_SPEED = DEFAULT_VAL;
int SO_CAPACITY = DEFAULT_VAL;
int SO_LATO = DEFAULT_VAL;
int SO_NAVI = DEFAULT_VAL;
int SO_PORTI = DEFAULT_VAL;
int SO_MERCI = DEFAULT_VAL;
int SO_SIZE = DEFAULT_VAL;
int SO_MIN_VITA = DEFAULT_VAL;
int SO_MAX_VITA = DEFAULT_VAL;
int SO_LOADSPEED = DEFAULT_VAL;

typedef struct Messaggio
{
    int idNave;
    int mType;
    Coordinates posizioneNave;
} Messaggio;

Nave new_nave(double px, double py, int id);
double point_distance(Nave N, Porto p);
double time_to_go(Nave N, double distance);
int nave_get_id(Nave N);
Coordinates get_random_coordinates(int max_X, int max_Y);
Lotto crea_lotto();
Porto get_porto_disponibile(Porto porti);
void carica_nave(Nave N, Porto p);
void scarica_nave(Nave N, Porto p);
void load_config();
Porto get_porto_richiesta(Porto porti, int merce);
void sleep_With_Decimal_Seconds(double seconds);
Porto get_porto_disponibile(Porto porti);
void safe_nanosleep(double seconds);
Porto cerca_porto_corrispondente(Porto porti, Porto porto_offerta, int indice_offerta);
void mescola_porti(Porto porti);
void invia_messaggio(int idNave, Coordinates posizioneNave);
void reportNave_debug(Nave navi);

int main(int argc, char *argv[])
{

#pragma region init_var
    /* init - var  */
    Nave n = NULL;
    Coordinates c;
    Lotto l = NULL;
    int i = 0;
    int j = 0;
    int k = 0;

    /* var - creazione_memoria_condivisa_per_avvio_e_terminazione_programma  */
    int shmid;
    int *shared_memory;
    key_t key;

    /* var - creazione_semafori_avvio_e_terminazione */
    sem_t *sem_memoria;

    /* var - creazione_memoria_condivisa_per_porti */
    int shared_mem_porto_id;
    size_t spazio_alloca;
    key_t shared_mem_porto_key;
    Porto shared_mem_porto;
    int num_porti;
    Porto p = NULL;
    sem_t *sem_porti;

    /* var - creazione_memoria_condivisa_per_navi */
    int shared_mem_nave_id;
    size_t spazio_alloca_nave;
    key_t shared_mem_nave_key;
    int num_nave;
    Nave shared_mem_nave;
    /* Nave *shared_mem_nave; */

    double distanza;
    double tempo_per_arrivare;
    int qt_merce;
    double tempo_scambio;

    int Continua; /* var Avvio - Terminazione */

    /* var - memoria_condivisa_per_lo_stato_delle_navi */
    sem_t *sem_stato_navi;
    key_t stato_navi_key;
    int stato_navi_shmid;
    int *stato_navi;

    int *in_mare_con_carico_ptr;
    int *in_mare_senza_carico_ptr;
    int *in_porto_ptr;

    /* Semaforo - sem_ports_initialized */
    sem_t *sem_ports_initialized;

    /* var - creazione_memoria_condivisa_per_navi */
    int shared_mem_navi_id;
    size_t spazio_alloca_navi;
    key_t shared_mem_navi_key;
    Nave *shared_mem_navi;
    int num_navi;

    sem_t *sem_nave;

    load_config(argv);
#pragma endregion

#pragma region sem_ports_initialized
    sem_ports_initialized = sem_open("ports_initialized", 0);
    if (sem_ports_initialized == SEM_FAILED)
    {
        perror("sem_open for ports_initialized nave");
        exit(1);
    }
    /* attende che tutti i porti siano pronti */
    for (i = 0; i < SO_PORTI; i++)
        sem_wait(sem_ports_initialized);

#pragma endregion

#pragma region creazione_semaforo_attesa_nave
    sem_nave = sem_open("sem_nave", 0);
    if (sem_nave == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
#pragma endregion

#pragma region crea_processi_navi
    for (i = 0; i < SO_NAVI; i++)
    {
        if (fork() == 0)
        {
            srand(getpid());
            c = get_random_coordinates(SO_LATO, SO_LATO);
            n = new_nave(c.x, c.y, i);

            n->lotti = new_lotto();
            if (n->lotti == NULL)
            {
                fprintf(stderr, "Errore durante l'allocazione del lotto per la nave %d\n", n->id);
                exit(1);
            }
            break;
        }
    }
    if (i == SO_NAVI)
        exit(0);
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
    shared_mem_porto_key = ftok("/etc", 1);
    num_porti = atoi(argv[2]);
    spazio_alloca = sizeof(struct porto) * num_porti;
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
#pragma endregion

#pragma region creazione_semafori_per_porti_conddivisi
    sem_porti = sem_open("porti", 0);
    if (sem_porti == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
#pragma endregion

#pragma region Creazione di memoria condivisa per lo stato delle navi
    /* Creazione di memoria condivisa per lo stato delle navi */
    stato_navi_key = ftok("/tmp", 66);
    stato_navi_shmid = shmget(stato_navi_key, 3 * sizeof(int), IPC_CREAT | 0666);
    if (stato_navi_shmid == -1)
    {
        perror("shmget per lo stato delle navi");
        exit(1);
    }

    stato_navi = shmat(stato_navi_shmid, NULL, 0);
    if (stato_navi == (int *)-1)
    {
        perror("shmat per lo stato delle navi");
        exit(1);
    }

    in_mare_con_carico_ptr = stato_navi;
    in_mare_senza_carico_ptr = stato_navi + 1;
    in_porto_ptr = stato_navi + 2;
#pragma endregion
#pragma region creazione_memoria_condivisa_per_navi

    /* carico navi in memoria condivisa */
    shared_mem_nave_key = ftok("/tmp", 20);
    spazio_alloca_nave = sizeof(struct nave) * SO_NAVI;
    shared_mem_nave_id = shmget(shared_mem_nave_key, spazio_alloca_nave, IPC_CREAT | 0666);
    if (shared_mem_nave_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    shared_mem_nave = (Nave)shmat(shared_mem_nave_id, NULL, 0);
    if (shared_mem_nave == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    shared_mem_nave[n->id].lotti = malloc(sizeof(struct lotto));
    if (shared_mem_nave[n->id].lotti == NULL)
    {
        perror("malloc");
        exit(1);
    }
    shared_mem_nave[n->id] = *n;

#pragma endregion

    sem_post(sem_nave);

    Continua = 1;
    while (Continua)
    {
        sem_wait(sem_porti);
        (*in_mare_senza_carico_ptr)++;
        p = get_porto_disponibile(shared_mem_porto);
        if (p == NULL)
        {
            /* printf("offerta terminata, non ho piu' merci da consegnare\n"); */
            sem_post(sem_porti);
            break;
        }
        else
        {
            /* printf("nave con id %d, va a porto con id %d\n", n->id, p->id); */
        }

        carica_nave(n, p); /* carico la nave prima di arrivare al porto in mopdo da diminuire gia l' oferta */
        /*  reportNave_debug(shared_mem_nave); */

        if (*in_mare_senza_carico_ptr > 0)
            (*in_mare_senza_carico_ptr)--;

        tempo_scambio = n->lotti->qt_merce / SO_LOADSPEED;

        (*in_porto_ptr)++;

        sem_post(sem_porti);

        sleep_With_Decimal_Seconds(tempo_scambio);

        sem_wait(sem_porti);
        if (*in_porto_ptr > 0)
            (*in_porto_ptr)--;
        if (p->banchine_occupate > 0)
        {
            p->banchine_occupate--;
        }
        sem_post(sem_porti);

        distanza = point_distance(n, p);
        tempo_per_arrivare = time_to_go(n, distanza);

        n->c.x = p->c.x;
        n->c.y = p->c.y;

        sem_wait(sem_porti);
        (*in_mare_con_carico_ptr)++;
        sem_post(sem_porti);

        sleep_With_Decimal_Seconds(tempo_per_arrivare);

        sem_wait(sem_porti);
        if (*in_mare_con_carico_ptr > 0)
            (*in_mare_con_carico_ptr)--;

        p = get_porto_richiesta(shared_mem_porto, n->lotti->id);
        invia_messaggio(n->id, n->c);
        if (p == NULL)
        {
            /*  printf("non c' e domanda per questa merce, sto fermo in mare\n");  */
            n->lotti->temp_vita = 0;
            n->lotti->id = 0;
            n->N_merce_scaduta_nave += n->lotti->qt_merce;
            n->lotti->qt_merce = 0;
            sem_post(sem_porti);
            continue;
        }
        else
        {
            /*  printf("nave con id %d, va a porto con id: %d e scarica merce id: %d  qt. %d  tmp vita: %d\n", n->id, p->id, n->lotti->id, n->lotti->qt_merce, n->lotti->temp_vita); */
        }
        qt_merce = n->lotti->qt_merce;

        scarica_nave(n, p);
        /*  reportNave_debug(shared_mem_nave); */

        n->c.x = p->c.x;
        n->c.y = p->c.y;

        sem_post(sem_porti);

        tempo_scambio = qt_merce / SO_LOADSPEED;

        sem_wait(sem_porti);
        (*in_porto_ptr)++;
        sem_post(sem_porti);

        safe_nanosleep(tempo_scambio);

        sem_wait(sem_porti);

        if (*in_porto_ptr > 0)
            (*in_porto_ptr)--;

        if (p->banchine_occupate > 0)
            p->banchine_occupate--;

        sem_post(sem_porti);

        sem_wait(sem_memoria);
        Continua = *shared_memory;
        sem_post(sem_memoria);
    }
    /* printf("PROCESSO NAVE FINE\n"); */

    return 0;
}

Nave new_nave(double px, double py, int id)
{
    Nave val = malloc(sizeof *val);
    val->c.x = px;
    val->c.y = py;
    val->id = id;
    val->N_merce_scaduta_nave = 0;
    /*  val->lotti = malloc(sizeof(struct lotto));
     if (val->lotti == NULL)
     {
         perror("malloc");
         exit(1);
     } */
    return val;
}

/*distanza tra una nave e un punto*/
double point_distance(Nave N, Porto P)
{
    double x1 = P->c.x;
    double y1 = P->c.y;
    return sqrt(pow(N->c.x - x1, 2) + pow(N->c.y - y1, 2));
}

/*tempo per andare data una nave e una distanza*/
double time_to_go(Nave N, double distance)
{
    return distance / (SO_SPEED);
}

/* Get id nave */
int nave_get_id(Nave N)
{
    return N->id;
}

/* Parametri di configurazione simulazione */
void load_config(char *argv[])
{
    SO_SPEED = atof(argv[8]);
    SO_CAPACITY = atoi(argv[9]);
    SO_LATO = atoi(argv[7]);
    SO_PORTI = atoi(argv[2]);
    SO_NAVI = atoi(argv[1]);
    SO_MERCI = atoi(argv[3]);
    SO_SIZE = atoi(argv[4]);
    SO_MIN_VITA = atoi(argv[5]);
    SO_MAX_VITA = atoi(argv[6]);
    SO_LOADSPEED = atoi(argv[11]);
}

/* Genera le coordinate date le dimensioni di un piano. */
Coordinates get_random_coordinates(int max_X, int max_Y)
{

    Coordinates coordinates;
    coordinates.x = (double)rand() / (RAND_MAX / max_X);
    coordinates.y = (double)rand() / (RAND_MAX / max_Y);
    return coordinates;
}

/* trova porto la cui offerta è una domanda di un'altro porto */
Porto get_porto_disponibile(Porto porti)
{
    int i, j;
    for (i = 0; i < SO_PORTI; i++)
    {
        if (i != porti->id && porti[i].offerta_N > 0)
        {
            for (j = 0; j < porti[i].offerta_N; j++)
            {
                Porto porto_corrispondente = cerca_porto_corrispondente(porti, &porti[i], j);
                if (porto_corrispondente != NULL)
                {
                    /*  printf("Trovato porto disponibile! Id: %d\n", porto_corrispondente->id); */
                    return porto_corrispondente;
                }
            }
        }
    }

    /* printf("Nessun porto disponibile trovato. Restituisco NULL.\n"); */
    return NULL;
}

Porto cerca_porto_corrispondente(Porto porti, Porto porto_offerta, int indice_offerta)
{
    int i, j;
    mescola_porti(porti);
    for (i = 0; i < SO_PORTI; i++)
    {
        if (i != porto_offerta->id && porti[i].domanda_N > 0)
        {
            for (j = 0; j < porti[i].domanda_N; j++)
            {
                if (porti[i].domanda_id[j] == porto_offerta->id_offerta[indice_offerta] &&
                    porti[i].domanda_qt[j] > 0)
                {
                    return &porti[i];
                }
            }
        }
    }
    return NULL;
}

/* Funzione ausiliare per shuffle porti */
void mescola_porti(Porto porti)
{
    int i;
    /* srand(getpid() ^ time(NULL)); */
    for (i = SO_PORTI - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        struct porto temp = porti[i];
        porti[i] = porti[j];
        porti[j] = temp;
    }
}

/* carica la nave in un porto di una merce casuale disponibile */
void carica_nave(Nave N, Porto p)
{
    int i;
    int j;
    p->banchine_occupate++;

    for (i = 0; i < p->offerta_N; i++)
    {
        if (p->qt_offerta[i] > 0 && p->temp_vita_offerta[i] > 0)
        {
            N->lotti->id = p->id_offerta[i];

            if (p->qt_offerta[i] > SO_CAPACITY)
            {
                N->lotti->qt_merce = SO_CAPACITY;
                p->qt_offerta[i] -= SO_CAPACITY;
                N->lotti->temp_vita = p->temp_vita_offerta[i];
            }
            else
            {
                N->lotti->qt_merce = p->qt_offerta[i];
                p->qt_offerta[i] = 0;
                N->lotti->temp_vita = p->temp_vita_offerta[i];

                for (j = i; j < p->offerta_N - 1; j++)
                {
                    p->id_offerta[j] = p->id_offerta[j + 1];
                    p->qt_offerta[j] = p->qt_offerta[j + 1];
                    p->temp_vita_offerta[j] = p->temp_vita_offerta[j + 1];
                }
                p->offerta_N--;
            }
            break;
        }
    }

    p->merce_spedita += N->lotti->qt_merce;
}

/* Scarica nave nel porto chr richiede */
void scarica_nave(Nave N, Porto p)
{

    int i;
    int j;

    p->banchine_occupate++;

    p->merce_ricevuta += N->lotti->qt_merce;

    for (i = 0; i < p->domanda_N; i++)
    {
        if (N->lotti->id == p->domanda_id[i] && N->lotti->temp_vita > 0)
        {
            if (N->lotti->qt_merce < p->domanda_qt[i])
            {
                p->domanda_qt[i] -= N->lotti->qt_merce;
                N->lotti->qt_merce = 0;
            }
            else
            {
                N->lotti->qt_merce -= p->domanda_qt[i];
                p->domanda_qt[i] = 0;

                for (j = i; j < p->domanda_N - 1; j++)
                {
                    p->domanda_id[j] = p->domanda_id[j + 1];
                    p->domanda_qt[j] = p->domanda_qt[j + 1];
                    p->temp_vita_offerta[j] = p->temp_vita_offerta[j + 1];
                }

                p->domanda_N--;
            }

            break;
        }
    }
}

/* Trova porto che richede merce di tipo specifico */
Porto get_porto_richiesta(Porto porti, int merce)
{
    int num_porti = SO_PORTI;
    int num_porti_richiesta = 0;
    int i;
    int j;

    for (i = 0; i < num_porti; i++)
    {
        if (porti[i].domanda_N > 0)
        {
            for (j = 0; j < porti[i].domanda_N; j++)
            {
                if (porti[i].domanda_id[j] == merce && porti[i].domanda_qt[j] > 0)
                {
                    num_porti_richiesta++;
                    break;
                }
            }
        }
    }

    if (num_porti_richiesta > 0)
    {
        int indice_casuale = rand() % num_porti_richiesta;

        int porti_trovati = 0;
        for (i = 0; i < num_porti; i++)
        {
            if (porti[i].domanda_N > 0)
            {
                for (j = 0; j < porti[i].domanda_N; j++)
                {
                    if (porti[i].domanda_id[j] == merce && porti[i].domanda_qt[j] > 0)
                    {
                        if (porti_trovati == indice_casuale)
                        {
                            return &porti[i];
                        }
                        porti_trovati++;
                        break;
                    }
                }
            }
        }
    }
    return NULL;
}

/* nanosleep decimal */
void sleep_With_Decimal_Seconds(double seconds)
{
    struct timespec sleepTime;

    double intPart;
    double fracPart = modf(seconds, &intPart);

    sleepTime.tv_sec = (time_t)intPart;
    sleepTime.tv_nsec = (long)(fracPart * 1e9);

    if (nanosleep(&sleepTime, NULL) == -1)
    {
        perror("nanosleep");
    }
}

/* nanosleep sicura per segnali */
void safe_nanosleep(double seconds)
{
    struct timespec requested_time;
    struct timespec remaining_time;

    double intPart;
    double fracPart = modf(seconds, &intPart);

    requested_time.tv_sec = (time_t)intPart;
    requested_time.tv_nsec = (long)(fracPart * 1e9);

    while (nanosleep(&requested_time, &remaining_time) == -1)
    {
        if (errno == EINTR)
        {
            requested_time = remaining_time;
        }
        else
        {
            perror("nanosleep");
            break;
        }
    }
}

/* invio msg con id e cordinate Nave */
void invia_messaggio(int idNave, Coordinates posizioneNave)
{
    struct Messaggio msg;
    struct msqid_ds buf;
    int idCodaMessaggi;
    key_t chiave = ftok("/srv", 90);
    idCodaMessaggi = msgget(chiave, 0666 | IPC_CREAT);
    if (idCodaMessaggi == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    /* Ottengo lo stato attuale della coda */
    msgctl(idCodaMessaggi, IPC_STAT, &buf);

    /* Controlla se la coda può accettare ulteriori messaggi */
    if (buf.msg_qnum < buf.msg_qbytes / sizeof(struct Messaggio))
    {
        msg.mType = TIPO_MESSAGGIO;
        msg.idNave = idNave;
        msg.posizioneNave = posizioneNave;

        if (msgsnd(idCodaMessaggi, &msg, sizeof(struct Messaggio), IPC_NOWAIT) == -1)
        {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Messaggio inviato: idNave = %d, posizioneNave = (%f, %f)\n", msg.idNave, msg.posizioneNave.x, msg.posizioneNave.y);
        }
    }
    else
    {
        printf("La coda dei messaggi è piena. Impossibile inviare il messaggio.\n");
    }
}

/* Stampa a video info nave */
void reportNave_debug(Nave navi)
{
    int i;
    printf("reportNave_debug: \n");
    for (i = 0; i < SO_NAVI; i++)
    {
        printf("Nave ID %d trasporta:\n", navi[i].id);
        if (navi[i].lotti != NULL)
        {
            printf("  Tipo Merce: %d\n", navi[i].lotti->id);
            printf("  Quantità: %d\n", navi[i].lotti->qt_merce);
            printf("  Vita Merce: %d\n", navi[i].lotti->temp_vita);

            if (navi[i].lotti->temp_vita > 0)
            {
                navi[i].lotti->temp_vita--;
            }
            if (navi[i].lotti->temp_vita == 0)
            {
                navi[i].N_merce_scaduta_nave += navi[i].lotti->qt_merce;
            }
            printf("Qtà. Merce scaduta in nave id %d: qtà %d\n", navi[i].id, navi[i].N_merce_scaduta_nave);
        }
        else
        {
            printf("  Nave vuota (senza merce)\n");
        }

        printf("\n");
    }
}