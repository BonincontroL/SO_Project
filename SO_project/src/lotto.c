#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lotto.h"

Lotto new_lotto();
int Get_id(Lotto l);
int Get_qt_merce(Lotto l);
int Get_temp_vita(Lotto l);



Lotto new_lotto()
{
    Lotto val;
    val = malloc(sizeof *val);
  /*   val=malloc(sizeof(struct lotto)); */
    if (val == NULL)
    {
        printf("Errore nell'allocazione di memoria!!");
        exit(1);
    }
    
    return val;
}

int Get_id(Lotto l)
{
    return l->id;
}

int Get_qt_merce(Lotto l)
{
    return l->qt_merce;
}

int Get_temp_vita(Lotto l)
{
    return l->temp_vita;
}


void Set_id(Lotto l, int p_id)
{
     l->id=p_id;
}

void Set_qt_merce(Lotto l,int p_qt_merce)
{
     l->qt_merce=p_qt_merce;
}

void Set_temp_vita(Lotto l,int p_temp_vita)
{
     l->temp_vita=p_temp_vita;
}



