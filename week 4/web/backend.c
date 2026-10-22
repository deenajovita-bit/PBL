#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../deadlock.h"

#define PORT 8001
#define BUFFER_SIZE 8192

/* Persistent simulation state */
static System system_state_data;

/* =========================================================
   HTTP RESPONSE
   ========================================================= */

static void send_response(int client,
                          int status,
                          const char *content_type,
                          const char *body)
{
    char header[1024];

    const char *reason =
        status == 200 ? "OK" : "Bad Request";

    int length = snprintf(
        header,
        sizeof(header),

        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "Content-Length: %zu\r\n\r\n",

        status,
        reason,
        content_type,
        strlen(body)
    );

    (void)write(client, header, (size_t)length);
    (void)write(client, body, strlen(body));
}


/* =========================================================
   DEMO SYSTEM
   ========================================================= */

static void setup_demo(System *s)
{
    static const int total[3] = {
        10, 5, 7
    };

    static const int allocation[5][3] = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };

    static const int maximum[5][3] = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };

    int i;
    int j;

    memset(s, 0, sizeof(*s));

    s->n = 5;
    s->m = 3;

    for (j = 0; j < s->m; j++) {
        s->total[j] = total[j];
    }

    for (i = 0; i < s->n; i++) {

        s->active[i] = 1;

        for (j = 0; j < s->m; j++) {

            s->alloc[i][j] =
                allocation[i][j];

            s->max[i][j] =
                maximum[i][j];
        }
    }

    update_available(s);
    calc_need(s);
}


/* =========================================================
   STATE JSON
   ========================================================= */

static void build_state_json(char *body,
                             size_t size)
{
    size_t used = 0;

    int i;
    int j;

    System *s = &system_state_data;

    used += (size_t)snprintf(
        body + used,
        size - used,

        "{"
        "\"processes\":%d,"
        "\"resources\":%d,"
        "\"total\":[",

        s->n,
        s->m
    );


    /* TOTAL */

    for (j = 0; j < s->m; j++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s%d",

            j ? "," : "",
            s->total[j]
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"available\":["
    );


    /* AVAILABLE */

    for (j = 0; j < s->m; j++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s%d",

            j ? "," : "",
            s->available[j]
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"allocation\":["
    );


    /* ALLOCATION */

    for (i = 0; i < s->n; i++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s[",

            i ? "," : ""
        );

        for (j = 0; j < s->m; j++) {

            used += (size_t)snprintf(
                body + used,
                size - used,

                "%s%d",

                j ? "," : "",
                s->alloc[i][j]
            );
        }

        used += (size_t)snprintf(
            body + used,
            size - used,
            "]"
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"maximum\":["
    );


    /* MAXIMUM */

    for (i = 0; i < s->n; i++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s[",

            i ? "," : ""
        );

        for (j = 0; j < s->m; j++) {

            used += (size_t)snprintf(
                body + used,
                size - used,

                "%s%d",

                j ? "," : "",
                s->max[i][j]
            );
        }

        used += (size_t)snprintf(
            body + used,
            size - used,
            "]"
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"need\":["
    );


    /* NEED */

    for (i = 0; i < s->n; i++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s[",

            i ? "," : ""
        );

        for (j = 0; j < s->m; j++) {

            used += (size_t)snprintf(
                body + used,
                size - used,

                "%s%d",

                j ? "," : "",
                s->need[i][j]
            );
        }

        used += (size_t)snprintf(
            body + used,
            size - used,
            "]"
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"request\":["
    );


    /* PENDING REQUEST */

    for (i = 0; i < s->n; i++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s[",

            i ? "," : ""
        );

        for (j = 0; j < s->m; j++) {

            used += (size_t)snprintf(
                body + used,
                size - used,

                "%s%d",

                j ? "," : "",
                s->request[i][j]
            );
        }

        used += (size_t)snprintf(
            body + used,
            size - used,
            "]"
        );
    }


    used += (size_t)snprintf(
        body + used,
        size - used,

        "],\"active\":["
    );


    /* ACTIVE PROCESSES */

    for (i = 0; i < s->n; i++) {

        used += (size_t)snprintf(
            body + used,
            size - used,

            "%s%d",

            i ? "," : "",
            s->active[i]
        );
    }


    snprintf(
        body + used,
        size - used,
        "]}"
    );
}


/* =========================================================
   SIMPLE QUERY PARAMETER READER
   Example:
   /api/request?p=1&r0=1&r1=0&r2=2&banker=1
   ========================================================= */

static int get_query_int(const char *request,
                         const char *name,
                         int default_value)
{
    char pattern[64];

    const char *p;

    snprintf(
        pattern,
        sizeof(pattern),
        "%s=",
        name
    );

    p = strstr(request, pattern);

    if (p == NULL) {
        return default_value;
    }

    p += strlen(pattern);

    return atoi(p);
}


/* =========================================================
   REQUEST HANDLER
   ========================================================= */

static void handle_request(int client,
                           const char *request)
{
    char body[BUFFER_SIZE];


    /* CORS */

    if (strncmp(request, "OPTIONS ", 8) == 0) {

        send_response(
            client,
            200,
            "text/plain",
            ""
        );

        return;
    }


    /* =====================================================
       GET STATE
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/state ",
            15
        ) == 0)
    {
        build_state_json(
            body,
            sizeof(body)
        );

        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }


    /* =====================================================
       BANKER'S ALGORITHM
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/safety ",
            16
        ) == 0)
    {
        int sequence[MAX_P];

        int safe =
            is_safe(
                &system_state_data,
                sequence
            );

        size_t used = 0;

        int i;


        used += (size_t)snprintf(
            body + used,
            sizeof(body) - used,

            "{\"safe\":%s,"
            "\"sequence\":[",

            safe ? "true" : "false"
        );


        if (safe) {

            for (i = 0;
                 i < system_state_data.n;
                 i++)
            {
                if (!system_state_data.active[
                        sequence[i]
                    ])
                {
                    continue;
                }

                used += (size_t)snprintf(
                    body + used,
                    sizeof(body) - used,

                    "%s\"P%d\"",

                    used &&
                    body[used - 1] != '['
                        ? ","
                        : "",

                    sequence[i]
                );
            }
        }


        snprintf(
            body + used,
            sizeof(body) - used,
            "]}"
        );


        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }


    /* =====================================================
       DEADLOCK DETECTION
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/detection ",
            19
        ) == 0)
    {
        int dead[MAX_P];

        int count =
            detect_deadlock(
                &system_state_data,
                dead
            );

        size_t used = 0;

        int i;


        used += (size_t)snprintf(
            body + used,
            sizeof(body) - used,

            "{\"deadlock\":%s,"
            "\"count\":%d,"
            "\"processes\":[",

            count > 0
                ? "true"
                : "false",

            count
        );


        for (i = 0; i < count; i++) {

            used += (size_t)snprintf(
                body + used,
                sizeof(body) - used,

                "%s\"P%d\"",

                i ? "," : "",
                dead[i]
            );
        }


        snprintf(
            body + used,
            sizeof(body) - used,
            "]}"
        );


        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }


    /* =====================================================
       RESOURCE REQUEST

       Example:
       /api/request?p=1&r0=1&r1=0&r2=1&banker=1
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/request?",
            17
        ) == 0)
    {
        int p;

        int req[MAX_R] = {0};

        int use_banker;

        ReqResult result;

        int j;


        p =
            get_query_int(
                request,
                "p",
                -1
            );


        for (j = 0;
             j < system_state_data.m;
             j++)
        {
            char parameter[16];

            snprintf(
                parameter,
                sizeof(parameter),
                "r%d",
                j
            );

            req[j] =
                get_query_int(
                    request,
                    parameter,
                    0
                );
        }


        use_banker =
            get_query_int(
                request,
                "banker",
                1
            );


        result =
            request_resources(
                &system_state_data,
                p,
                req,
                use_banker
            );


        if (result == REQ_GRANTED) {

            snprintf(
                body,
                sizeof(body),

                "{"
                "\"result\":\"granted\","
                "\"message\":"
                "\"Request granted successfully.\""
                "}"
            );
        }

        else if (result == REQ_WAIT) {

            snprintf(
                body,
                sizeof(body),

                "{"
                "\"result\":\"waiting\","
                "\"message\":"
                "\"Not enough resources are available. Process is waiting.\""
                "}"
            );
        }

        else if (
            result ==
            REQ_DENIED_UNSAFE
        )
        {
            snprintf(
                body,
                sizeof(body),

                "{"
                "\"result\":\"unsafe\","
                "\"message\":"
                "\"Request denied because it would create an unsafe state.\""
                "}"
            );
        }

        else {

            snprintf(
                body,
                sizeof(body),

                "{"
                "\"result\":\"error\","
                "\"message\":"
                "\"Invalid resource request.\""
                "}"
            );
        }


        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }


    /* =====================================================
       TERMINATION RECOVERY

       Automatically chooses the cheapest deadlocked victim.
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/recovery/terminate ",
            28
        ) == 0)
    {
        int dead[MAX_P];

        int count =
            detect_deadlock(
                &system_state_data,
                dead
            );


        if (count <= 0) {

            snprintf(
                body,
                sizeof(body),

                "{"
                "\"recovered\":false,"
                "\"message\":"
                "\"No deadlock exists. Recovery is not required.\""
                "}"
            );
        }

        else {

            int victim =
                pick_victim(
                    &system_state_data,
                    dead,
                    count
                );


            if (victim < 0) {

                snprintf(
                    body,
                    sizeof(body),

                    "{"
                    "\"recovered\":false,"
                    "\"message\":"
                    "\"No valid recovery victim was found.\""
                    "}"
                );
            }

            else {

                recover_terminate(
                    &system_state_data,
                    victim
                );


                snprintf(
                    body,
                    sizeof(body),

                    "{"
                    "\"recovered\":true,"
                    "\"victim\":\"P%d\","
                    "\"message\":"
                    "\"Process P%d was terminated and its resources were released.\""
                    "}",

                    victim,
                    victim
                );
            }
        }


        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }
    /* =====================================================
       RESOURCE PREEMPTION RECOVERY

       Takes resources from one deadlocked process
       without terminating the process.
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/recovery/preempt ",
            strlen("GET /api/recovery/preempt ")
        ) == 0)
    {
        int dead[MAX_P];

        int count =
            detect_deadlock(
                &system_state_data,
                dead
            );


        if (count <= 0) {

            snprintf(
                body,
                sizeof(body),

                "{"
                "\"recovered\":false,"
                "\"message\":"
                "\"No deadlock exists. Preemption is not required.\""
                "}"
            );
        }

        else {

            int victim =
                pick_victim(
                    &system_state_data,
                    dead,
                    count
                );


            if (victim < 0) {

                snprintf(
                    body,
                    sizeof(body),

                    "{"
                    "\"recovered\":false,"
                    "\"message\":"
                    "\"No valid process was found for preemption.\""
                    "}"
                );
            }

            else {

                int success =
                    recover_preempt(
                        &system_state_data,
                        victim
                    );


                if (success) {

                    snprintf(
                        body,
                        sizeof(body),

                        "{"
                        "\"recovered\":true,"
                        "\"victim\":\"P%d\","
                        "\"message\":"
                        "\"Resources were preempted from P%d. The process remains active.\""
                        "}",

                        victim,
                        victim
                    );
                }

                else {

                    snprintf(
                        body,
                        sizeof(body),

                        "{"
                        "\"recovered\":false,"
                        "\"message\":"
                        "\"The selected process had no resources to preempt.\""
                        "}"
                    );
                }
            }
        }


        send_response(
            client,
            200,
            "application/json",
            body
        );

        return;
    }

    /* =====================================================
       RESET SYSTEM
       ===================================================== */

    if (strncmp(
            request,
            "GET /api/reset ",
            15
        ) == 0)
    {
        setup_demo(
            &system_state_data
        );

        send_response(
            client,
            200,
            "application/json",

            "{"
            "\"reset\":true,"
            "\"message\":"
            "\"Simulation reset successfully.\""
            "}"
        );

        return;
    }/* ==========================================
   REAL UNSAFE EXAMPLE
   ========================================== */

if (strncmp(
        request,
        "GET /api/example/unsafe ",
        strlen("GET /api/example/unsafe ")
    ) == 0)
{
    int total[3] = {3, 0, 0};

    /*
       2 processes, 3 resource types.

       Total R0 = 3

       P0 holds 1, needs 2 more
       P1 holds 1, needs 2 more

       Available R0 = 1

       Neither process can finish first.

       Therefore:
       UNSAFE but NOT DEADLOCKED.
    */

    sys_init(
        &system_state_data,
        2,
        3,
        total
    );

    /* Allocation */

    system_state_data.alloc[0][0] = 1;
    system_state_data.alloc[1][0] = 1;


    /* Maximum */

    system_state_data.max[0][0] = 3;
    system_state_data.max[1][0] = 3;


    calc_need(&system_state_data);
    update_available(&system_state_data);


    send_response(
        client,
        200,
        "application/json",

        "{"
        "\"loaded\":true,"
        "\"message\":"
        "\"Real unsafe example loaded.\""
        "}"
    );

    return;
}
/* ==========================================
   REAL DEADLOCK EXAMPLE
   ========================================== */

if (strncmp(
        request,
        "GET /api/example/deadlock ",
        strlen("GET /api/example/deadlock ")
    ) == 0)
{
    int total[3] = {1, 1, 0};

    /* Create 2 processes and 3 resource types */
    sys_init(
        &system_state_data,
        2,
        3,
        total
    );

    /*
       P0 holds R0 and waits for R1
       P1 holds R1 and waits for R0

       P0: R0 -> P0 -> R1
       P1: R1 -> P1 -> R0

       Circular wait = DEADLOCK
    */

    system_state_data.alloc[0][0] = 1;
    system_state_data.alloc[0][1] = 0;
    system_state_data.alloc[0][2] = 0;

    system_state_data.alloc[1][0] = 0;
    system_state_data.alloc[1][1] = 1;
    system_state_data.alloc[1][2] = 0;


    /* Maximum requirements */

    system_state_data.max[0][0] = 1;
    system_state_data.max[0][1] = 1;
    system_state_data.max[0][2] = 0;

    system_state_data.max[1][0] = 1;
    system_state_data.max[1][1] = 1;
    system_state_data.max[1][2] = 0;


    /* Waiting requests */

    system_state_data.request[0][0] = 0;
    system_state_data.request[0][1] = 1;
    system_state_data.request[0][2] = 0;

    system_state_data.request[1][0] = 1;
    system_state_data.request[1][1] = 0;
    system_state_data.request[1][2] = 0;


    /* Calculate Need + Available */

    calc_need(&system_state_data);
    update_available(&system_state_data);


    send_response(
        client,
        200,
        "application/json",

        "{"
        "\"loaded\":true,"
        "\"message\":"
        "\"Real deadlock example loaded.\""
        "}"
    );

    return;
}

    /* =====================================================
       HOME
       ===================================================== */

    send_response(
        client,
        200,
        "application/json",

        "{"
        "\"message\":"
        "\"Deadlock simulation C backend is running.\","

        "\"endpoints\":["
        "\"/api/state\","
        "\"/api/safety\","
        "\"/api/detection\","
        "\"/api/request\","
        "\"/api/recovery/terminate\","
        "\"/api/reset\""
        "]"
        "}"
    );
}


/* =========================================================
   MAIN SERVER
   ========================================================= */

int main(void)
{
    int server;

    int opt = 1;

    struct sockaddr_in address;


    /* Load initial system ONCE */
    setup_demo(
        &system_state_data
    );


    server =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );


    if (server < 0) {

        perror("socket");

        return 1;
    }


    if (setsockopt(
            server,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)
        ) < 0)
    {
        perror("setsockopt");

        close(server);

        return 1;
    }


    memset(
        &address,
        0,
        sizeof(address)
    );


    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        htonl(INADDR_ANY);

    address.sin_port =
        htons(PORT);


    if (bind(
            server,
            (struct sockaddr *)&address,
            sizeof(address)
        ) < 0)
    {
        perror("bind");

        close(server);

        return 1;
    }


    if (listen(
            server,
            10
        ) < 0)
    {
        perror("listen");

        close(server);

        return 1;
    }


    printf(
        "\n========================================\n"
    );

    printf(
        " DEADLOCK SIMULATION C BACKEND\n"
    );

    printf(
        "========================================\n"
    );

    printf(
        "Listening on port %d\n",
        PORT
    );

    printf(
        "\nAvailable endpoints:\n"
    );

    printf(
        "  /api/state\n"
    );

    printf(
        "  /api/safety\n"
    );

    printf(
        "  /api/detection\n"
    );

    printf(
        "  /api/request\n"
    );

    printf(
        "  /api/recovery/terminate\n"
    );

    printf(
        "  /api/reset\n\n"
    );


    for (;;) {

        int client =
            accept(
                server,
                NULL,
                NULL
            );

        char request[
            BUFFER_SIZE
        ];

        ssize_t bytes;


        if (client < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("accept");

            continue;
        }


        bytes =
            read(
                client,
                request,
                sizeof(request) - 1
            );


        if (bytes > 0) {

            request[bytes] =
                '\0';

            handle_request(
                client,
                request
            );
        }


        close(client);
    }


    close(server);

    return 0;
}