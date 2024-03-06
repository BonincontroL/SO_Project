#define _GNU_SOURCE

#include "nave.h"
#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

/*file configurazione specifica progetto*/
#define CONFIG_FILE "src/config.txt"
#define TIPO_MESSAGGIO 1
#define N_PARAM 14

#pragma region var
/* init var  */
int i, j, k;
int giorno;

/* var - creazione_memoria_condivisa_per_avvio_e_terminazione_programma */

key_t key;
int *shared_memory;

/* var - creazione_semafori_avvio_e_terminazione */
struct timespec ts = {1, 0};
int sem_N;
sem_t *sem_memoria;
sem_t *sem_porti;
int shmid;

/* var - creazione_memoria_condivisa_per_porti */
int shared_mem_porto_id;
size_t spazio_alloca;
key_t shared_mem_porto_key;
int num_porti;
Porto shared_mem_porto;

/* var - creazione_memoria_condivisa_per_navi */
int shared_mem_nave_id;
size_t spazio_alloca_nave;
key_t shared_mem_nave_key;
int num_nave;
Nave shared_mem_nave;

/* var - creazione_sem_ports_initialized */
sem_t *sem_ports_initialized;

/* var - memoria_condivisa_per_lo_stato_delle_navi */
key_t stato_navi_key;
int stato_navi_shmid;
int *stato_navi;

/* var - stato nave */
int *in_mare_con_carico_ptr;
int *in_mare_senza_carico_ptr;
int *in_porto_ptr;

sem_t *sem_nave;

#pragma endregion

/* array con parametri di configurazione letti da file */
/* 0. / | 1. SO_NAVI | 2. SO_PORTI | 3. SO_MERCI | 4. SO_SIZE | 5. SO_MIN_VITA | 6. SO_MAX_VITA | 7. SO_LATO
   8. SO_SPEED | 9. SO_CAPACITY | 10. SO_BANCHINE | 11. SO_LOADSPEED | 12. SO_FILL | 13. SO_DAYS | 14. NULL  */
char *param_arr[N_PARAM];

int idCodaMessaggi;
typedef struct Messaggio
{
    int idNave;
    int mType;
    Coordinates posizioneNave;
} Messaggio;

void load_Config();
void execute_programs();
void Stampa_Report();
void safe_nanosleep(struct timespec *requested_time);
void stampa_parametri_config(char *param_arr[N_PARAM]);
Porto trovaPortoOffertaMassima(Porto porti);
void stampaPortoOffertaMassima(Porto porti);
Porto trovaPortoRichiestaMassima(Porto porti);
void stampaPortoRichiestaMassima(Porto porti);
void stampaPortiOffertaERichiestaMassima(Porto porti);
Porto trovaPortoRichiestaMassimaPerMerce(Porto porti, int tipoMerce);
Porto trovaPortoOffertaMassimaPerMerce(Porto porti, int tipoMerce);
void stampaInformazioniMerce(Porto porti, Nave navi);
int calcolaQuantitaTotalePerTipo(Porto porti, Nave navi, int tipoMerce);
void stampaPortiOffertaERichiestaMassima(Porto porti);
void reportNave(Nave navi);
void statoNave(int *in_mare_con_carico_ptr, int *in_mare_senza_carico_ptr, int *in_porto_ptr);
void reportPorto(Porto porti);
void ricevi_messaggio();
void signal_handler(int signo);

int main(int argc, char *argv[])
{

#pragma region controlli_sig
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, signal_handler) == SIG_ERR)
    {
        perror("signal");
        exit(EXIT_FAILURE);
    }
#pragma endregion

#pragma region init_var

    load_Config();

#pragma endregion

#pragma region creazione_memoria_condivisa_per_avvio_e_terminazione_programma
    /* creo memoria condivisa per avvio */
    key = ftok("/tmp", 65);
    shmid = shmget(key, sizeof(int), IPC_CREAT | 0666);
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
    *shared_memory = 1;
#pragma endregion

#pragma region creazione_semafori_avvio_e_terminazione
    /* creo semafori */
    sem_N = atoi(param_arr[1]) + atoi(param_arr[2]);

    sem_memoria = sem_open("/avvio", O_CREAT, 0666, sem_N); /* setto il semaforo a SO_PORTI + SO_NAVI */
    if (sem_memoria == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
#pragma endregion

#pragma region creazione_semaforo_attesa_nave
    sem_nave = sem_open("sem_nave", O_CREAT, 0666, 0);
    if (sem_nave == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
#pragma endregion

#pragma region creazione_memoria_condivisa_per_porti
    shared_mem_porto_key = ftok("/etc", 1);
    num_porti = atoi(param_arr[2]);
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

#pragma region creazione_memoria_condivisa_per_navi

    shared_mem_nave_key = ftok("/tmp", 20);
    num_nave = atoi(param_arr[1]);
    spazio_alloca_nave = sizeof(struct nave) * num_nave;
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
#pragma endregion

#pragma region creazione_semafori_per_porti_conddivisi
    sem_porti = sem_open("porti", O_CREAT, 0666, 1);
    if (sem_porti == SEM_FAILED)
    {
        perror("sem_open");
        exit(1);
    }
#pragma endregion

#pragma region sem_ports_initialized
    sem_ports_initialized = sem_open("ports_initialized", O_CREAT, 0666, 0);
    if (sem_ports_initialized == SEM_FAILED)
    {
        perror("sem_open for ports_initialized master");
        exit(1);
    }
#pragma endregion

#pragma region memoria_condivisa_per_lo_stato_delle_navi
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

#pragma region avvio_simulazione_e_report_days

    /* avvio simulazione */
    execute_programs();

    /* attende che tutte le navi siano pronte */
    for (i = 0; i < atoi(param_arr[1]); i++)
    {
        sem_wait(sem_nave);
    }

    for (giorno = 0; giorno < atoi(param_arr[13]); giorno++)
    {
        nanosleep(&ts, NULL);
        printf("\n");
        printf("---------------------------------------------\n");
        printf("Giorno: %d\n", giorno + 1);
        printf("---------------------------------------------\n");
        printf("\n");
        sem_wait(sem_porti);
        statoNave(in_mare_con_carico_ptr, in_mare_senza_carico_ptr, in_porto_ptr);
        printf("\n");
        reportPorto(shared_mem_porto);
        reportNave(shared_mem_nave);
        stampaInformazioniMerce(shared_mem_porto, shared_mem_nave);
        for (i = 0; i < atoi(param_arr[1]); i++)
        {
            ricevi_messaggio();
        }

        sem_post(sem_porti);
    }

#pragma endregion

#pragma region terminazione
    /* termino simulazione */
    for (i = 0; i < sem_N; i++)
    {
        sem_wait(sem_memoria);
    }
    *shared_memory = 0;
    for (i = 0; i < sem_N; i++)
    {
        sem_post(sem_memoria);
    }
#pragma endregion

#pragma region report_finale

    printf("\n");
    printf("---------------------------------------------\n");
    printf("REPORT FINALE!\n");
    printf("---------------------------------------------\n");
    printf("\n");

    sem_wait(sem_porti);

    statoNave(in_mare_con_carico_ptr, in_mare_senza_carico_ptr, in_porto_ptr);
    printf("\n");
    reportPorto(shared_mem_porto);
    reportNave(shared_mem_nave);
    stampaPortoOffertaMassima(shared_mem_porto);
    stampaPortoRichiestaMassima(shared_mem_porto);
    stampaPortiOffertaERichiestaMassima(shared_mem_porto);
    stampaInformazioniMerce(shared_mem_porto, shared_mem_nave);

    sem_post(sem_porti);

#pragma endregion

#pragma region terminazione_e_rimozione_risorse_IPC
    sem_close(sem_memoria);
    sem_close(sem_porti);
    sem_close(sem_nave);
    sem_close(sem_ports_initialized);
    sem_unlink("avvio");
    sem_unlink("porti");
    sem_unlink("sem_nave");
    sem_unlink("ports_initialized");
    shmdt(shared_mem_porto);
    shmdt(shared_memory);
    shmdt(shared_mem_nave);
    shmdt(stato_navi);

    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shared_mem_porto_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shared_mem_nave_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    if (shmctl(stato_navi_shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    if (msgctl(idCodaMessaggi, IPC_RMID, NULL) == -1)
    {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
    printf("Simulazione programma finito :( !\n");

#pragma endregion

    return 0;
}

/* Caricameneto parametri configurazione letti da file */
void load_Config()
{
    /*exec*/

    /*Variabili per lettura da file*/
    char line[256];
    char *name;
    char *value;

    /*indice param param_arr[i] letti da file*/
    int param_index;

    int i;

    FILE *config_file = fopen(CONFIG_FILE, "r");

    if (config_file == NULL)
    {
        perror("Errore nell'apertura del file di configurazione!");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < N_PARAM; i++)
    {
        param_arr[i] = (char *)malloc(20);
        param_arr[i][0] = '\0';
    }
    param_arr[N_PARAM] = NULL;
    while (fgets(line, sizeof(line), config_file) != NULL)
    {
        name = strtok(line, "=");
        value = strtok(NULL, "=");
        if (name != NULL && value != NULL)
        {
            param_index = -1;
            if (strcmp(name, "SO_NAVI") == 0)
                param_index = 1;
            else if (strcmp(name, "SO_PORTI") == 0)
                param_index = 2;
            else if (strcmp(name, "SO_MERCI") == 0)
                param_index = 3;
            else if (strcmp(name, "SO_SIZE") == 0)
                param_index = 4;
            else if (strcmp(name, "SO_MIN_VITA") == 0)
                param_index = 5;
            else if (strcmp(name, "SO_MAX_VITA") == 0)
                param_index = 6;
            else if (strcmp(name, "SO_LATO") == 0)
                param_index = 7;
            else if (strcmp(name, "SO_SPEED") == 0)
                param_index = 8;
            else if (strcmp(name, "SO_CAPACITY") == 0)
                param_index = 9;
            else if (strcmp(name, "SO_BANCHINE") == 0)
                param_index = 10;
            else if (strcmp(name, "SO_LOADSPEED") == 0)
                param_index = 11;
            else if (strcmp(name, "SO_FILL") == 0)
                param_index = 12;
            else if (strcmp(name, "SO_DAYS") == 0)
                param_index = 13;

            if (param_index == 1 && atoi(value) < 1)
            {
                printf("Errore: SO_NAVI deve essere maggiore di 1!\n");
                exit(EXIT_FAILURE);
            }
            if (param_index == 2 && atoi(value) < 4)
            {
                printf("Errore: SO_PORTI deve essere maggiore di 4!\n");
                exit(EXIT_FAILURE);
            }

            if (param_index != -1)
            {
                snprintf(param_arr[param_index], 20, "%s", value);
            }
            else
            {
                printf("Errore nel file di configurazione: parametro sconosciuto!\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(config_file);
}
/* avvio programma con execve */
void execute_programs()
{
    char *executables[] = {"./bin/nave.out", "./bin/porto.out"};
    int i;
    for (i = 0; i < 2; i++)
    {
        pid_t child_pid = fork(); /* Creare un nuovo processo figlio */

        if (child_pid == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) /* Questo è il processo figlio */
        {

            if (execve(executables[i], param_arr, NULL) == -1)
            {
                perror("execve");
                exit(EXIT_FAILURE);
            }
        }
        else /* Questo è il processo padre, aspetta il completamento del figlio */
        {
            /* int status;
            if (wait(&status) == -1)
            {
                perror("wait");
                exit(EXIT_FAILURE);
            } */
        }
    }
}

/* nanosleep sicura per segnali */
void safe_nanosleep(struct timespec *requested_time)
{
    struct timespec remaining_time;
    while (nanosleep(requested_time, &remaining_time) == -1)
    {
        if (errno == EINTR)
        {
            *requested_time = remaining_time;
        }
        else
        {
            perror("nanosleep");
            break;
        }
    }
}

/* Funzione per trovare il porto con la richiesta massima di merce */
Porto trovaPortoOffertaMassima(Porto porti)
{
    Porto portoMassimo = &porti[0];
    int i;
    for (i = 0; i < atoi(param_arr[2]); ++i)
    {
        if (porti[i].offerta_N > portoMassimo->offerta_N)
        {
            portoMassimo = &porti[i];
        }
    }

    return portoMassimo;
}

/* Funzione per trovare il porto con la offerta massima di merce */
void stampaPortoOffertaMassima(Porto porti)
{
    Porto portoOffertaMassima = trovaPortoOffertaMassima(porti);

    printf("Il porto con l'offerta massima è il Porto %d\n",
           portoOffertaMassima->id);
}

/* Funzione per trovare il porto che ha richiesto la quantità massima di un tipo di merce */
Porto trovaPortoRichiestaMassima(Porto porti)
{
    Porto portoMassimo = &porti[0];
    int i;
    for (i = 0; i < atoi(param_arr[2]); ++i)
    {
        if (porti[i].domanda_N > portoMassimo->domanda_N)
        {
            portoMassimo = &porti[i];
        }
    }

    return portoMassimo;
}

/* Funzione per stampare il porto con la richiesta massima di merce */
void stampaPortoRichiestaMassima(Porto porti)
{
    Porto portoRichiestaMassima = trovaPortoRichiestaMassima(porti);

    printf("Il porto con la richiesta massima è il Porto %d\n",
           portoRichiestaMassima->id);
}

/* Funzione per trovare il porto che ha offerto la quantità massima di un tipo di merce */
Porto trovaPortoOffertaMassimaPerMerce(Porto porti, int tipoMerce)
{
    Porto portoMassimo = &porti[0];

    int i;
    for (i = 0; i < atoi(param_arr[2]); i++)
    {
        if (porti[i].qt_offerta[tipoMerce] > portoMassimo->qt_offerta[tipoMerce])
        {
            portoMassimo = &porti[i];
        }
    }

    return portoMassimo;
}

Porto trovaPortoRichiestaMassimaPerMerce(Porto porti, int tipoMerce)
{
    Porto portoMassimo = &porti[0];
    int i;
    for (i = 0; i < atoi(param_arr[2]); i++)
    {
        if (porti[i].domanda_qt[tipoMerce] > portoMassimo->domanda_qt[tipoMerce])
        {
            portoMassimo = &porti[i];
        }
    }

    return portoMassimo;
}

/* Funzione per stampare i porti che hanno offerto e richiesto la quantità massima di ciascun tipo di merce */
void stampaPortiOffertaERichiestaMassima(Porto porti)
{
    int tipoMerce;
    for (tipoMerce = 0; tipoMerce < atoi(param_arr[3]); tipoMerce++)
    {
        Porto portoOffertaMassima = trovaPortoOffertaMassimaPerMerce(porti, tipoMerce);
        Porto portoRichiestaMassima = trovaPortoRichiestaMassimaPerMerce(porti, tipoMerce);

        printf("Tipo Merce %d:\n", tipoMerce);
        printf("Porto con offerta massima: Porto %d con %d unità di merce offerta.\n",
               portoOffertaMassima->id, portoOffertaMassima->qt_offerta[tipoMerce]);

        printf("Porto con richiesta massima: Porto %d con %d unità di merce richiesta.\n",
               portoRichiestaMassima->id, portoRichiestaMassima->domanda_qt[tipoMerce]);

        printf("\n");
    }
}

/* Funzione per calcolare la quantità totale di merce per ogni tipo */
int calcolaQuantitaTotalePerTipo(Porto porti, Nave navi, int tipoMerce)
{
    int quantitaTotale = 0;
    int i;

    for (i = 0; i < atoi(param_arr[2]); i++)
    {
        quantitaTotale += porti[i].qt_offerta[tipoMerce];
        quantitaTotale += porti[i].domanda_qt[tipoMerce];
    }

    for (i = 0; i < atoi(param_arr[1]); i++)
    {
        quantitaTotale += navi[i].lotti->qt_merce;
    }

    return quantitaTotale;
}

/* Funzione per calcolare e stampare le informazioni sulla merce per ogni tipo */
void stampaInformazioniMerce(Porto porti, Nave navi)
{
    int i;
    int tipoMerce;
    int quantitaFermaInPorto = 0;
    int quantitaScadutaInPorto = 0;
    int quantitaScadutaInNave = 0;
    int quantitaConsegnataDaNave = 0;
    int quantitaTotale;
    for (tipoMerce = 0; tipoMerce < atoi(param_arr[3]); tipoMerce++)
    {
        quantitaTotale = calcolaQuantitaTotalePerTipo(porti, navi, tipoMerce);
        quantitaFermaInPorto = 0;
        quantitaScadutaInPorto = 0;
        quantitaScadutaInNave = 0;
        quantitaConsegnataDaNave = 0;

        for (i = 0; i < atoi(param_arr[2]); i++)
        {
            quantitaFermaInPorto += porti[i].qt_offerta[tipoMerce];
            quantitaScadutaInPorto += porti[i].N_merce_scaduta_porto;
        }

        for (i = 0; i < atoi(param_arr[1]); i++)
        {
            quantitaScadutaInNave += navi[i].N_merce_scaduta_nave;
            quantitaConsegnataDaNave += navi[i].lotti->qt_merce;
        }

        printf("Tipo Merce %d:\n", tipoMerce);
        printf("Quantità totale generata: %d\n", quantitaTotale);
        printf("Quantità rimasta ferma in porto: %d\n", quantitaFermaInPorto);
        printf("Quantità scaduta in porto: %d\n", quantitaScadutaInPorto);
        printf("Quantità scaduta in nave: %d\n", quantitaScadutaInNave);
        printf("Quantità consegnata da qualche nave: %d\n", quantitaConsegnataDaNave);
        printf("\n");
    }
}

/* stampa cosa trasporta ogni nave, */
void reportNave(Nave navi)
{
    int i;
    for (i = 0; i < atoi(param_arr[1]); i++)
    {
        printf("Nave ID %d trasporta:\n", navi[i].id);
        navi[i].lotti = malloc(sizeof(struct lotto));
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
        /* free(navi[i].lotti); */
    }
}

/* info STATO nave */
void statoNave(int *in_mare_con_carico_ptr, int *in_mare_senza_carico_ptr, int *in_porto_ptr)
{
    printf("Navi in mare con carico: %d\n", *in_mare_con_carico_ptr);
    printf("Navi in mare senza carico: %d\n", *in_mare_senza_carico_ptr);
    printf("Navi in porto: %d\n", *in_porto_ptr);

    (*in_mare_con_carico_ptr) = 0;
    (*in_mare_senza_carico_ptr) = 0;
    (*in_porto_ptr) = 0;
}

/* Stampa info dei porti */
void reportPorto(Porto porti)
{
    int portoIndex, offertaIndex, domandaIndex;
    for (portoIndex = 0; portoIndex < atoi(param_arr[2]); portoIndex++)
    {

        printf("Report Porto: %d\n", portoIndex);
        printf("Qtà. Merce spedita: %d\n", porti[portoIndex].merce_spedita);
        printf("Qtà. Merce ricevuta: %d\n", porti[portoIndex].merce_ricevuta);

        /* porti[portoIndex].merce_spedita = 0;
        porti[portoIndex].merce_ricevuta = 0; */

        for (offertaIndex = 0; offertaIndex < porti[portoIndex].offerta_N; offertaIndex++)
        {

            if (porti[portoIndex].temp_vita_offerta[offertaIndex] > 0)
                porti[portoIndex].temp_vita_offerta[offertaIndex]--;
            if (porti[portoIndex].temp_vita_offerta[offertaIndex] == 0)
            {
                if (porti[portoIndex].qt_offerta[offertaIndex] > 0)
                    porti[portoIndex].N_merce_scaduta_porto += porti[portoIndex].qt_offerta[offertaIndex];
                porti[portoIndex].temp_vita_offerta[offertaIndex] = 0;
                porti[portoIndex].qt_offerta[offertaIndex] = 0;
            }

            printf("Qtà. Merce presente: %d di tipo: %d tmp vita: %d\n",
                   porti[portoIndex].qt_offerta[offertaIndex],
                   porti[portoIndex].id_offerta[offertaIndex],
                   porti[portoIndex].temp_vita_offerta[offertaIndex]);
            if (porti[portoIndex].temp_vita_offerta[offertaIndex] == 0)
            {
                printf("Merce id %d Scaduto in Porto\n", porti[portoIndex].id_offerta[offertaIndex]);
            }
        }

        printf("qtà. merce scaduta: %d\n", porti[portoIndex].N_merce_scaduta_porto);

        /*  porti[portoIndex].N_merce_scaduta_porto = 0; */

        for (domandaIndex = 0; domandaIndex < porti[portoIndex].domanda_N; domandaIndex++)
        {

            printf("Qtà. Merce richiesta: %d di tipo: %d\n",
                   porti[portoIndex].domanda_qt[domandaIndex],
                   porti[portoIndex].domanda_id[domandaIndex]);
        }

        printf("Banchine occupate: %d | Banchine totali: %d\n",
               porti[portoIndex].banchine_occupate,
               porti[portoIndex].num_banchine);
    }
}

/* stampa a video i parametri di configurazione */
void stampa_parametri_config(char *param_arr[N_PARAM])
{
    int i;
    for (i = 0; i < N_PARAM; i++)
    {
        printf("%s", param_arr[i]);
    }
}
void ricevi_messaggio()
{
    struct Messaggio msg;
    key_t chiave = ftok("/srv", 90);
    idCodaMessaggi = msgget(chiave, 0666 | IPC_CREAT);

    if (idCodaMessaggi == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Coda dei messaggi aperta con successo. ID: %d\n", idCodaMessaggi);

    if (msgrcv(idCodaMessaggi, &msg, sizeof(struct Messaggio), TIPO_MESSAGGIO, IPC_NOWAIT) == -1)
    {
        if (errno == ENOMSG)
        {
            printf("Nessun messaggio presente nella coda.\n");
        }
        else
        {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (msg.mType == TIPO_MESSAGGIO)
        {
            printf("Messaggio ricevuto: idNave = %d, posizioneNave = (%f, %f)\n", msg.idNave, msg.posizioneNave.x, msg.posizioneNave.y);
        }
        else
        {
            printf("Messaggio di tipo sconosciuto nella coda.\n");
        }
    }
}

/* gestore signal */
void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGSEGV)
    {
        sem_close(sem_memoria);
        sem_close(sem_porti);
        sem_close(sem_nave);
        sem_close(sem_ports_initialized);
        sem_unlink("avvio");
        sem_unlink("porti");
        sem_unlink("sem_nave");
        sem_unlink("ports_initialized");
        shmdt(shared_mem_porto);
        shmdt(shared_memory);
        shmdt(shared_mem_nave);
        shmdt(stato_navi);

        if (shmctl(shmid, IPC_RMID, NULL) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        if (shmctl(shared_mem_porto_id, IPC_RMID, NULL) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        if (shmctl(shared_mem_nave_id, IPC_RMID, NULL) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        if (shmctl(stato_navi_shmid, IPC_RMID, NULL) == -1)
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        /* if (msgctl(idCodaMessaggi, IPC_RMID, NULL) == -1)
        {
            perror("msgctl");
            exit(EXIT_FAILURE);
        } */

        system("pkill nave.out");
        system("pkill porto.out");
        system("pkill master.out");

        printf("\nCleanuup complete. Exiting...\n");
        exit(EXIT_SUCCESS);
    }
}