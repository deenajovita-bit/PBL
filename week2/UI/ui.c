#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <strings.h>
#include <ctype.h>

#include "sim_interface.h"
#include "sync.h"

/* =========================================
   IPC HANDLES
   Socket (commands + replies), shared memory (live state)
   ========================================= */

int sock_fd = -1;

SharedState *shared = NULL;


/* Read exactly 'size' bytes. Returns size, 0 if Core closed, -1 on error. */

ssize_t read_full(int fd, void *buffer, size_t size)
{
    char *p = buffer;
    size_t done = 0;

    while (done < size)
    {
        ssize_t n = read(fd, p + done, size - done);

        if (n == 0)
            return 0;

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        done += (size_t)n;
    }

    return (ssize_t)done;
}


/* Write exactly 'size' bytes. Returns 0 or -1. */

int write_full(int fd, const void *buffer, size_t size)
{
    const char *p = buffer;
    size_t done = 0;

    while (done < size)
    {
        ssize_t n = write(fd, p + done, size - done);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            return -1;
        }

        done += (size_t)n;
    }

    return 0;
}


/* =========================================
   LIVE STATE (reads SHARED MEMORY)
   lock_state / unlock_state are the semaphores
   so we never read half-written data.
   ========================================= */

void show_live_state(void)
{
    SharedState copy;

    lock_state();
    copy = *shared;
    unlock_state();

    printf("\n");
    printf("======= LIVE STATE (shared memory) =======\n");
    printf("ACC = %d    PC = %d\n", copy.accumulator, copy.program_counter);
    printf("Stack : size=%d", copy.stack_top + 1);

    for (int i = 0; i <= copy.stack_top && i < STACK_SIZE; i++)
        printf("%s%d", i == 0 ? "  [" : ", ", copy.stack[i]);

    if (copy.stack_top >= 0)
        printf("]");

    printf("\n");
    printf("Queue : %d instruction(s) waiting\n", copy.queue_count);
    printf("Memory (non-zero cells):");

    int shown = 0;

    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        if (copy.memory[i] != 0)
        {
            printf(" [%d]=%d", i, copy.memory[i]);
            shown++;
        }
    }

    if (shown == 0)
        printf(" none");

    printf("\n==========================================\n");
}


/* =========================================
   DISPLAY MENU
   ========================================= */

void display_menu(void)
{
    printf("\n");
    printf("========================================\n");
    printf("           CPU SIMULATOR UI\n");
    printf("========================================\n");
    printf(" 1.  LOAD   - Load a value into ACC\n");
    printf(" 2.  ADD    - Add a value to ACC\n");
    printf(" 3.  SUB    - Subtract a value from ACC\n");
    printf(" 4.  PUSH   - Push a value onto the stack\n");
    printf(" 5.  POP    - Pop a value into ACC\n");
    printf(" 6.  STORE  - Store ACC in a memory address\n");
    printf(" 7.  LOADM  - Load a memory address into ACC\n");
    printf(" 8.  PRINT  - Print ACC\n");
    printf(" 9.  Show CPU\n");
    printf("10.  Show memory\n");
    printf("11.  Show stack\n");
    printf("12.  Show instruction queue\n");
    printf("13.  Live state (shared memory)\n");
    printf(" 0.  Exit simulator\n");
    printf("========================================\n");
    printf("(You can also type a command, for example: ADD 5)\n");
}


/* =========================================
   MENU CHOICE -> COMMAND TEXT
   Turns a menu number into the same text command the user
   could have typed (for example choice 2 + value 5 -> "ADD 5"),
   so the rest of the UI does not change.

   Returns 1 = 'input' now holds a command to run,
           2 = STATE (handled by the UI itself),
           0 = nothing to run (cancelled or invalid).
   ========================================= */

int read_number(const char *prompt, int *number)
{
    char line[64];
    char *end;

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL)
        return 0;

    long v = strtol(line, &end, 10);

    if (end == line)
        return 0;

    while (*end == ' ' || *end == '\n' || *end == '\r')
        end++;

    if (*end != '\0')
        return 0;

    *number = (int)v;
    return 1;
}


int menu_choice_to_command(int choice, char *input, size_t size)
{
    static const char *with_value[] = { "LOAD", "ADD", "SUB", "PUSH" };
    int number;

    if (choice >= 1 && choice <= 4)
    {
        if (!read_number("Enter value: ", &number))
        {
            printf("\nThat is not a number. Command cancelled.\n");
            return 0;
        }

        snprintf(input, size, "%s %d", with_value[choice - 1], number);
        return 1;
    }

    if (choice == 6 || choice == 7)
    {
        if (!read_number("Enter memory address: ", &number))
        {
            printf("\nThat is not a number. Command cancelled.\n");
            return 0;
        }

        snprintf(input, size, "%s %d", choice == 6 ? "STORE" : "LOADM", number);
        return 1;
    }

    switch (choice)
    {
        case 0:  snprintf(input, size, "EXIT");        return 1;
        case 5:  snprintf(input, size, "POP");         return 1;
        case 8:  snprintf(input, size, "PRINT");       return 1;
        case 9:  snprintf(input, size, "SHOW CPU");    return 1;
        case 10: snprintf(input, size, "SHOW MEMORY"); return 1;
        case 11: snprintf(input, size, "SHOW STACK");  return 1;
        case 12: snprintf(input, size, "SHOW QUEUE");  return 1;
        case 13: return 2;
    }

    printf("\nInvalid choice. Please pick a number from 0 to 13.\n");
    return 0;
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
    if (write_full(sock_fd, msg, sizeof(Message)) == -1)
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

    ssize_t n = read_full(sock_fd, &response, sizeof(Message));

    if (n <= 0)
    {
        if (n == 0)
            printf("\nCore closed the connection.\n");
        else
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
       Semaphores: wait until Core is ready
       ----------------------------------------- */

    signal(SIGPIPE, SIG_IGN);

    if (sync_open() == -1)
    {
        printf("Make sure the Core process is running.\n");
        return EXIT_FAILURE;
    }

    if (wait_core_ready(10) == -1)
    {
        sync_close();
        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Shared memory: map Core's state
       ----------------------------------------- */

    int shm_fd = shm_open(SIM_SHM_NAME, O_RDWR, 0666);

    if (shm_fd == -1)
    {
        perror("Failed to open shared memory");
        sync_close();
        return EXIT_FAILURE;
    }

    shared = mmap(NULL, sizeof(SharedState),
                  PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    close(shm_fd);

    if (shared == MAP_FAILED)
    {
        perror("Failed to map shared memory");
        sync_close();
        return EXIT_FAILURE;
    }


    /* -----------------------------------------
       Socket: connect to Core
       ----------------------------------------- */

    struct sockaddr_un addr;

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock_fd == -1)
    {
        perror("socket");
        sync_close();
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SIM_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("Failed to connect to Core");
        printf("Make sure the Core process is running.\n");
        close(sock_fd);
        sync_close();
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

        printf("\nEnter your choice (0-13), or type a command: ");
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


        /* Menu number? Turn it into the matching command text. */

        int is_number = 1;

        for (size_t i = 0; input[i] != '\0'; i++)
        {
            if (!isdigit((unsigned char)input[i]))
                is_number = 0;
        }

        if (is_number)
        {
            int result = menu_choice_to_command(atoi(input), input, sizeof(input));

            if (result == 0)
                continue;

            if (result == 2)
            {
                show_live_state();
                continue;
            }
        }


        /* Typed commands are accepted in lower case too. */

        for (size_t i = 0; input[i] != '\0'; i++)
            input[i] = (char)toupper((unsigned char)input[i]);


        /* STATE is handled here: it reads shared memory, no message to Core */

        if (strcmp(input, "STATE") == 0)
        {
            show_live_state();
            continue;
        }


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

    close(sock_fd);
    munmap(shared, sizeof(SharedState));
    sync_close();


    printf("UI process terminated.\n");

    return EXIT_SUCCESS;
}
