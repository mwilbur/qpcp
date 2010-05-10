
enum ISRSigs {
    I2C_SLA_W_SIG = Q_USER_SIG,
    I2C_DATA_SIG,
    I2C_PS_SIG,
    EEPROM_WRITE_SIG,
    START_ADDRESS_SIG,
    NBYTES_SIG,
    ADD_BYTE_SIG
};

enum Commands {
    EEPROM_WRITE
};

enum {EEPROM_SIZE = 128};
enum {EEPROM_QSIZE = 8};
enum {N_EVENTS = 16};

typedef struct I2CDataEvtTag {
    QEvent super;
    uint8_t byte;
} I2CDataEvt;

typedef struct StartAddrEvtTag {
    QEvent super;
    uint8_t addr;
} StartAddrEvt;

typedef struct NumBytesEvtTag {
    QEvent super;
    uint8_t nbytes;
} NumBytesEvt;

typedef struct AddByteEvtTag {
    QEvent super;
    uint8_t byte;
} AddByteEvt;

void Eeprom_ctor(void);
void ISR_ctor(void);

extern QActive * const AO_Eeprom;
extern QFsm * const FSM_Isr;
