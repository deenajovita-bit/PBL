#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>

#include "ui.h"
#include "ipc_common.h"

mqd_t ui_to_core;
mqd_t core_to_ui;


/* Display menu */
void display_menu(void)
{
    printf("\n========== CPU SIMULATOR ==========\n");
    printf("LOAD <value>\n");
    printf("ADD <value>\n");
    printf("SUB <value>\n");
    printf("PUSH <value>\n");
    printf("POP\n");
    printf("STORE <address> <value>\n");
    printf("LOADM <address>\n");
    printf("PRINT\n");
    printf("SHOW CPU\n");
    printf("SHOW MEMORY\n");
    printf("SHOW STACK\n");
    printf("SHOW QUEUE\n");
    printf("EXIT\n");
    printf("===================================\n");
}


/* Convert user input into a Message */
int create_command(char *input, Message *msg)
{
    char command[20];
    char option[20];
    int value;
    int address;

    memset(msg, 0, sizeof(Message));

    /* STORE <address> <value> */
    if (sscanf(input, "%19s %d %d",
               command, &address, &value) == 3)
    {
        if (strcmp(command, "STORE") == 0)
        {
            msg->type = CMD_STORE;
            msg->address = address;
            msg->value = value;
            return 1;
        }

        return 0;
    }

    /* Commands with one value */
    if (sscanf(input, "%19s %d",
               command, &value) == 2)
    {
        if (strcmp(command, "LOAD") == 0)
        {
            msg->type = CMD_LOAD;
            msg->value = value;
        }
        else if (strcmp(command, "ADD") == 0)
        {
            msg->type = CMD_ADD;
            msg->value = value;
        }
        else if (strcmp(command, "SUB") == 0)
        {
            msg->type = CMD_SUB;
            msg->value = value;
        }
        else if (strcmp(command, "PUSH") == 0)
        {
            msg->type = CMD_PUSH;
            msg->value = value;
        }
        else if (strcmp(command, "LOADM") == 0)
        {
            msg->type = CMD_LOADM;
            msg->address = value;
        }
        else
        {
            return 0;
        }

        return 1;
    }

    /* SHOW CPU / MEMORY / STACK / QUEUE */
    if (sscanf(input, "%19s %19s",
               command, option) == 2)
    {
        if (strcmp(command, "SHOW") != 0)
            return 0;

        if (strcmp(option, "CPU") == 0)
            msg->type = CMD_SHOW_CPU;
        else if (strcmp(option, "MEMORY") == 0)
            msg->type = CMD_SHOW_MEMORY;
        else if (strcmp(option, "STACK") == 0)
            msg->type = CMD_SHOW_STACK;
        else if (strcmp(option, "QUEUE") == 0)
            msg->type = CMD_SHOW_QUEUE;
        else
            return 0;

        return 1;
    }

    /* Commands without arguments */
    if (sscanf(input, "%19s", command) == 1)
    {
        if (strcmp(command, "POP") == 0)
            msg->type = CMD_POP;
        else if (strcmp(command, "PRINT") == 0)
            msg->type = CMD_PRINT;
        else if (strcmp(command, "EXIT") == 0)
            msg->type = CMD_EXIT;
        else
            return 0;

        return 1;
    }

    return 0;
}


/* Send message to Core */
void send_to_core(Message *msg)
{
    if (mq_send(ui_to_core,
                (const char *)msg,
                sizeof(Message),
                0) == -1)
    {
        perror("mq_send");
    }
}


/* Receive response from Core */
void receive_from_core(void)
{
    Message response;

    if (mq_receive(core_to_ui,
                   (char *)&response,
                   sizeof(Message),
                   NULL) == -1)
    {
        perror("mq_receive");
        return;
    }

    switch (response.type)
    {
        case RESPONSE_SUCCESS:
            printf("\nSUCCESS: %s\n", response.data);
            break;

        case RESPONSE_ERROR:
            printf("\nERROR: %s\n", response.data);
            break;

        case RESPONSE_CPU:
            printf("\n--- CPU INFORMATION ---\n");
            printf("%s\n", response.data);
            break;

        case RESPONSE_MEMORY:
            printf("\n--- MEMORY INFORMATION ---\n");
            printf("%s\n", response.data);
            break;

        case RESPONSE_STACK:
            printf("\n--- STACK INFORMATION ---\n");
            printf("%s\n", response.data);
            break;

        case RESPONSE_QUEUE:
            printf("\n--- QUEUE INFORMATION ---\n");
            printf("%s\n", response.data);
            break;

        default:
            printf("\nUnknown response received from Core.\n");
            break;
    }
}


/* Main UI process */
int main(void)
{
    char input[MAX_DATA];
    Message message;

    printf("\nStarting CPU Simulator UI...\n");

    /* Open UI -> Core queue */
    ui_to_core = mq_open(UI_TO_CORE_QUEUE, O_WRONLY);

    if (ui_to_core == (mqd_t)-1)
    {
        perror("Failed to open UI-to-Core queue");
        return EXIT_FAILURE;
    }

    /* Open Core -> UI queue */
    core_to_ui = mq_open(CORE_TO_UI_QUEUE, O_RDONLY);

    if (core_to_ui == (mqd_t)-1)
    {
        perror("Failed to open Core-to-UI queue");
        mq_close(ui_to_core);
        return EXIT_FAILURE;
    }

    printf("Connected to Core process.\n");

    while (1)
    {
        display_menu();

        printf("\nEnter command: ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
            continue;

        /* Convert input into Message */
        if (!create_command(input, &message))
        {
            printf("\nInvalid command.\n");
            continue;
        }

        /* Send command to Core */
        send_to_core(&message);

        /* EXIT */
        if (message.type == CMD_EXIT)
        {
            printf("\nExiting UI...\n");
            break;
        }

        /* Receive response from Core */
        receive_from_core();
    }

    /* Close queues */
    mq_close(ui_to_core);
    mq_close(core_to_ui);

    printf("\nUI process terminated.\n");

    return EXIT_SUCCESS;
}
