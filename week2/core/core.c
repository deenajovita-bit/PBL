#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "ipc_common.h"


/* =========================================================
   CPU
   ========================================================= */

typedef struct
{
    int accumulator;
    int program_counter;

} CPU;


/* =========================================================
   MEMORY
   ========================================================= */

typedef struct
{
    int data[MEMORY_SIZE];

} Memory;


/* =========================================================
   STACK
   ========================================================= */

typedef struct
{
    int data[STACK_SIZE];
    int top;

} Stack;


/* =========================================================
   INSTRUCTION QUEUE
   ========================================================= */

typedef struct
{
    Message data[QUEUE_SIZE];

    int front;
    int rear;
    int count;

} InstructionQueue;


/* =========================================================
   GLOBAL CORE COMPONENTS
   ========================================================= */

CPU cpu;

Memory memory;

Stack stack;

InstructionQueue instruction_queue;


/* =========================================================
   MESSAGE QUEUE DESCRIPTORS
   ========================================================= */

mqd_t ui_to_core = (mqd_t)-1;

mqd_t core_to_ui = (mqd_t)-1;

mqd_t core_to_logger = (mqd_t)-1;


/* =========================================================
   CPU FUNCTIONS
   ========================================================= */

void cpu_init(void)
{
    cpu.accumulator = 0;
    cpu.program_counter = 0;
}


void cpu_load(int value)
{
    cpu.accumulator = value;
}


void cpu_add(int value)
{
    cpu.accumulator += value;
}


void cpu_sub(int value)
{
    cpu.accumulator -= value;
}


void cpu_increment_pc(void)
{
    cpu.program_counter++;
}


/* =========================================================
   MEMORY FUNCTIONS
   ========================================================= */

void memory_init(void)
{
    int i;

    for (i = 0; i < MEMORY_SIZE; i++)
    {
        memory.data[i] = 0;
    }
}


int memory_valid_address(int address)
{
    if (address >= 0 && address < MEMORY_SIZE)
    {
        return 1;
    }

    return 0;
}


int memory_store(int address, int value)
{
    if (!memory_valid_address(address))
    {
        return 0;
    }

    memory.data[address] = value;

    return 1;
}


int memory_load(int address, int *value)
{
    if (!memory_valid_address(address))
    {
        return 0;
    }

    *value = memory.data[address];

    return 1;
}


/* =========================================================
   STACK FUNCTIONS
   ========================================================= */

void stack_init(void)
{
    stack.top = -1;
}


int stack_is_empty(void)
{
    return stack.top == -1;
}


int stack_is_full(void)
{
    return stack.top == STACK_SIZE - 1;
}


int stack_push(int value)
{
    if (stack_is_full())
    {
        return 0;
    }

    stack.top++;

    stack.data[stack.top] = value;

    return 1;
}


int stack_pop(int *value)
{
    if (stack_is_empty())
    {
        return 0;
    }

    *value = stack.data[stack.top];

    stack.top--;

    return 1;
}


int stack_size(void)
{
    return stack.top + 1;
}


/* =========================================================
   INSTRUCTION QUEUE FUNCTIONS
   ========================================================= */

void queue_init(void)
{
    instruction_queue.front = 0;
    instruction_queue.rear = -1;
    instruction_queue.count = 0;
}


int queue_is_empty(void)
{
    return instruction_queue.count == 0;
}


int queue_is_full(void)
{
    return instruction_queue.count == QUEUE_SIZE;
}


int queue_enqueue(Message message)
{
    if (queue_is_full())
    {
        return 0;
    }

    instruction_queue.rear =
        (instruction_queue.rear + 1) % QUEUE_SIZE;

    instruction_queue.data[instruction_queue.rear] = message;

    instruction_queue.count++;

    return 1;
}


int queue_dequeue(Message *message)
{
    if (queue_is_empty())
    {
        return 0;
    }

    *message = instruction_queue.data[instruction_queue.front];

    instruction_queue.front =
        (instruction_queue.front + 1) % QUEUE_SIZE;

    instruction_queue.count--;

    return 1;
}


/* =========================================================
   SEND MESSAGE TO UI
   ========================================================= */

void send_to_ui(
    MessageType type,
    int value,
    int address,
    const char *text
)
{
    Message message;

    memset(&message, 0, sizeof(Message));

    message.type = type;

    message.value = value;

    message.address = address;

    if (text != NULL)
    {
        strncpy(
            message.data,
            text,
            MAX_DATA - 1
        );

        message.data[MAX_DATA - 1] = '\0';
    }


    if (mq_send(
            core_to_ui,
            (const char *)&message,
            sizeof(Message),
            0
        ) == -1)
    {
        perror("Error sending message to UI");
    }
}


/* =========================================================
   SEND MESSAGE TO LOGGER
   ========================================================= */

void send_to_logger(
    MessageType type,
    int value,
    int address,
    const char *text
)
{
    Message message;

    memset(&message, 0, sizeof(Message));

    message.type = type;

    message.value = value;

    message.address = address;

    if (text != NULL)
    {
        strncpy(
            message.data,
            text,
            MAX_DATA - 1
        );

        message.data[MAX_DATA - 1] = '\0';
    }


    if (mq_send(
            core_to_logger,
            (const char *)&message,
            sizeof(Message),
            0
        ) == -1)
    {
        perror("Error sending message to Logger");
    }
}


/* =========================================================
   SEND SUCCESS
   ========================================================= */

void send_success(
    int value,
    int address,
    const char *text
)
{
    send_to_ui(
        RESPONSE_SUCCESS,
        value,
        address,
        text
    );
}


/* =========================================================
   SEND ERROR
   ========================================================= */

void send_error(const char *text)
{
    send_to_ui(
        RESPONSE_ERROR,
        0,
        -1,
        text
    );

    send_to_logger(
        LOG_ERROR,
        0,
        -1,
        text
    );
}


/* =========================================================
   SHOW CPU
   ========================================================= */

void show_cpu(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "ACC=%d, PC=%d",
        cpu.accumulator,
        cpu.program_counter
    );


    send_to_ui(
        RESPONSE_CPU,
        cpu.accumulator,
        cpu.program_counter,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        cpu.accumulator,
        cpu.program_counter,
        "CPU state displayed."
    );
}


/* =========================================================
   SHOW MEMORY
   ========================================================= */

void show_memory(int address)
{
    char text[MAX_DATA];

    if (!memory_valid_address(address))
    {
        send_error("Invalid memory address.");

        return;
    }


    snprintf(
        text,
        MAX_DATA,
        "Memory[%d] = %d",
        address,
        memory.data[address]
    );


    send_to_ui(
        RESPONSE_MEMORY,
        memory.data[address],
        address,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        memory.data[address],
        address,
        "Memory value displayed."
    );
}


/* =========================================================
   SHOW STACK
   ========================================================= */

void show_stack(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "Stack size=%d, Top=%d",
        stack_size(),
        stack.top
    );


    send_to_ui(
        RESPONSE_STACK,
        stack_size(),
        stack.top,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        stack_size(),
        stack.top,
        "Stack state displayed."
    );
}


/* =========================================================
   SHOW INSTRUCTION QUEUE
   ========================================================= */

void show_queue(void)
{
    char text[MAX_DATA];

    snprintf(
        text,
        MAX_DATA,
        "Instruction Queue size=%d",
        instruction_queue.count
    );


    send_to_ui(
        RESPONSE_QUEUE,
        instruction_queue.count,
        instruction_queue.front,
        text
    );


    send_to_logger(
        LOG_SYSTEM,
        instruction_queue.count,
        instruction_queue.front,
        "Instruction queue state displayed."
    );
}


/* =========================================================
   EXECUTE COMMAND
   ========================================================= */

int execute_command(Message *command)
{
    int value;


    switch (command->type)
    {

        /* -------------------------------------------------
           LOAD
           ------------------------------------------------- */

        case CMD_LOAD:

            cpu_load(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "LOAD executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "LOAD executed."
            );

            break;


        /* -------------------------------------------------
           ADD
           ------------------------------------------------- */

        case CMD_ADD:

            cpu_add(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "ADD executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "ADD executed."
            );

            break;


        /* -------------------------------------------------
           SUB
           ------------------------------------------------- */

        case CMD_SUB:

            cpu_sub(command->value);

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "SUB executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                -1,
                "SUB executed."
            );

            break;


        /* -------------------------------------------------
           PUSH
           ------------------------------------------------- */

        case CMD_PUSH:

            if (!stack_push(command->value))
            {
                send_error(
                    "Stack overflow. PUSH failed."
                );

                return 0;
            }


            cpu_increment_pc();

            send_success(
                command->value,
                stack.top,
                "PUSH executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                command->value,
                stack.top,
                "PUSH executed."
            );

            break;


        /* -------------------------------------------------
           POP
           ------------------------------------------------- */

        case CMD_POP:

            if (!stack_pop(&value))
            {
                send_error(
                    "Stack underflow. POP failed."
                );

                return 0;
            }


            cpu.accumulator = value;

            cpu_increment_pc();

            send_success(
                value,
                stack.top,
                "POP executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                value,
                stack.top,
                "POP executed."
            );

            break;


        /* -------------------------------------------------
           STORE
           ------------------------------------------------- */

        case CMD_STORE:

            if (!memory_store(
                    command->address,
                    cpu.accumulator
                ))
            {
                send_error(
                    "Invalid memory address. STORE failed."
                );

                return 0;
            }


            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                command->address,
                "STORE executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                cpu.accumulator,
                command->address,
                "STORE executed."
            );

            break;


        /* -------------------------------------------------
           LOADM
           ------------------------------------------------- */

        case CMD_LOADM:

            if (!memory_load(
                    command->address,
                    &value
                ))
            {
                send_error(
                    "Invalid memory address. LOADM failed."
                );

                return 0;
            }


            cpu.accumulator = value;

            cpu_increment_pc();

            send_success(
                value,
                command->address,
                "LOADM executed successfully."
            );

            send_to_logger(
                LOG_EXECUTION,
                value,
                command->address,
                "LOADM executed."
            );

            break;


        /* -------------------------------------------------
           PRINT
           ------------------------------------------------- */

        case CMD_PRINT:

            cpu_increment_pc();

            send_success(
                cpu.accumulator,
                -1,
                "PRINT executed."
            );

            send_to_logger(
                LOG_EXECUTION,
                cpu.accumulator,
                -1,
                "PRINT executed."
            );

            break;


        /* -------------------------------------------------
           SHOW CPU
           ------------------------------------------------- */

        case CMD_SHOW_CPU:

            show_cpu();

            break;


        /* -------------------------------------------------
           SHOW MEMORY
           ------------------------------------------------- */

        case CMD_SHOW_MEMORY:

            show_memory(command->address);

            break;


        /* -------------------------------------------------
           SHOW STACK
           ------------------------------------------------- */

        case CMD_SHOW_STACK:

            show_stack();

            break;


        /* -------------------------------------------------
           SHOW QUEUE
           ------------------------------------------------- */

        case CMD_SHOW_QUEUE:

            show_queue();

            break;


        /* -------------------------------------------------
           EXIT
           ------------------------------------------------- */

        case CMD_EXIT:

            send_success(
                0,
                -1,
                "Core received EXIT command."
            );

            send_to_logger(
                LOG_SYSTEM,
                0,
                -1,
                "Core received EXIT command."
            );

            return 1;


        /* -------------------------------------------------
           INVALID COMMAND
           ------------------------------------------------- */

        default:

            send_error(
                "Invalid command received by Core."
            );

            return 0;
    }


    return 0;
}


/* =========================================================
   OPEN MESSAGE QUEUES
   ========================================================= */

int open_queues(void)
{
    struct mq_attr attributes;

    memset(
        &attributes,
        0,
        sizeof(attributes)
    );


    /*
     * POSIX message queue attributes.
     *
     * This is separate from QUEUE_SIZE,
     * which is the Core's internal instruction queue.
     */

    attributes.mq_maxmsg = 10;

    attributes.mq_msgsize = sizeof(Message);


    /* UI → CORE */

    ui_to_core = mq_open(
        UI_TO_CORE_QUEUE,
        O_CREAT | O_RDONLY,
        0666,
        &attributes
    );


    if (ui_to_core == (mqd_t)-1)
    {
        perror("mq_open ui_to_core");

        return 0;
    }


    /* CORE → UI */

    core_to_ui = mq_open(
        CORE_TO_UI_QUEUE,
        O_CREAT | O_WRONLY,
        0666,
        &attributes
    );


    if (core_to_ui == (mqd_t)-1)
    {
        perror("mq_open core_to_ui");

        mq_close(ui_to_core);

        return 0;
    }


    /* CORE → LOGGER */

    core_to_logger = mq_open(
        CORE_TO_LOGGER_QUEUE,
        O_CREAT | O_WRONLY,
        0666,
        &attributes
    );


    if (core_to_logger == (mqd_t)-1)
    {
        perror("mq_open core_to_logger");

        mq_close(ui_to_core);
        mq_close(core_to_ui);

        return 0;
    }


    return 1;
}


/* =========================================================
   CLOSE MESSAGE QUEUES
   ========================================================= */

void close_queues(void)
{
    if (ui_to_core != (mqd_t)-1)
    {
        mq_close(ui_to_core);
    }


    if (core_to_ui != (mqd_t)-1)
    {
        mq_close(core_to_ui);
    }


    if (core_to_logger != (mqd_t)-1)
    {
        mq_close(core_to_logger);
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    Message command;

    ssize_t bytes_received;

    int running = 1;


    printf("\n");
    printf("========================================\n");
    printf("       CPU SIMULATOR - CORE PROCESS\n");
    printf("========================================\n");


    /* -----------------------------------------
       Initialize CPU
       ----------------------------------------- */

    cpu_init();


    /* -----------------------------------------
       Initialize Memory
       ----------------------------------------- */

    memory_init();


    /* -----------------------------------------
       Initialize Stack
       ----------------------------------------- */

    stack_init();


    /* -----------------------------------------
       Initialize Instruction Queue
       ----------------------------------------- */

    queue_init();


    /* -----------------------------------------
       Open POSIX Message Queues
       ----------------------------------------- */

    if (!open_queues())
    {
        fprintf(
            stderr,
            "Failed to open POSIX message queues.\n"
        );

        return EXIT_FAILURE;
    }


    printf("Core initialized successfully.\n");
    printf("Waiting for commands from UI...\n");


    send_to_logger(
        LOG_SYSTEM,
        0,
        -1,
        "Core process started."
    );


    /* =================================================
       MAIN LOOP
       ================================================= */

    while (running)
    {

        memset(
            &command,
            0,
            sizeof(Message)
        );


        /*
         * Wait for a command from UI.
         */

        bytes_received = mq_receive(
            ui_to_core,
            (char *)&command,
            sizeof(Message),
            NULL
        );


        if (bytes_received == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }


            perror("mq_receive");


            send_to_logger(
                LOG_ERROR,
                0,
                -1,
                "Failed to receive command from UI."
            );


            break;
        }


        /*
         * Verify message size.
         */

        if ((size_t)bytes_received != sizeof(Message))
        {
            send_error(
                "Invalid message size."
            );

            continue;
        }


        /*
         * Add received command to
         * the internal instruction queue.
         */

        if (!queue_enqueue(command))
        {
            send_error(
                "Instruction queue is full."
            );

            continue;
        }


        /*
         * Fetch command from internal
         * instruction queue.
         */

        if (!queue_dequeue(&command))
        {
            send_error(
                "Unable to fetch instruction."
            );

            continue;
        }


        /*
         * Execute command.
         */

        if (execute_command(&command) == 1)
        {
            running = 0;
        }
    }


    /* =================================================
       SHUTDOWN
       ================================================= */

    send_to_logger(
        LOG_SYSTEM,
        0,
        -1,
        "Core process shutting down."
    );


    close_queues();


    printf("\n");
    printf("Core process terminated.\n");


    return EXIT_SUCCESS;
}
