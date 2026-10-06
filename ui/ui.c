#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "ui.h"
#include "ipc_common.h"

/* POSIX message queue descriptors */
mqd_t ui_to_core;
mqd_t core_to_ui;


/* ---------------------------------------------------------
   Display the simulator menu
   --------------------------------------------------------- */
void display_menu(void)
{
    printf("\n========== CPU SIMULATOR ==========\n");
    printf("1. LOAD <value>\n");
    printf("2. ADD <value>\n");
    printf("3. SUB <value>\n");
    printf("4. PUSH <value>\n");
    printf("5. POP\n");
    printf("6. STORE <address> <value>\n");
    printf("7. LOADM <address>\n");
    printf("8. PRINT\n");
    printf("9. SHOW CPU\n");
    printf("10. SHOW MEMORY\n");
    printf("11. SHOW STACK\n");
    printf("12. SHOW QUEUE\n");
    printf("13. HALT\n");
    printf("14. EXIT\n");
    printf("===================================\n");
}


/* ---------------------------------------------------------
   Convert user input into a Message
   --------------------------------------------------------- */
int create_command(char *input, Message *msg)
{
    char command[20];
    char option[20];
    int value;
    int address;

    /* Clear the message */
    memset(msg, 0, sizeof(Message));

    /*
     * STORE requires two integers:
     * STORE <address> <value>
     */
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


    /*
     * Commands that require one integer
     */
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


    /*
     * Handle:
     * SHOW CPU
     * SHOW MEMORY
     * SHOW STACK
     * SHOW QUEUE
     */
    if (sscanf(input, "%19s %19s",
               command, option) == 2)
    {
        if (strcmp(command, "SHOW") != 0)
        {
            return 0;
        }

        if (strcmp(option, "CPU") == 0)
        {
            msg->type = CMD_SHOW_CPU;
        }

        else if (strcmp(option, "MEMORY") == 0)
        {
            msg->type = CMD_SHOW_MEMORY;
        }

        else if (strcmp(option, "STACK") == 0)
        {
            msg->type = CMD_SHOW_STACK;
        }

        else if (strcmp(option, "QUEUE") == 0)
        {
            msg->type = CMD_SHOW_QUEUE;
        }

        else
        {
            return 0;
        }

        return 1;
    }


    /*
     * Commands that don't require arguments
     */
    if (sscanf(input, "%19s", command) == 1)
    {
        if (strcmp(command, "POP") == 0)
        {
            msg->type = CMD_POP;
        }

        else if (strcmp(command, "PRINT") == 0)
        {
            msg->type = CMD_PRINT;
        }

        else if (strcmp(command, "HALT") == 0)
        {
            msg->type = CMD_HALT;
        }

        else if (strcmp(command, "EXIT") == 0)
        {
            msg->type = CMD_EXIT;
        }

        else
        {
            return 0;
        }

        return 1;
    }

    return 0;
}


/* ---------------------------------------------------------
   Send message to Core
   --------------------------------------------------------- */
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


/* ---------------------------------------------------------
   Receive response from Core
   --------------------------------------------------------- */
void receive_from_core(void)
{
    Message response;

    ssize_t bytes_received;

    bytes_received = mq_receive(
        core_to_ui,
        (char *)&response,
        sizeof(Message),
        NULL
    );

    if (bytes_received == -1)
    {
        perror("mq_receive");
        return;
    }


    /*
     * Display response according to its type
     */
    switch (response.type)
    {
        case RESPONSE_SUCCESS:

            printf("\nSUCCESS: %s\n",
                   response.data);

            break;


        case RESPONSE_ERROR:

            printf("\nERROR: %s\n",
                   response.data);

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


/* ---------------------------------------------------------
   Main UI Process
   --------------------------------------------------------- */
int main(void)
{
    char input[MAX_DATA];
    Message message;


    printf("Starting CPU Simulator UI...\n");


    /*
     * Open UI -> Core queue
     *
     * UI only writes to this queue.
     */
    ui_to_core = mq_open(
        UI_TO_CORE_QUEUE,
        O_WRONLY
    );


    if (ui_to_core == (mqd_t)-1)
    {
        perror("Failed to open UI-to-Core queue");

        return EXIT_FAILURE;
    }


    /*
     * Open Core -> UI queue
     *
     * UI only reads from this queue.
     */
    core_to_ui = mq_open(
        CORE_TO_UI_QUEUE,
        O_RDONLY
    );


    if (core_to_ui == (mqd_t)-1)
    {
        perror("Failed to open Core-to-UI queue");

        mq_close(ui_to_core);

        return EXIT_FAILURE;
    }


    printf("Connected to Core process.\n");


    /*
     * Main UI loop
     */
    while (1)
    {
        display_menu();


        printf("\nEnter command: ");


        /*
         * Read user input
         */
        if (fgets(input,
                  sizeof(input),
                  stdin) == NULL)
        {
            break;
        }


        /*
         * Remove newline added by fgets()
         */
        input[strcspn(input, "\n")] = '\0';


        /*
         * Ignore empty input
         */
        if (strlen(input) == 0)
        {
            continue;
        }


        /*
         * Convert the user's input
         * into a Message.
         */
        if (!create_command(input, &message))
        {
            printf("\nInvalid command.\n");
            printf("Please enter a valid command.\n");

            continue;
        }


        /*
         * Send command to Core.
         */
        send_to_core(&message);


        /*
         * CMD_EXIT terminates the UI.
         *
         * Core receives CMD_EXIT and handles
         * termination of the simulator.
         */
        if (message.type == CMD_EXIT)
        {
            printf("\nExiting UI...\n");

            break;
        }


        /*
         * Wait for Core response.
         */
        receive_from_core();
    }


    /*
     * Close POSIX message queues.
     *
     * We close them here, but do NOT unlink them
     * because queue ownership/cleanup should be
     * coordinated with the Core/integration code.
     */
    mq_close(ui_to_core);
    mq_close(core_to_ui);


    printf("UI process terminated.\n");


    return EXIT_SUCCESS;
}
