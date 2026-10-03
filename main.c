#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define CLOSE_SOCKET closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int socket_t;
#define CLOSE_SOCKET close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

#define TOTAL_SEATS 10
#define SERVER_PORT 8080
#define REQUEST_BUFFER_SIZE 16384

typedef struct Seat {
    int seatNo;
    int booked;
    char customerName[50];
    struct Seat *next;
} Seat;

Seat *createSeat(int seatNo) {
    Seat *newSeat = (Seat *)malloc(sizeof(Seat));
    if (newSeat == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    newSeat->seatNo = seatNo;
    newSeat->booked = 0;
    strcpy(newSeat->customerName, "Available");
    newSeat->next = NULL;
    return newSeat;
}

void insertSeat(Seat **head, int seatNo) {
    Seat *newSeat = createSeat(seatNo);
    if (*head == NULL) {
        *head = newSeat;
        return;
    }

    Seat *temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newSeat;
}

Seat *findSeat(Seat *head, int seatNo) {
    while (head != NULL) {
        if (head->seatNo == seatNo) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void reserveSeat(Seat *head, int seatNo, const char *name) {
    Seat *seat = findSeat(head, seatNo);
    if (seat == NULL) {
        printf("Seat %d does not exist.\n", seatNo);
        return;
    }

    if (seat->booked) {
        printf("Seat %d is already allocated to %s.\n", seatNo, seat->customerName);
        return;
    }

    seat->booked = 1;
    strncpy(seat->customerName, name, sizeof(seat->customerName) - 1);
    seat->customerName[sizeof(seat->customerName) - 1] = '\0';
    printf("Seat %d booked successfully for %s.\n", seatNo, seat->customerName);
}

void cancelSeat(Seat *head, int seatNo) {
    Seat *seat = findSeat(head, seatNo);
    if (seat == NULL) {
        printf("Seat %d does not exist.\n", seatNo);
        return;
    }

    if (!seat->booked) {
        printf("Seat %d is already available.\n", seatNo);
        return;
    }

    seat->booked = 0;
    strcpy(seat->customerName, "Available");
    printf("Reservation for seat %d cancelled.\n", seatNo);
}

void displaySeats(Seat *head) {
    printf("\nTheatre Seat Layout:\n");
    printf("--------------------\n");

    while (head != NULL) {
        printf("Seat %d: %s\n", head->seatNo, head->booked ? head->customerName : "Available");
        head = head->next;
    }
    printf("--------------------\n");
}

void displayAvailableSeats(Seat *head) {
    int found = 0;
    printf("\nAvailable Seats: ");

    while (head != NULL) {
        if (!head->booked) {
            printf("%d ", head->seatNo);
            found = 1;
        }
        head = head->next;
    }

    if (!found) {
        printf("None");
    }
    printf("\n");
}

void freeSeats(Seat *head) {
    Seat *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

static int sendAll(socket_t client, const char *data, size_t length) {
    size_t sent = 0;
    while (sent < length) {
        int result = send(client, data + sent, (int)(length - sent), 0);
        if (result == SOCKET_ERROR || result == 0) {
            return 0;
        }
        sent += (size_t)result;
    }
    return 1;
}

static void sendResponse(socket_t client, int status, const char *reason,
                         const char *contentType, const char *body, size_t bodyLength) {
    char header[512];
    int headerLength = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n",
        status, reason, contentType, (unsigned int)bodyLength);
    if (headerLength > 0 && (size_t)headerLength < sizeof(header)) {
        sendAll(client, header, (size_t)headerLength);
        sendAll(client, body, bodyLength);
    }
}

static void sendJson(socket_t client, int status, const char *reason, const char *message) {
    char body[256];
    int bodyLength = snprintf(body, sizeof(body), "{\"message\":\"%s\"}", message);
    if (bodyLength < 0 || (size_t)bodyLength >= sizeof(body)) {
        const char fallback[] = "{\"message\":\"Unable to create response.\"}";
        sendResponse(client, 500, "Internal Server Error", "application/json; charset=utf-8",
                     fallback, sizeof(fallback) - 1);
        return;
    }
    sendResponse(client, status, reason, "application/json; charset=utf-8", body, (size_t)bodyLength);
}

static int appendText(char *buffer, size_t capacity, size_t *used, const char *text) {
    size_t length = strlen(text);
    if (length >= capacity - *used) {
        return 0;
    }
    memcpy(buffer + *used, text, length);
    *used += length;
    buffer[*used] = '\0';
    return 1;
}

static int appendJsonString(char *buffer, size_t capacity, size_t *used, const char *text) {
    if (!appendText(buffer, capacity, used, "\"")) {
        return 0;
    }
    while (*text != '\0') {
        unsigned char ch = (unsigned char)*text++;
        char escaped[7];
        const char *piece = NULL;
        switch (ch) {
            case '"': piece = "\\\""; break;
            case '\\': piece = "\\\\"; break;
            case '\b': piece = "\\b"; break;
            case '\f': piece = "\\f"; break;
            case '\n': piece = "\\n"; break;
            case '\r': piece = "\\r"; break;
            case '\t': piece = "\\t"; break;
            default:
                if (ch < 0x20) {
                    snprintf(escaped, sizeof(escaped), "\\u%04x", ch);
                    piece = escaped;
                } else {
                    escaped[0] = (char)ch;
                    escaped[1] = '\0';
                    piece = escaped;
                }
        }
        if (!appendText(buffer, capacity, used, piece)) {
            return 0;
        }
    }
    return appendText(buffer, capacity, used, "\"");
}

static void sendSeatList(socket_t client, Seat *head) {
    char body[4096] = "[";
    size_t used = 1;
    int first = 1;
    char seatInfo[96];

    while (head != NULL) {
        if (!first && !appendText(body, sizeof(body), &used, ",")) {
            sendJson(client, 500, "Internal Server Error", "Unable to create seat list.");
            return;
        }
        first = 0;
        int length = snprintf(seatInfo, sizeof(seatInfo),
            "{\"seatNo\":%d,\"booked\":%s,\"customerName\":",
            head->seatNo, head->booked ? "true" : "false");
        if (length < 0 || (size_t)length >= sizeof(seatInfo)
            || !appendText(body, sizeof(body), &used, seatInfo)
            || !appendJsonString(body, sizeof(body), &used, head->customerName)
            || !appendText(body, sizeof(body), &used, "}")) {
            sendJson(client, 500, "Internal Server Error", "Unable to create seat list.");
            return;
        }
        head = head->next;
    }

    if (!appendText(body, sizeof(body), &used, "]")) {
        sendJson(client, 500, "Internal Server Error", "Unable to create seat list.");
        return;
    }
    sendResponse(client, 200, "OK", "application/json; charset=utf-8", body, used);
}

static int getIntegerField(const char *body, const char *field, int *value) {
    char key[64];
    int keyLength = snprintf(key, sizeof(key), "\"%s\"", field);
    if (keyLength < 0 || (size_t)keyLength >= sizeof(key)) {
        return 0;
    }
    const char *position = strstr(body, key);
    if (position == NULL) {
        return 0;
    }
    position = strchr(position + keyLength, ':');
    if (position == NULL) {
        return 0;
    }
    position++;
    while (isspace((unsigned char)*position)) {
        position++;
    }
    char *end = NULL;
    long parsed = strtol(position, &end, 10);
    if (end == position || parsed < 1 || parsed > TOTAL_SEATS) {
        return 0;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != ',' && *end != '}' && *end != '\0') {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int getStringField(const char *body, const char *field, char *output, size_t capacity) {
    char key[64];
    int keyLength = snprintf(key, sizeof(key), "\"%s\"", field);
    if (keyLength < 0 || (size_t)keyLength >= sizeof(key)) {
        return 0;
    }
    const char *position = strstr(body, key);
    if (position == NULL) {
        return 0;
    }
    position = strchr(position + keyLength, ':');
    if (position == NULL) {
        return 0;
    }
    position++;
    while (isspace((unsigned char)*position)) {
        position++;
    }
    if (*position++ != '"') {
        return 0;
    }

    size_t used = 0;
    while (*position != '\0' && *position != '"') {
        unsigned char ch = (unsigned char)*position++;
        if (ch == '\\') {
            ch = (unsigned char)*position++;
            switch (ch) {
                case '"': case '\\': case '/': break;
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                default: return 0;
            }
        }
        if (ch < 0x20 || used + 1 >= capacity) {
            return 0;
        }
        output[used++] = (char)ch;
    }
    if (*position != '"' || used == 0) {
        return 0;
    }
    output[used] = '\0';
    return 1;
}

static int sendFile(socket_t client, const char *fileName, const char *contentType) {
    FILE *file = fopen(fileName, "rb");
    if (file == NULL) {
        return 0;
    }
    char content[262144];
    size_t length = fread(content, 1, sizeof(content), file);
    int failed = ferror(file);
    fclose(file);
    if (failed) {
        return 0;
    }
    sendResponse(client, 200, "OK", contentType, content, length);
    return 1;
}

static size_t findHeaderEnd(const char *request, size_t length) {
    size_t index;
    for (index = 0; index + 3 < length; index++) {
        if (request[index] == '\r' && request[index + 1] == '\n'
            && request[index + 2] == '\r' && request[index + 3] == '\n') {
            return index + 4;
        }
    }
    return 0;
}

static size_t getContentLength(const char *request) {
    const char *line = strstr(request, "\r\n");
    if (line == NULL) {
        return 0;
    }
    line += 2;
    while (*line != '\0' && !(line[0] == '\r' && line[1] == '\n')) {
        const char *lineEnd = strstr(line, "\r\n");
        if (lineEnd == NULL) {
            break;
        }
        int isContentLength = 1;
        const char expected[] = "Content-Length:";
        size_t headerIndex;
        for (headerIndex = 0; headerIndex < sizeof(expected) - 1; headerIndex++) {
            if (tolower((unsigned char)line[headerIndex]) != tolower((unsigned char)expected[headerIndex])) {
                isContentLength = 0;
                break;
            }
        }
        if (isContentLength) {
            const char *value = line + 15;
            while (isspace((unsigned char)*value)) {
                value++;
            }
            char *end = NULL;
            unsigned long parsed = strtoul(value, &end, 10);
            if (end == value || parsed > REQUEST_BUFFER_SIZE) {
                return REQUEST_BUFFER_SIZE + 1;
            }
            return (size_t)parsed;
        }
        line = lineEnd + 2;
    }
    return 0;
}

static void handleRequest(socket_t client, Seat *head) {
    char request[REQUEST_BUFFER_SIZE];
    size_t received = 0;
    size_t headerEnd = 0;
    size_t contentLength = 0;

    while (received < sizeof(request) - 1) {
        int count = recv(client, request + received, (int)(sizeof(request) - 1 - received), 0);
        if (count <= 0) {
            return;
        }
        received += (size_t)count;
        request[received] = '\0';
        if (headerEnd == 0) {
            headerEnd = findHeaderEnd(request, received);
            if (headerEnd != 0) {
                contentLength = getContentLength(request);
                if (contentLength > REQUEST_BUFFER_SIZE) {
                    sendJson(client, 413, "Payload Too Large", "Request body is too large.");
                    return;
                }
            }
        }
        if (headerEnd != 0 && received >= headerEnd + contentLength) {
            break;
        }
    }

    if (headerEnd == 0 || received < headerEnd + contentLength) {
        sendJson(client, 400, "Bad Request", "Incomplete HTTP request.");
        return;
    }

    char method[8];
    char path[256];
    if (sscanf(request, "%7s %255s", method, path) != 2) {
        sendJson(client, 400, "Bad Request", "Invalid HTTP request.");
        return;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/seats") == 0) {
        sendSeatList(client, head);
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/reservations") == 0) {
        char body[REQUEST_BUFFER_SIZE];
        char customerName[50];
        int seatNo;
        memcpy(body, request + headerEnd, contentLength);
        body[contentLength] = '\0';
        if (!getIntegerField(body, "seatNo", &seatNo)
            || !getStringField(body, "customerName", customerName, sizeof(customerName))) {
            sendJson(client, 400, "Bad Request", "Enter a valid seat number and guest name.");
            return;
        }
        Seat *seat = findSeat(head, seatNo);
        if (seat == NULL) {
            sendJson(client, 404, "Not Found", "That seat does not exist.");
        } else if (seat->booked) {
            sendJson(client, 409, "Conflict", "That seat has already been reserved.");
        } else {
            reserveSeat(head, seatNo, customerName);
            char message[80];
            snprintf(message, sizeof(message), "Seat %d is reserved. Enjoy the show!", seatNo);
            sendJson(client, 201, "Created", message);
        }
    } else if (strcmp(method, "DELETE") == 0) {
        int seatNo = 0;
        char trailing = '\0';
        if (sscanf(path, "/api/reservations/%d%c", &seatNo, &trailing) != 1
            || seatNo < 1 || seatNo > TOTAL_SEATS) {
            sendJson(client, 404, "Not Found", "That reservation does not exist.");
        } else {
            Seat *seat = findSeat(head, seatNo);
            if (seat == NULL || !seat->booked) {
                sendJson(client, 409, "Conflict", "That seat is not currently reserved.");
            } else {
                cancelSeat(head, seatNo);
                char message[80];
                snprintf(message, sizeof(message), "Reservation for seat %d has been cancelled.", seatNo);
                sendJson(client, 200, "OK", message);
            }
        }
    } else if (strcmp(method, "GET") == 0
        && (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0)) {
        if (!sendFile(client, "index.html", "text/html; charset=utf-8")) {
            sendJson(client, 500, "Internal Server Error", "Unable to open index.html.");
        }
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/styles.css") == 0) {
        if (!sendFile(client, "styles.css", "text/css; charset=utf-8")) {
            sendJson(client, 500, "Internal Server Error", "Unable to open styles.css.");
        }
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/script.js") == 0) {
        if (!sendFile(client, "script.js", "text/javascript; charset=utf-8")) {
            sendJson(client, 500, "Internal Server Error", "Unable to open script.js.");
        }
    } else {
        sendJson(client, 404, "Not Found", "The requested page or API route was not found.");
    }
}

static int startWebServer(Seat *head) {
#ifdef _WIN32
    WSADATA winsockData;
    if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0) {
        fprintf(stderr, "Unable to initialize Windows sockets.\n");
        return 1;
    }
#endif

    socket_t server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "Unable to create the web server socket.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(SERVER_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR
        || listen(server, 8) == SOCKET_ERROR) {
        fprintf(stderr, "Unable to listen on http://127.0.0.1:%d. The port may already be in use.\n", SERVER_PORT);
        CLOSE_SOCKET(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("Theatre booking is ready at http://127.0.0.1:%d\n", SERVER_PORT);
    printf("Keep this window open while using the browser. Press Ctrl+C to stop.\n");
    fflush(stdout);

    for (;;) {
        socket_t client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        handleRequest(client, head);
        CLOSE_SOCKET(client);
    }
}

int main(int argc, char *argv[]) {
    Seat *head = NULL;
    int choice, seatNo;
    char name[50];

    for (int i = 1; i <= TOTAL_SEATS; i++) {
        insertSeat(&head, i);
    }

    if (argc > 1 && strcmp(argv[1], "--server") == 0) {
        int result = startWebServer(head);
        freeSeats(head);
        return result;
    }

    while (1) {
        printf("\nTheatre Seat Allocation System\n");
        printf("1. Display all seats\n");
        printf("2. Allocate a seat\n");
        printf("3. Cancel a reservation\n");
        printf("4. Display available seats\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            printf("Invalid choice. Please enter a number from 1 to 5.\n");
            continue;
        }
        getchar();

        switch (choice) {
            case 1:
                displaySeats(head);
                break;

            case 2:
                printf("Enter seat number to allocate (1-%d): ", TOTAL_SEATS);
                if (scanf("%d", &seatNo) != 1) {
                    while (getchar() != '\n') {}
                    printf("Invalid seat number.\n");
                    break;
                }
                getchar();
                printf("Enter customer name: ");
                if (fgets(name, sizeof(name), stdin) == NULL) {
                    printf("Unable to read customer name.\n");
                    break;
                }
                name[strcspn(name, "\n")] = '\0';
                if (seatNo < 1 || seatNo > TOTAL_SEATS) {
                    printf("Invalid seat number. Please choose between 1 and %d.\n", TOTAL_SEATS);
                } else {
                    reserveSeat(head, seatNo, name);
                }
                break;

            case 3:
                printf("Enter seat number to cancel (1-%d): ", TOTAL_SEATS);
                if (scanf("%d", &seatNo) != 1) {
                    while (getchar() != '\n') {}
                    printf("Invalid seat number.\n");
                    break;
                }
                getchar();
                if (seatNo < 1 || seatNo > TOTAL_SEATS) {
                    printf("Invalid seat number. Please choose between 1 and %d.\n", TOTAL_SEATS);
                } else {
                    cancelSeat(head, seatNo);
                }
                break;

            case 4:
                displayAvailableSeats(head);
                break;

            case 5:
                freeSeats(head);
                printf("Program terminated.\n");
                return 0;

            default:
                printf("Invalid choice. Please select a valid option.\n");
                break;
        }
    }
}
