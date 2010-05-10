#include "qp_port.h"
#include "test_isr.h"
#include "bsp.h"

/* Local-scope objects -----------------------------------------------------*/
static QEvent const *l_eepromQueueSto[EEPROM_QSIZE];
static union SmallEvent {
     void *min_size;
     StartAddrEvt sae;
     NumBytesEvt nbe;
     /* other event types to go into this pool */
} l_smlPoolSto[N_EVENTS];              /* storage for the small event pool */

/*..........................................................................*/
int main(int argc, char *argv[]) {
    uint8_t n;

    BSP_init(argc, argv);           /* initialize the Board Support Package */
    
    Eeprom_ctor();
    ISR_ctor();

    QF_init();     /* initialize the framework and the underlying RT kernel */

    QF_poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(l_smlPoolSto[0]));

    QActive_start(AO_Eeprom, (uint8_t)1,
                  l_eepromQueueSto, Q_DIM(l_eepromQueueSto),
                  (void *)0, 0, (QEvent *)0);

    QF_run();                                     /* run the QF application */

    return 0;
}
