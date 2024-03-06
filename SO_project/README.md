# Documentazione del progetto di Sistemi operativi - "Transito navale di merci"

## Introduzione
Questo progetto, sviluppato per il corso di Sistemi Operativi nell'anno accademico 2022/2023, simula un sistema di gestione di navi e porti.

### Configurazione di Sviluppo
- Sistema Operativo: Ubuntu su Windows Subsystem for Linux (WSL) {Versione WSL: 2.0.9.0, con Windows 11}

## Processo Master - master.c
Il processo `master`, attraverso `execve`, avvia i processi `Nave` e `Porto`. Lancia due eseguibili separati specificati dai percorsi "./bin/nave.out" e "./bin/porto.out". In breve, il processo master crea due processi figli, ognuno eseguendo uno dei programmi specificati. Se si verifica un errore durante l'esecuzione, i processi figli terminano con un messaggio di errore.

### Fasi Principali del Programma - master.c

1. **Inizializzazione delle Variabili:** Vengono dichiarate e inizializzate varie variabili utilizzate durante l'esecuzione del programma.
2. **Lettura dei Parametri di Configurazione:** Viene chiamata la funzione `load_Config()` per leggere i parametri di configurazione da un file ("src/config.txt").
3. **Creazione delle Risorse IPC (Inter-Process Communication):** Vengono create varie risorse IPC, come memoria condivisa, semafori e code dei messaggi, per la comunicazione tra processi.
4. **Avvio dei Processi Nave e Porto:** Viene chiamata la funzione `execute_programs()` per eseguire i programmi relativi alle navi e ai porti. La funzione utilizza `fork()` per creare processi figli che eseguiranno i programmi specificati.
5. **Simulazione e Report Giornaliero:** Viene eseguita la simulazione giorno per giorno. Vengono stampati report sullo stato delle navi, dei porti e sulle merci presenti. Vengono utilizzate diverse funzioni ausiliarie per trovare e stampare informazioni specifiche.
6. **Terminazione della Simulazione e Report Finale:** Dopo la simulazione dei giorni specificati, il programma termina la simulazione e stampa un report finale che include statistiche complessive sull'intero periodo.
7. **Terminazione e Rimozione delle Risorse IPC:** Vengono chiusi e rimossi i semafori e la memoria condivisa. Questa fase garantisce la pulizia delle risorse IPC create durante l'esecuzione del programma.

>**N.B. vengono utilizzati i semafori POSIX della libreria <``semaphore.h``>** 
_________________________________________________________________________________

### Semafori POSIX (`<semaphore.h>`):

L'header file `<semaphore.h>` fa parte dell'API dei semafori POSIX, una serie di standard di interfaccia di programmazione per i sistemi operativi basati su UNIX. Le funzioni e le strutture dati fornite in `<semaphore.h>` sono progettate conformemente agli standard POSIX, garantendo una maggiore portabilità tra i vari sistemi operativi che aderiscono a tali standard.

Alcune delle funzioni comuni presenti in `<semaphore.h>` includono:

- `sem_open`
- `sem_close`
- `sem_wait`
- `sem_post`

### System V Semaphore (`<sys/sem.h>`):

L'header file `<sys/sem.h>` fa parte dell'API dei semafori di System V, un sistema di interfaccia di programmazione utilizzato in alcuni sistemi UNIX. Le funzioni e le strutture dati definite in `<sys/sem.h>` seguono lo standard System V Semaphore API, il quale può variare tra i diversi sistemi operativi UNIX.

Alcune delle funzioni comuni presenti in `<sys/sem.h>` includono:

- `semget`
- `semctl`
- `semop`
_________________________________________________________________________________


## Processo Nave - nave.c

```c 
typedef struct
{
  double x;
  double y;
} Coordinates;

struct nave
{
    Coordinates c;
    int id;
    Lotto lotti;

    int N_merce_scaduta_nave;

};

typedef struct nave *Nave;

#pragma region crea_processi_navi
    for (i = 0; i < SO_NAVI; i++)
    {
        if (fork() == 0)
        {
            srand(getpid());
            c = get_random_coordinates(SO_LATO, SO_LATO);
            n = new_nave(c.x, c.y, i);

            l = new_lotto();
            n->lotti = l;
            break;
        }
    }
    if (i == SO_NAVI)
        exit(0);
#pragma endregion

```

### Funzione Principale - main nave
Il main inizia con l'inizializzazione di variabili, semafori e memoria condivisa. La simulazione avvia poi un certo numero di processi figli (navi) con una `fork()`. I processi figli eseguono il corpo principale della simulazione, coinvolgendo la gestione di semafori, la creazione e l'utilizzo di memoria condivisa, la navigazione tra i porti, il caricamento e lo scaricamento delle merci, e l'invio di messaggi tramite code dei messaggi IPC.

1. **Inizializzazione delle Variabili e delle Strutture Dati Condivise:** Vengono dichiarate e inizializzate variabili e strutture dati per la simulazione, inclusi semafori, memoria condivisa per porti e navi, variabili di stato delle navi, ecc.
2. **Creazione dei Processi Navi:** Viene utilizzata una `fork` per creare processi figli per le navi. Ogni nave è rappresentata da un processo figlio, e ogni processo figlio inizierà l'esecuzione dalla riga `if (fork() == 0)`.
3. **Utilizzo di Semafori:** Vengono utilizzati semafori per la sincronizzazione tra i processi. Ad esempio, il semaforo `sem_ports_initialized` è utilizzato per garantire che tutti i porti siano pronti prima che le navi inizino a operare.
4. **Creazione e Utilizzo di Memoria Condivisa:** Viene utilizzata la memoria condivisa per condividere dati tra i processi. Ad esempio, la memoria condivisa viene utilizzata per memorizzare le informazioni dei porti, delle navi e altre informazioni utili alla simulazione.
5. **Ciclo di Simulazione:** La simulazione avviene all'interno di un ciclo while (con condizione `continua` ) principale. Le navi si muovono tra i porti, carico e scaricano merci, e gestiscono gli scambi commerciali.
`La condizione del ciclo while` verrà modifica attraverso l'utilizzo della memoria condivisa e semafori per sincronizzare: 
```c
        sem_wait(sem_memoria);
        Continua = *shared_memory;
        sem_post(sem_memoria);
```
## Processo Porto - porto.c

```c
struct porto
{
  Coordinates c;
  int id;
  int banchine_occupate;
  int num_banchine;

  int N_merce_scaduta_porto;

  int merce_spedita;
  int merce_ricevuta;

  int id_offerta[10000];
  int qt_offerta[10000];
  int temp_vita_offerta[10000];

  int offerta_N;
  int domanda_N;

  int domanda_id[10000];
  int domanda_qt[10000];
};
typedef struct porto *Porto;

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
#pragma endregion
```

### Funzione Principale - main porto
Il main Inizializza variabili e strutture dati, crea processi per i porti, utilizza memoria condivisa e semafori per la sincronizzazione tra i processi dei porti, carica la configurazione della simulazione, e gestisce la terminazione del programma.

In sintesi, questo porto.c crea un certo numero di processi, ognuno dei quali simula un porto. I processi figli vengono generati all'interno di un ciclo for. Ogni processo figlio genera le sue coordinate e crea un porto con queste coordinate e "l'ID" del processo.

## Lotto
```c
typedef struct lotto *Lotto;

struct lotto
{
    int id;          /* id merce */
    int qt_merce;
    int temp_vita;
};
```
Questa struttura rappresenta un "lotto" di merci con tre attributi: id (identificatore univoco del lotto,tipo merce), qt_merce (quantità di merci nel lotto), e temp_vita (tempo di vita residuo delle merci nel lotto).

Il file Lotto.c fornisce un insieme di funzioni per la creazione, l'accesso e la modifica degli ""oggetti"" Lotto.


## Compilazione e Esecuzione

Il progetto è composto anche da un Makefile per semplificare il processo di sviluppo e per permettere la compilazione e l'esecuzione del codice. Di seguito sono elencati i comandi principali:

- `make all`: Compila tutti i programmi (`master`, `nave`, `porto`).
- `make master`: Compila ed esegue il programma `master`, avviando la simulazione e generando un report in `out.txt`.
- `make nave`: Compila il programma `nave`.
- `make porto`: Compila il programma `porto`.
- `make clear`: Pulisce l'ambiente rimuovendo le directory `bin` e il file `out.txt`.


## Pulizia dell'Ambiente

Per rimuovere i file generati durante la compilazione e l'esecuzione, è possibile utilizzare il comando:

```bash
make clear
```
Lo script Bash seguente è un'utilità per pulire le risorse IPC (Inter-Process Communication) create durante l'esecuzione del programma. Le risorse IPC sono un meccanismo di comunicazione tra processi in un sistema operativo.
- Le risorse vengono normalmente rimosse con la terminazione della simulazione, in caso contrario, quindi in caso di blocco dell'esecuzione del programma o di errore si può utilizzare tale codice per pulire le risorse IPC:


```bash
#!/bin/bash

# Termina i processi nave, porto e master se sono in esecuzione
pkill nave.out 
pkill porto.out
pkill master.out


# Rimuovere tutte le code di messaggi
ipcs -q | awk '{if ($1 ~ /^[0-9]/) print "-q " $2}' | xargs ipcrm

# Rimuovere tutte le shared memory segments
ipcs -m | grep '^0x' | awk '{print "-m " $2}' | xargs ipcrm

# Rimuovere tutti i semafori
ipcs -s | awk '{if ($1 ~ /^[0-9]/) print "-s " $2}' | xargs ipcrm

echo "Tutte le risorse IPC rimosse con successo."
```
pkill nave.out, pkill porto.out, pkill master.out: Questi comandi usano pkill per terminare i processi con i nomi specificati (nave.out, porto.out, master.out).

Le successive tre sezioni utilizzano comandi ipcs per elencare le risorse IPC attualmente in uso, filtrano i risultati e utilizzano xargs per passare gli ID delle risorse alle relative versioni di ipcrm che le rimuovono.

* `ipcs -q`: elenca tutte le code di messaggi.
* `ipcs -m`: elenca tutte le shared memory segments.
* `ipcs -s`: elenca tutti i semafori.

Le risorse vengono rimosse utilizzando il comando ipcrm seguito dall'opzione corrispondente (-q, -m, -s) e l'ID della risorsa.

Infine, viene stampato un messaggio indicando che tutte le risorse IPC sono state rimosse con successo.


