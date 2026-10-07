#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>

#include "ipc_common.h"

/* =========================================
   MESSAGE QUEUE DESCRIPTORS
   ========================================= */

mqd_t ui_to_core;
mqd_t core_to_ui;


/* =========================================
   DISPLAY MENU
   ========================================= */

void display_menu(void)
{
    printf("\n");
    printf("========================================\n");
    printf("           CPU SIMULATOR UI\n");
    printf("========================================\n");
    printf("LOAD <value>       - Load value into ACC\n");
    printf("ADD <value>        - Add value to ACC\n");
    printf("SUB <value>        - Subtract value from ACC\n");
    printf("PUSH <value>       - Push value onto stack\n");
    printf("POP                - Pop value into ACC\n");
    printf("STORE <address>    - Store ACC in memory\n");
    printf("LOADM <address>    - Load memory into ACC\n");
    printf("PRINT              - Print ACC\n");
    printf("SHOW CPU           - Show CPU information\n");
    printf("SHOW MEMORY        - Show memory information\n");
    printf("SHOW STACK         - Show stack information\n");
    printf("SHOW QUEUE         - Show instruction queue\n");
    printf("EXIT               - Exit simulator\n");
    printf("========================================\n");
}


/* =========================================
   CREATE MESSAGE FROM USER COMMAND
   ========================================= */

int create_command(char *input, Message *msg)
{
    char command[20];
    char option[20];
    int address;

    memset(msg, 0, sizeof(Message));

    /* -----------------------------------------
       STORE <address>
       ----------------------------------------- */

    if (sscanf(input, "%19s %d", command, &address) == 2)
    {
        if (strcmp(command, "STORE") == 0)
        {
            msg->type = CMD_STORE;
            msg->address = address;
            return 1;
        }

        if (strcmp(command, "LOADM") == 0)
        {
            msg->type = CMD_LOADM;
            msg->address = address;
            return 1;
        }

        if (strcmp(command, "LOAD") == 0)
        {
            msg->type = CMD_LOAD;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "ADD") == 0)
        {
            msg->type = CMD_ADD;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "SUB") == 0)
        {
            msg->type = CMD_SUB;
            msg->value = address;
            return 1;
        }

        if (strcmp(command, "PUSH") == 0)
        {
            msg->type = CMD_PUSH;
            msg->value = address;
            return 1;
        }

        return 0;
    }


    /* -----------------------------------------
       SHOW CPU / MEMORY / STACK / QUEUE
       ----------------------------------------- */

    if (sscanf(input, "%19s %19s", command, option) == 2)
    {
        if (strcmp(command, "SHOW") != 0)
            return 0;

        if (strcmp(option, "CPU") == 0)
        {
            msg->type = CMD_SHOW_CPU;
            return 1;
        }

        if (strcmp(option, "MEMORY") == 0)
        {
            msg->type = CMD_SHOW_MEMORY;
            return 1;
        }

        if (strcmp(option, "STACK") == 0)
        {
            msg->type = CMD_SHOW_STACK;
            return 1;
        }

        if (strcmp(option, "QUEUE") == 0)
        {
            msg->type = CMD_SHOW_QUEUE;
            return 1;
        }

        return 0;
    }


    /* -----------------------------------------
       COMMANDS WITHOUT ARGUMENTS
       ----------------------------------------- */

    if (sscanf(input, "%19s", command) == 1)
    {
        if (strcmp(command, "POP") == 0)
        {
            msg->type = CMD_POP;
            return 1;
        }

        if (strcmp(command, "PRINT") == 0)
        {
            msg->type = CMD_PRINT;
            return 1;
        }

        if (strcmp(command, "EXIT") == 0)
        {
            msg->type = CMD_EXIT;
            return 1;
        }
    }

    return 0;
}


/* =========================================
   SEND COMMAND TO CORE
   ========================================= */

int send_to_core(Message *msg)
{
    if (mq_send(ui_to_core,
                (const char *)msg,
                sizeof(Message),
                0) == -1)
    {
        perror("Error sending message to Core");
        return 0;
    }

    return 1;
}


/* =========================================
   RECEIVE RESPONSE FROM CORE
   ========================================= */

void receive_from_core(void)
{
    Message response;

    if (mq_receive(core_to_ui,
                   (char *)&response,
                   sizeof(Message),
                   NULL) == -1)
    {
        perror("Error receiving response from Core");
        return;
    }

    switch (response.type)
    {
        case RESPONSE_SUCCESS:

            printf("\nSUCCESS: ");

            if (strlen(response.data) > 0)
                printf("%s", response.data);
            else
                printf("Command executed successfully.");

            printf("\n");
            break;


        case RESPONSE_ERROR:

            printf("\nERROR: %s\n", response.data);
            break;


        case RESPONSE_CPU:

            printf("\n");
            printf("------------- CPU -------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_MEMORY:

            printf("\n");
            printf("----------- MEMORY ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_STACK:

            printf("\n");
            printf("------------ STACK ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        case RESPONSE_QUEUE:

            printf("\n");
            printf("------------ QUEUE ------------\n");
            printf("%s\n", response.data);
            printf("-------------------------------\n");
            break;


        default:

            printf("\nUnknown response received from Core.\n");
            break;
    }
}


/* =========================================
   MAIN UI PROCESS
   ========================================= */

int main(void)
{
    char input[MAX_DATA];
    Message message;

    printf("\n");
    printf("========================================\n");
    printf("       CPU SIMULATOR - UI PROCESS\n");
    printf("========================================\n");


    /* -----------------------------------------
       Open UI -> Core queue
       ----------------------------------------- */

    ui_to_core = mq_open(UI_TO_CORE_QUEUE, O_WRONLY);

    if (ui_to_core == (mqd_t)-1)
    {
        perror("Failed to open UI-to-Core queue");
        printf("Make sure the Core process is running.\n");
        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Open Core -> UI queue
       ----------------------------------------- */

    core_to_ui = mq_open(CORE_TO_UI_QUEUE, O_RDONLY);

    if (core_to_ui == (mqd_t)-1)
    {
        perror("Failed to open Core-to-UI queue");

        mq_close(ui_to_core);

        return EXIT_FAILURE;
    }


    printf("Connected to Core process.\n");
    printf("You can now enter commands.\n");


    /* -----------------------------------------
       Main command loop
       ----------------------------------------- */

    while (1)
    {
        display_menu();

        printf("\nEnter command: ");
        fflush(stdout);


        /* Read user input */

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\nInput closed.\n");
            break;
        }


        /* Remove newline */

        input[strcspn(input, "\n")] = '\0';


        /* Ignore empty input */

        if (strlen(input) == 0)
            continue;


        /* Convert command to Message */

        if (!create_command(input, &message))
        {
            printf("\nInvalid command.\n");
            printf("Please enter a valid command from the menu.\n");
            continue;
        }


        /* Send command to Core */

        if (!send_to_core(&message))
            continue;


        /* -----------------------------------------
           EXIT command
           ----------------------------------------- */

        if (message.type == CMD_EXIT)
        {
            /*
             * Core sends a final success response
             * before terminating.
             */
            receive_from_core();

            printf("\nUI process terminating...\n");
            break;
        }


        /* -----------------------------------------
           Receive Core response
           ----------------------------------------- */

        receive_from_core();
    }


    /* -----------------------------------------
       Close message queues
       ----------------------------------------- */

    mq_close(ui_to_core);
    mq_close(core_to_ui);


    printf("UI process terminated.\n");

    return EXIT_SUCCESS;
}

