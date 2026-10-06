#ifndef UI_H
#define UI_H

#include "ipc_common.h"

/* Display the simulator menu */
void display_menu(void);

/* Send a message to the Core process */
void send_to_core(Message *msg);

/* Receive a response from the Core process */
void receive_from_core(void);

/* Convert user input into a Message */
int create_command(char *input, Message *msg);

#endif
