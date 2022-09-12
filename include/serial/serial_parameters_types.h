#pragma once

typedef enum {
    BAUDRATE_110,
    BAUDRATE_300,
    BAUDRATE_600,
    BAUDRATE_1200,
    BAUDRATE_2400,
    BAUDRATE_4800,
    BAUDRATE_9600,
    BAUDRATE_19200,
    BAUDRATE_38400,
    BAUDRATE_57600,
    BAUDRATE_115200,
} baudrate_t;

typedef enum {
    DATABITS_5,
    DATABITS_6,
    DATABITS_7,
    DATABITS_8,
    DATABITS_16,
} databits_t;

typedef enum { PARITY_NONE, PARITY_ODD, PARITY_EVEN } parity_t;

typedef enum { STOPBITS_1, STOPBITS_2 } stopbits_t;
