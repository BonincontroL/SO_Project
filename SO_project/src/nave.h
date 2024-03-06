#include "porto.h"
#include "lotto.h"
struct nave
{
    Coordinates c;
    int id;
    Lotto lotti;
    int N_merce_scaduta_nave;
};

typedef struct nave *Nave;

Nave new_nave(double, double, int);
double point_distance(Nave, Porto); /*distanza tra una nave e un punto*/
double time_to_go(Nave, double);    /*tempo per andare data una nave e una distanza*/

int nave_get_id(Nave);
Lotto crea_lotto();

void carica_nave(Nave N, Porto p);
void scarica_nave(Nave N, Porto p);