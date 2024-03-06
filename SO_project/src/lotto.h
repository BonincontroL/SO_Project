
typedef struct lotto *Lotto;

struct lotto
{
    int id; /* id merce */
    int qt_merce;
    int temp_vita;
};

Lotto  new_lotto();
int Get_id(Lotto);
int Get_qt_merce(Lotto);
int Get_temp_vita(Lotto);

void Set_temp_vita(Lotto,int);
void Set_qt_merce(Lotto,int);
void Set_temp_vita(Lotto,int);