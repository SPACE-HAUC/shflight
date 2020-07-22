/**
 * @file adar1000.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function prototypes and data structure for ADAR1000 SPI Driver (Linux)
 * ADAR1000 SPI operation mode is 0.
 * @version 0.1
 * @date 2020-07-20
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef ADAR_1000_H
#define ADAR_1000_H
/** 
 * @brief Custom assert function to check if struct sizes are accurate.
 */
#define CASSERT(predicate) _impl_CASSERT_LINE(predicate, __LINE__)

#define _impl_PASTE(a, b) a##b
#define _impl_CASSERT_LINE(predicate, line) \
    typedef char _impl_PASTE(assertion_failed, line)[2 * !!(predicate)-1];

#ifndef SPI_BUS
#define SPI_BUS "/dev/spi1"
#endif

/**
 * @brief Data type representing an ADAR register.
 * To write to all chips, reset addr and set bit 11 of reg.
 * The internal structure is packed to produce 3 bytes instead of 4 due to 
 * alignment.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned char data;      ///< Contains the payload for the register
        unsigned short reg : 13; ///< Register address
        unsigned short addr : 3; ///< Chip addess
    };
    unsigned char bytes[3]; ///< Data for transfer
} adar_register;

typedef enum
{
    INTERFACE_CONFIG_A = 0x000,
    INTERFACE_CONFIG_B,
    DEV_CONFIG,
    CHIP_TYPE,
    PRODUCT_ID_H,
    PRODUCT_ID_L,
    SCRATCH_PAD = 0xa,
    SPI_REV,
    VENDOR_ID_H,
    VENDOR_ID_L,
    TRANSFER_REG = 0xf,
    CH1_RX_GAIN,
    CH2_RX_GAIN,
    CH3_RX_GAIN,
    CH4_RX_GAIN,
    CH1_RX_PHASE_I,
    CH1_RX_PHASE_Q,
    CH2_RX_PHASE_I,
    CH2_RX_PHASE_Q,
    CH3_RX_PHASE_I,
    CH3_RX_PHASE_Q,
    CH4_RX_PHASE_I,
    CH4_RX_PHASE_Q,
    CH1_TX_GAIN,
    CH2_TX_GAIN,
    CH3_TX_GAIN,
    CH4_TX_GAIN,
    CH1_TX_PHASE_I,
    CH1_TX_PHASE_Q,
    CH2_TX_PHASE_I,
    CH2_TX_PHASE_Q,
    CH3_TX_PHASE_I,
    CH3_TX_PHASE_Q,
    CH4_TX_PHASE_I,
    CH4_TX_PHASE_Q,
    LD_WRK_REGS = 0x28,
    CH1_PA_BIAS_ON, ///< Transmit
    CH2_PA_BIAS_ON,
    CH3_PA_BIAS_ON,
    CH4_PA_BIAS_ON,
    LNA_BIAS_ON,
    RX_ENABLES,
    TX_ENABLES,
    MISC_ENABLES = 0x30,
    SW_CTRL,
    ADC_CTRL,
    ADC_OUTPUT,
    BIAS_CURRENT_RX_LNA,
    BIAS_CURRENT_RX,
    BIAS_CURRENT_TX,
    BIAS_CURRENT_TX_DRV,
    MEM_CTRL = 0x38,
    RX_CHX_MEM,
    TX_CHX_MEM,
    RX_CH1_MEM = 0x3d,
    RX_CH2_MEM,
    RX_CH3_MEM,
    RX_CH4_MEM,
    TX_CH1_MEM,
    TX_CH2_MEM,
    TX_CH3_MEM,
    TX_CH4_MEM,
    REV_ID,
    CH1_PA_BIAS_OFF, ///< Receive
    CH2_PA_BIAS_OFF,
    CH3_PA_BIAS_OFF,
    CH4_PA_BIAS_OFF,
    LNA_BIAS_OFF,
    TX_TO_RX_DELAY_CTRL,
    RX_TO_TX_DELAY_CTRL,
    TX_BEAM_STEP_START,
    TX_BEAM_STEP_STOP,
    RX_BEAM_STEP_START,
    RX_BEAM_STEP_STOP,
    RX_BIAS_RAM_CTL,
    TX_BIAS_RAM_CTL,
    LDO_TRIM_CTL_0 = 0x400,
    LDO_TRIM_CTL_1
} ADAR_REG_ADDR;

/**
 * @brief Fields of register INTERFACE_CONFIG_A.
 * Functions of the last four bits in this register are intentionally replicated 
 * from the first four bits in a reverse manner so that the bit pattern
 * is the same, whether sent LSB first or MSB first.
 */
typedef union {
    struct
    {
        unsigned char softreset_ : 1;
        unsigned char lsb_first_ : 1;
        unsigned char addr_ascn_ : 1;
        unsigned char sdo_active_ : 1;
        unsigned char sdo_active : 1; ///< SDO Active, reset = 0
        unsigned char addr_ascn : 1;  ///< Adress ascension, reset = 0
        unsigned char lsb_first : 1;  ///< LSB first, reset = 0
        unsigned char softreset : 1;  ///< soft reset, reset = 0
    };
    unsigned char val;
} iface_config_a;
CASSERT(sizeof(iface_config_a) == 1); // Assert that the union is indeed 1 byte

/**
 * @brief Fields of register INTERFACE_CONFIG_B.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned rsvd : 1;
        unsigned soft_rst : 2; ///< soft reset
        unsigned rsvd2 : 1;
        unsigned slow_interface_ctr : 1; ///< Slow interface control
        unsigned master_slave_rb : 1;    ///< Master-slave readback
        unsigned csb_stall : 1;          ///< CSB Stall
        unsigned single_instr : 1;       ///< Single Instruction
    };
    unsigned char val;
} iface_config_b;
CASSERT(sizeof(iface_config_b) == 1);

/**
 * @brief Fields of register DEV_CONFIG.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned norm_operating_mode : 2; ///< Normal operating modes
        unsigned cust_operating_mode : 2; ///< Custom operating modes
        unsigned dev_status : 4;          ///< Device status
    };
    unsigned char val;
} dev_config;
CASSERT(sizeof(dev_config) == 1);

/**
 * @brief Fields of register TRANSFER_REG.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned master_slave_xfer : 1; ///< Master slave transfer
        unsigned rsvd : 7;
    };
    unsigned char val;
} transfer_reg;
CASSERT(sizeof(transfer_reg) == 1);

/**
 * @brief Fields of register CH1_TX_GAIN or CH1_RX_GAIN or similar.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_vga_chx : 7; ///< Channel X receive/transmit VGA gain control
        unsigned chx_attn_rx : 1; ///< Channel X attenuator setting for receive/transmit mode
    };
    unsigned char val;
} chx_trx_gain;
CASSERT(sizeof(chx_trx_gain) == 1);

/**
 * @brief Fields of register CH1_TX_PHASE_I or CH1_RX_PHASE_Q or similar.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_vm_chx_gain : 5; ///< TRX vector modulator I/Q gain
        unsigned trx_vm_chx_pol : 1;  ///< TRX vector modulator I/Q polarity
        unsigned rsvd : 2;
    };
    unsigned char val;
} chx_trx_phase;
CASSERT(sizeof(chx_trx_phase) == 1);

/**
 * @brief Loads working registers from SPI for transmit or receive.
 */
typedef union {
    struct __attribute__((packed))
    {
        unsigned ldrx_override : 1; ///< Loads receive working registers from SPI
        unsigned ldtx_override : 1; ///< Loads transmit working registers from SPI
        unsigned rsvd : 6;
    };
    unsigned char val;
} ld_wrk_regs;
CASSERT(sizeof(ld_wrk_regs) == 1);

typedef unsigned char chx_pa_bias_on; ///< External bias for external PA X
CASSERT(sizeof(chx_pa_bias_on) == 1);

typedef unsigned char lna_bias_on; ///< External bias for external LNAs, 0 to -4.8V
CASSERT(sizeof(lna_bias_on) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_vga_en : 1; ///< Enables the trx channel VGAs
        unsigned trx_vm_en : 1;  ///< Enables the trx channel vector modulators
        unsigned trx_lna_en : 1; ///< Enables receive channel LNAs or transmit channel drivers
        unsigned ch4_trx_en : 1; ///< Enables channel 4 subcircuits
        unsigned ch3_trx_en : 1; ///< Enables channel 3 subcircuits
        unsigned ch2_trx_en : 1; ///< Enables channel 2 subcircuits
        unsigned ch1_trx_en : 1; ///< Enables channel 1 subcircuits
        unsigned rsvd : 1;
    };
    unsigned char val;
} trx_enables;
CASSERT(sizeof(trx_enables) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned char ch4_det_en : 1; ///< Enables channel 4 power detector
        unsigned char ch3_det_en : 1; ///< Enables channel 3 power detector
        unsigned char ch2_det_en : 1; ///< Enables channel 2 power detector
        unsigned char ch1_det_en : 1; ///< Enables channel 1 power detector
        /**
         * @brief Enables output of LNA Bias DAC.
         * 0 = Open and 1 = Bias connected.
         */
        unsigned char lna_bias_out_en : 1;
        unsigned char bias_en : 1; ///< Enables PA and LNA bias DACs. 0 = enabled
        /**
         * @brief External Amplifer Bias Control.
         * 0 = DACs assume the on register values.
         * 1 = DACs vary with device mode (transmit and receive).
         */
        unsigned char bias_ctrl : 1;
        /**
         * @brief Transmit/Receive Output Driver Select.
         * If 0, TR_SW_NEG is enabled.
         * If 1, TR_SW_POS is enabled.
         */
        unsigned char sw_drv_tr_mode_sel : 1;
    };
    unsigned char val;
} misc_enables;
CASSERT(sizeof(misc_enables) == 1);

typedef union {
    struct __attribute__((packed))
    {
        /**
         * @brief Control for external polarity switch drivers.
         * 0 == outputs 0V
         * 1 == outputs -5V if switch is enabled.
         */
        unsigned char pol : 1;
        unsigned char tr_spi : 1;          ///< State of SPI control. 0 = receive, 1 = transmit
        unsigned char tr_src : 1;          ///< Source for transmit/receive contro. 0 = TR_SPI, 1 = TR input
        unsigned char sw_drv_en_pol : 1;   ///< Enables switch driver for external polarization switch. 1 = Enabled
        unsigned char sw_drv_en_tr : 1;    ///< Enables switch driver for external transmit/receive switch. 1 = Enabled.
        unsigned char rx_en : 1;           ///< Enables receive channel subcircuits under SPI control. 1 = Enabled
        unsigned char tx_en : 1;           ///< Enables transmit channel subcircuits under SPI control. 1 = Enabled
        unsigned char sw_drv_tr_state : 1; ///< Control sense of transmit/receive swutch driver output. If 0, the driver outputs 0V in receive mode.
    };
    unsigned char val;
} sw_ctrl;
CASSERT(sizeof(sw_ctrl) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned adc_eoc : 1;         ///< ADC end of conversion signal
        unsigned mux_sel : 3;         ///< ADC input signal select
        unsigned st_conv : 1;         ///< Pulse triggers conversion cycle
        unsigned clk_en : 1;          ///< Turns on clock oscillator
        unsigned adc_en : 1;          ///< Turns on comparator and resets state machine
        unsigned adc_clkfreq_sel : 1; ///< ADC clock frequency selection
    };
    unsigned char val;
} adc_ctrl;
CASSERT(sizeof(adc_ctrl) == 1);

typedef unsigned char adc_output;
CASSERT(sizeof(adc_output) == 1);

typedef unsigned char bias_current_rx_lna; ///< 4 bits only
CASSERT(sizeof(bias_current_rx_lna) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_vm_bias : 3;  ///< TR channel vector modulator bias current setting
        unsigned trx_vga_bias : 4; ///< TR channel VGA bias current setting
        unsigned rsvd : 1;
    };
    unsigned char val;
} bias_current_trx;
CASSERT(sizeof(bias_current_trx) == 1);

typedef unsigned char bias_current_tx_drv;
CASSERT(sizeof(bias_current_rx_lna) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned rx_chx_ram_bypass : 1; ///< Bypass RAM for receive channels
        unsigned tx_chx_ram_bypass : 1; ///< Bypass RAM for transmit channels
        unsigned rx_beam_step_en : 1;   ///< Sequentially step through stored receive beam positions
        unsigned tx_beam_step_en : 1;   ///< Sequentially step through stored transmit beam positions
        unsigned rsvd : 1;
        unsigned bias_ram_bypass : 1; ///< Bypass RAM and load bias position settings from SPI
        unsigned beam_ram_bypass : 1; ///< Bypass RAM and load beam position settings from SPI
        unsigned scan_mode_en : 1;    ///< Scan mode enable
    };
    unsigned char val;
} mem_ctrl;
CASSERT(sizeof(mem_ctrl) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_chx_ram_index : 7; ///< get tr channel beam settings from RAM
        unsigned trx_chx_ram_fetch : 1; ///< RAM index for TR channels
    };
    unsigned char val;
} trx_chx_mem;
CASSERT(sizeof(trx_chx_mem) == 1);

typedef unsigned char rev_id; ///< Chip revision ID
CASSERT(sizeof(rev_id) == 1);

typedef unsigned char chx_pa_bias_off; ///< External bias for external PA X
CASSERT(sizeof(chx_pa_bias_off) == 1);

typedef unsigned char lna_bias_off; ///< External bias for external LNAs, 0 to -4.8V
CASSERT(sizeof(lna_bias_off) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned tx_to_rx_delay_2 : 4; ///< TR switch to LNA Bias on delay
        unsigned tx_to_rx_delay_1 : 4; ///< PA bias off to TR switch delay
    };
    unsigned char val;
} tx_to_rx_delay_ctrl;
CASSERT(sizeof(tx_to_rx_delay_ctrl) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned rx_to_tx_delay_2 : 4; ///< TR switch to PA bias on delay
        unsigned rx_to_tx_delay_1 : 4; ///< LNA bias off to TR switch delay
    };
    unsigned char val;
} rx_to_tx_delay_ctrl;
CASSERT(sizeof(rx_to_tx_delay_ctrl) == 1);

typedef unsigned char tx_beam_step_start; ///< Start memory address for transmit channel beam stepping
CASSERT(sizeof(tx_beam_step_start) == 1);

typedef unsigned char tx_beam_step_stop; ///< Stop memory address for transmit channel beam stepping
CASSERT(sizeof(tx_beam_step_stop) == 1);

typedef unsigned char rx_beam_step_start; ///< Start memory address for receive channel beam stepping
CASSERT(sizeof(rx_beam_step_start) == 1);

typedef unsigned char rx_beam_step_stop; ///< Stop memory address for receive channel beam stepping
CASSERT(sizeof(rx_beam_step_stop) == 1);

typedef union {
    struct __attribute__((packed))
    {
        unsigned trx_bias_ram_index : 3; ///< RAM index for TRX channels
        unsigned trx_bias_ram_fetch : 1; ///< Get TRX beam settings from RAM
        unsigned rsvd : 4;
    };
    unsigned char val;
} trx_bias_ram_ctrl;
CASSERT(sizeof(trx_bias_ram_ctrl) == 1);

typedef unsigned char ldo_trim_ctl_0; ///< Trim Values for Adjusting LDO Outputs
CASSERT(sizeof(ldo_trim_ctl_0) == 1);

/**
 * @brief Set value to 2 (10 binary) to enable user adjustments for LDO outputs. 
 * Other combinations not recommended.
 */
typedef unsigned char ldo_trim_ctl_1; ///< 2 bits
CASSERT(sizeof(ldo_trim_ctl_1) == 1);

    /**
     * @brief  Beam Position Vector Modulator (VM) and VGA Decoding for Receiver
     *  and Transmitter Channel 1 to Channel 4.
     * This struct is packed in reverse order so that
     * xfer of val in SPI proceeds normally.
     * 
     * Proceed after Weston's reply.
     */ 
typedef union {
    // struct __attribute__((packed))
    // {
    //     unsigned vga_gain:7;
    //     unsigned attenuator:1;
    //     unsigned vm_i_gain:5;
    //     unsigned vm_i_pol:1;
    //     unsigned rsvd:2;
    //     unsigned vm_q_gain:5;
    //     unsigned vm_q_pol:1;
    //     unsigned rsvd2:2;
    // };
    struct __attribute__((packed))
    {
        unsigned rsvd:8; ///< Unused, for N/A address padding
        unsigned q_phase:8; ///< Combined gain and polarity from LUT
        unsigned i_phase:8; ///< Combined gain and polarity from LUT
        unsigned vga:8;
    };
    unsigned char val[4];
} adar_beam_pos;
CASSERT(sizeof(adar_beam_pos)==4);

/**
 * @brief Lookup table to convert phase angle to I value for vector modulator.
 * LUT phase angle quantization is 2.8125 deg. A given phase can be rounded to
 * the nearest integer after division by 2.8125.
 */ 
unsigned char adar_phase_to_i[] = {
    63,
    63,
    63,
    63,
    63,
    62,
    62,
    61,
    61,
    60,
    60,
    59,
    58,
    57,
    56,
    55,
    54,
    53,
    52,
    51,
    50,
    48,
    47,
    46,
    44,
    43,
    42,
    40,
    39,
    37,
    36,
    34,
    33,
    1,
    3,
    4,
    6,
    7,
    8,
    10,
    11,
    13,
    14,
    15,
    17,
    18,
    19,
    20,
    22,
    23,
    24,
    25,
    25,
    26,
    27,
    28,
    28,
    29,
    30,
    30,
    30,
    31,
    31,
    31,
    31,
    31,
    31,
    31,
    31,
    30,
    30,
    29,
    29,
    28,
    28,
    27,
    26,
    25,
    24,
    23,
    22,
    21,
    20,
    19,
    18,
    16,
    15,
    14,
    12,
    11,
    10,
    8,
    7,
    5,
    4,
    2,
    1,
    33,
    35,
    36,
    38,
    39,
    40,
    42,
    43,
    45,
    46,
    47,
    49,
    50,
    51,
    52,
    54,
    55,
    56,
    57,
    57,
    58,
    59,
    60,
    60,
    61,
    62,
    62,
    62,
    63,
    63,
    63};

/**
 * @brief Lookup table to convert phase angle to Q value for vector modulator.
 * LUT phase angle quantization is 2.8125 deg. A given phase can be rounded to
 * the nearest integer after division by 2.8125.
 */ 
unsigned char adar_phase_to_q[] = {
    32,
    33,
    35,
    36,
    38,
    39,
    40,
    42,
    43,
    45,
    46,
    47,
    48,
    49,
    51,
    52,
    53,
    54,
    55,
    56,
    56,
    57,
    58,
    58,
    59,
    60,
    60,
    60,
    61,
    61,
    61,
    61,
    61,
    61,
    61,
    61,
    61,
    60,
    60,
    60,
    59,
    58,
    58,
    57,
    56,
    56,
    55,
    54,
    53,
    52,
    51,
    49,
    48,
    47,
    46,
    45,
    43,
    42,
    40,
    39,
    38,
    36,
    35,
    33,
    32,
    1,
    3,
    4,
    6,
    7,
    8,
    10,
    11,
    13,
    14,
    15,
    16,
    17,
    19,
    20,
    21,
    22,
    23,
    24,
    24,
    25,
    26,
    26,
    27,
    28,
    28,
    28,
    29,
    29,
    29,
    29,
    29,
    29,
    29,
    29,
    29,
    28,
    28,
    28,
    27,
    26,
    26,
    25,
    24,
    24,
    23,
    22,
    21,
    20,
    19,
    17,
    16,
    15,
    14,
    13,
    11,
    10,
    8,
    7,
    6,
    4,
    3,
    1};

/**
 * @brief Collection of 121 beam parameters for storage in ADAR1000 RAM.
 */ 
typedef union __attribute__((packed)){
    adar_beam_pos beam[121];
    unsigned char val[121*sizeof(adar_beam_pos)];
} trx_beam_pos;
CASSERT(sizeof(trx_beam_pos)==121*sizeof(adar_beam_pos));

#include <sys/types.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

typedef struct
{
    unsigned char addr:3;
    struct spi_ioc_transfer xfer[1]; ///< SPI Transfer IO buffer
    int file;                        ///< File descriptor for SPI bus
    __u8 mode;                       ///< SPI Mode (Mode 0)
    __u8 lsb;                        ///< MSB First
    __u8 bits;                       ///< Number of bits per transfer (16)
    __u32 speed;                     ///< SPI Bus speed (1 MHz)
    char fname[40];                  ///< SPI device file name
    int cs_gpio;                     ///< GPIO file descriptor
} adar1000;

int adar1000_init(adar1000 *dev, unsigned char addr);
int adar1000_load_multiple_chns(adar1000 *devs, float *phases);
int adar1000_xfer(adar1000 *dev, void *data, ssize_t len);
int adar1000_wr_reg(adar1000 *dev, adar_register *reg);
void adar1000_destroy(adar1000 *dev);
#endif // ADAR_1000_H
/*
typedef union {
    struct __attribute__((packed))
    {
        
    };
    unsigned char val;
};
CASSERT(sizeof()==1);
*/