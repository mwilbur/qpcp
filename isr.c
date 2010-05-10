#include "qp_port.h"
#include "test_isr.h"

#include <stdio.h>

typedef struct IsrTag {
    QFsm super;
    uint8_t command;
} Isr;


QState ISR_initial( Isr *me, QEvent *e);
QState ISR_idle( Isr *me, QEvent *e);
QState ISR_get_command( Isr *me, QEvent *e);
QState ISR_get_eeprom_write_address( Isr *me, QEvent *e);
QState ISR_get_eeprom_write_count( Isr *me, QEvent *e);
QState ISR_get_eeprom_write_bytes( Isr *me, QEvent *e);

/* Local objects -----------------------------------------------------------*/

static Isr l_fsm_isr;

/* Global-scope objects ----------------------------------------------------*/

QFsm * const FSM_Isr = (QFsm*) &l_fsm_isr;

/*..........................................................................*/
void ISR_ctor(void) {
    QFsm_ctor(&l_fsm_isr.super, \
              (QStateHandler)&ISR_initial);              /* superclass ctor */
}


/*..........................................................................*/
QState ISR_initial( Isr *me, QEvent *e) {
     
    return Q_TRAN( &ISR_idle );

}

/*..........................................................................*/
QState ISR_idle( Isr *me, QEvent *e) {
    
    switch(e->sig) {
    case I2C_SLA_W_SIG:
        printf("Received slave start\n");
        return Q_TRAN( &ISR_get_command );
    }
    
    return Q_IGNORED();
}

/*..........................................................................*/
QState ISR_get_command( Isr *me, QEvent *e) {
        
    switch(e->sig) {

    case Q_ENTRY_SIG:
        printf("Waiting for command\n");
        break;
    case I2C_DATA_SIG: {
        I2CDataEvt *de = (I2CDataEvt*) e;

        //        printf("Received command: %d\n",de->byte);
        me->command = de->byte;
        printf("Received command: %d\n",me->command);

        switch(me->command) {
         case EEPROM_WRITE:
             printf("EEPROM_WRITE\n");
             return Q_TRAN(&ISR_get_eeprom_write_address);
             break;
        default:
            printf("Unknown command.  Returning to idle\n");
            return Q_TRAN(&ISR_idle);
        }
    }
    }

    return Q_IGNORED();
}


/*..........................................................................*/
QState ISR_get_eeprom_write_address( Isr *me, QEvent *e) {

    switch(e->sig) {

    case Q_ENTRY_SIG:
        printf("Waiting for eeprom write address\n");
        break;
    case I2C_DATA_SIG: {
        I2CDataEvt *de = (I2CDataEvt*) e;
        printf("Write address: 0x%x\n", de->byte );
        return Q_TRAN( &ISR_get_eeprom_write_count );
        break;
    }
    }
    return Q_IGNORED();
}

/*..........................................................................*/
QState ISR_get_eeprom_write_count( Isr *me, QEvent *e) {
    
    switch(e->sig) {

    case Q_ENTRY_SIG:
        printf("Waiting for eeprom write count\n");
        break;
    case I2C_DATA_SIG: {
        I2CDataEvt *de = (I2CDataEvt*) e;
        printf("Number of bytes: 0x%x\n", de->byte );
        return Q_TRAN( &ISR_get_eeprom_write_bytes );
        break;
    }
    }
    return Q_IGNORED();
}

/*..........................................................................*/
QState ISR_get_eeprom_write_bytes( Isr *me, QEvent *e) {
    
    switch(e->sig) {

    case Q_ENTRY_SIG:
        printf("Collecting eeprom write byte\n");
        break;

    case I2C_DATA_SIG: {
        I2CDataEvt *de = (I2CDataEvt*) e;
        printf("Got byte: 0x%x\n", de->byte );
        return Q_TRAN( &ISR_get_eeprom_write_bytes );
        break;
    }

    case I2C_PS_SIG: {
        printf("Done EEPROM_WRITE\n");
        return Q_TRAN( &ISR_idle);
    }
    }

    return Q_IGNORED();
}
        
