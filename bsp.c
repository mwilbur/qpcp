#include "qp_port.h"
#include "test_isr.h"
#include "bsp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>                            /* for memcpy() and memset() */
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

Q_DEFINE_THIS_FILE


/* Local objects -----------------------------------------------------------*/

static struct termios l_tsav;
static pthread_mutex_t l_isr_mutex;
static pthread_cond_t  l_isr_signal_cv;
static uint8_t l_isr_signal = 0;

/*..........................................................................*/
static void *idleThread(void *me) {      /* the expected P-Thread signature */
    for (;;) {
        struct timeval timeout = { 0 };             /* timeout for select() */
        fd_set con;                      /* FD set representing the console */
        FD_ZERO(&con);
        FD_SET(STDIN_FILENO, &con);
        timeout.tv_usec = 8000;

        /* sleep for the full tick or until a console input arrives */
        if (0 != select(STDIN_FILENO+1, &con, 0, 0, &timeout)) {  /* any descriptor set? */
            char ch;
            read(STDIN_FILENO, &ch, 1);
	    pthread_mutex_lock( &l_isr_mutex );
	    l_isr_signal = (uint8_t) strtol(&ch, (char **)NULL, 16);
	    if( l_isr_signal > 0 ) 
		 pthread_cond_signal( &l_isr_signal_cv);
	    pthread_mutex_unlock( &l_isr_mutex );
            }
        }
}


static void *isrThread(void *me) {

    static QEvent sla_w_evt = {I2C_SLA_W_SIG, 0};
    static QEvent ps_evt = {I2C_PS_SIG, 0};
    static I2CDataEvt data_evt = {{I2C_DATA_SIG, 0}, 0};
    static QEvent *e;

    for (;;) {
         
        static int state = 0;
        uint8_t sig;
        
        pthread_mutex_lock( &l_isr_mutex );
        if( l_isr_signal == 0 )
            pthread_cond_wait( &l_isr_signal_cv, &l_isr_mutex );
        
        sig = l_isr_signal;
        l_isr_signal = 0;

        pthread_mutex_unlock( &l_isr_mutex );
        
        switch(sig) {

        case 1: 
            e = &sla_w_evt;
            printf("Dispatching I2C_SLA_W_EVT to FSM\n");
            QFsm_dispatch( FSM_Isr, e);
            break;

        case 2: {
            char ch;
            read(STDIN_FILENO, &ch, 1);
            data_evt.byte = (uint8_t) strtol(&ch, (char **)NULL, 16);
            e = (QEvent*) &data_evt;
            printf("Dispatching I2C_DATA_EVT to FSM: %d\n",data_evt.byte);
            QFsm_dispatch( FSM_Isr, e);
            break;
        }

        case 3: {
            e = (QEvent*) &ps_evt;
            printf("Dispatching I2C_PS_EVT to FSM.\n");
            QFsm_dispatch( FSM_Isr, e);
            break;
        }
        }
        
    }
}


/*..........................................................................*/
void BSP_init(int argc, char *argv[]) {
    char const *hostAndPort = "localhost:6601";

    if (argc > 2) {                                      /* port specified? */
        hostAndPort = argv[2];
    }
    if (!QS_INIT(hostAndPort)) {
        printf("\nUnable to open QS socket\n");
        QF_stop();
    }
    printf("Dining Philosopher Problem example"
           "\nQEP %s\nQF  %s\n"
           "Press ESC to quit...\n",
           QEP_getVersion(),
           QF_getVersion());
}
/*..........................................................................*/
void QF_onStartup(void) {                               /* startup callback */
    struct termios tio;                     /* modified terminal attributes */
    pthread_attr_t attr;
    struct sched_param param;
    pthread_t idle, isr;

    QFsm_init(FSM_Isr, (QEvent *)0); /* take the initial transition */

    tcgetattr(0, &l_tsav);          /* save the current terminal attributes */
    tcgetattr(0, &tio);           /* obtain the current terminal attributes */
    tio.c_lflag &= ~(ICANON | ECHO);   /* disable the canonical mode & echo */
    tcsetattr(0, TCSANOW, &tio);                  /* set the new attributes */

    pthread_mutex_init( &l_isr_mutex, NULL );  /* pthread mutext to protect */ 
                                                      /* condition variable */
    pthread_cond_init( &l_isr_signal_cv, NULL );
    
    /* SCHED_FIFO corresponds to real-time preemptive priority-based scheduler
    * NOTE: This scheduling policy requires the superuser priviledges
    */
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = sched_get_priority_min(SCHED_FIFO);

    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&idle, &attr, &idleThread, 0) != 0) {
               /* Creating the p-thread with the SCHED_FIFO policy failed.
               * Most probably this application has no superuser privileges,
               * so we just fall back to the default SCHED_OTHER policy
               * and priority 0.
               */
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        param.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &param);
        Q_ALLEGE(pthread_create(&idle, &attr, &idleThread, 0) == 0);
    }

    if (pthread_create(&isr, &attr, &isrThread, 0) != 0) {
               /* Creating the p-thread with the SCHED_FIFO policy failed.
               * Most probably this application has no superuser privileges,
               * so we just fall back to the default SCHED_OTHER policy
               * and priority 0.
               */
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        param.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &param);
        Q_ALLEGE(pthread_create(&isr, &attr, &isrThread, 0) == 0);
    }

    pthread_attr_destroy(&attr);
}
/*..........................................................................*/
void QF_onCleanup(void) {                               /* cleanup callback */
    printf("\nBye! Bye!\n");
    tcsetattr(0, TCSANOW, &l_tsav);/* restore the saved terminal attributes */
    pthread_mutex_destroy( &l_isr_mutex );
    pthread_cond_destroy( &l_isr_signal_cv );
    QS_EXIT();                                    /* perfomr the QS cleanup */
}
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    fprintf(stderr, "Assertion failed in %s, line %d", file, line);
    QF_stop();
}





