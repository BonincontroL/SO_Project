/* #include "lotto.h" */


typedef struct
{
  double x;
  double y;
} Coordinates;

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

/*esistono 4 porti uno per ogni angolo della mappa*/
Porto new_porto(double, double, int);

double tempo_scambio_merci(int);
void genera_domanda_offerta(Porto);
int porto_ha_bisogno_di_id(Porto, int);
void stampa_domanda(Porto);
void stampa_offerta(Porto);
int porto_get_id(Porto);