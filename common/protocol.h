#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CMD_QUEUE  "/atm_commands"
#define RESP_QUEUE "/atm_responses"
#define LOG_QUEUE  "/bank_logs"

typedef struct {
    int type;
    char command[16];
    char account[16];
    long amount;
    long balance;
    char status[16];
    char details[128];
} Message;

#endif