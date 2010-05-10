#include "qp_port.h"
#include "test_isr.h"
#include "bsp.h"

Q_DEFINE_THIS_FILE

/* Active object class -----------------------------------------------------*/
typedef struct EepromTag {
     QActive super;
     uint8_t write_index;
     uint8_t storage[EEPROM_SIZE];
} Eeprom;

static QState Eeprom_initial(Eeprom *me, QEvent const *e);
static QState Eeprom_idle(Eeprom *me, QEvent const *e);
static QState Eeprom_write(Eeprom *me, QEvent const *e);
static QState Eeprom_write_address(Eeprom *me, QEvent const *e);
static QState Eeprom_write_nbytes(Eeprom *me, QEvent const *e);

/* Local objects -----------------------------------------------------------*/
static Eeprom l_eeprom;  /* the single instance of the Eeprom active object */

/* Global-scope objects ----------------------------------------------------*/
QActive * const AO_Eeprom = (QActive *)&l_eeprom;    /* "opaque" AO pointer */

/*..........................................................................*/
void Eeprom_ctor(void) {

     Eeprom *me = &l_eeprom;

     QActive_ctor(&me->super, (QStateHandler)&Eeprom_initial);

     int i;
     for (i=0; i<EEPROM_SIZE; i++) {
	  me->storage[i] = 0;
     }
     
}
/*..........................................................................*/
QState Eeprom_initial(Eeprom *me, QEvent const *e) {
    (void)e;        /* suppress the compiler warning about unused parameter */

    return Q_TRAN(&Eeprom_idle);
}
/*..........................................................................*/
QState Eeprom_idle(Eeprom *me, QEvent const *e) {
     
     switch( e->sig ) {
     case EEPROM_WRITE_SIG:
	  return Q_TRAN(&Eeprom_write);
     }
     
     return Q_SUPER(&QHsm_top);
}

QState Eeprom_write(Eeprom *me, QEvent const *e) {

     switch( e->sig ) {
     case START_ADDRESS_SIG: {
	  StartAddrEvt *sae = (StartAddrEvt*) e;
	  me->write_index = sae->addr;
	  return Q_TRAN(&Eeprom_write_address);
     }
     }

     return Q_SUPER(&QHsm_top);
}


QState Eeprom_write_address(Eeprom *me, QEvent const *e) {

     return Q_SUPER(&QHsm_top);
}

	  
