
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <winsock2.h>
#include "helpdesk.h"

#define PORT 8080
#define DATA_FILE "backend/queries.txt"

struct Query *head = NULL;
int nextId = 101;

/* Send all response data */

int sendAll(SOCKET client, const char *data, int length) {
    int sent = 0;
    int n;

    while (sent < length) {
        n = send(client, data + sent, length - sent, 0);

        if (n == SOCKET_ERROR || n == 0)
            return 0;

        sent += n;
    }

    return 1;
}

/* Send HTTP response */

void sendResponse(SOCKET client, const char *type,
                  const char *body) {
    char header[512];

    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n\r\n",
        type, (int)strlen(body));

    sendAll(client, header, (int)strlen(header));
    sendAll(client, body, (int)strlen(body));
}

/* Send website files */

void serveFile(SOCKET client, const char *filename,
               const char *type) {
    FILE *fp;
    char buffer[4096];
    char header[512];
    int bytes;
    long size;

    fp = fopen(filename, "rb");

    if (fp == NULL) {
        sendResponse(client, "text/plain", "File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        type, size);

    sendAll(client, header, (int)strlen(header));

    while ((bytes = (int)fread(buffer, 1,
                               sizeof(buffer), fp)) > 0) {
        if (!sendAll(client, buffer, bytes))
            break;
    }

    fclose(fp);
}

/* Decode form data */

void decode(char *text) {
    char *src = text;
    char *dst = text;

    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        else if (*src == '%' && src[1] && src[2]) {
            char hex[3];

            hex[0] = src[1];
            hex[1] = src[2];
            hex[2] = '\0';

            *dst++ = (char)strtol(hex, NULL, 16);
            src += 3;
        }
        else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

/* Get a value from submitted form */

void getValue(char *body, char *key,
              char *output, int size) {
    char copy[4096];
    char *token;
    char *equal;

    output[0] = '\0';

    strncpy(copy, body, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    token = strtok(copy, "&");

    while (token != NULL) {
        equal = strchr(token, '=');

        if (equal != NULL) {
            *equal = '\0';

            if (strcmp(token, key) == 0) {
                strncpy(output, equal + 1, size - 1);
                output[size - 1] = '\0';
                decode(output);
                return;
            }
        }

        token = strtok(NULL, "&");
    }
}

/* Remove characters that disturb file storage */

void cleanText(char *text) {
    int i;

    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] == '|' ||
            text[i] == '\n' ||
            text[i] == '\r') {
            text[i] = ' ';
        }
    }
}

/* Save queries into file */

void saveQueries(void) {
    FILE *fp;
    struct Query *temp = head;

    fp = fopen(DATA_FILE, "w");

    if (fp == NULL) {
        printf("Unable to save queries!\n");
        return;
    }

    while (temp != NULL) {
        fprintf(fp, "%d|%s|%s|%s|%s|%d\n",
            temp->id,
            temp->name,
            temp->rollNo,
            temp->category,
            temp->description,
            temp->status);

        temp = temp->next;
    }

    fclose(fp);
}

/* Load previously saved queries */

void loadQueries(void) {
    FILE *fp;
    char line[512];
    struct Query *newNode;
    struct Query *temp;

    fp = fopen(DATA_FILE, "r");

    if (fp == NULL) {
        fp = fopen(DATA_FILE, "w");

        if (fp != NULL)
            fclose(fp);

        return;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {

        newNode = (struct Query *)malloc(sizeof(struct Query));

        if (newNode == NULL)
            break;

        if (sscanf(line,
            "%d|%49[^|]|%19[^|]|%29[^|]|%199[^|]|%d",
            &newNode->id,
            newNode->name,
            newNode->rollNo,
            newNode->category,
            newNode->description,
            &newNode->status) != 6) {

            free(newNode);
            continue;
        }

        newNode->next = NULL;

        if (newNode->id >= nextId)
            nextId = newNode->id + 1;

        if (head == NULL) {
            head = newNode;
        }
        else {
            temp = head;

            while (temp->next != NULL)
                temp = temp->next;

            temp->next = newNode;
        }
    }

    fclose(fp);
}

/* Add a new query using Linked List */

int addQuery(char *name, char *roll,
             char *category, char *description) {

    struct Query *newNode;
    struct Query *temp;

    newNode = (struct Query *)malloc(sizeof(struct Query));

    if (newNode == NULL)
        return 0;

    newNode->id = nextId++;

    strncpy(newNode->name, name, 49);
    newNode->name[49] = '\0';

    strncpy(newNode->rollNo, roll, 19);
    newNode->rollNo[19] = '\0';

    strncpy(newNode->category, category, 29);
    newNode->category[29] = '\0';

    strncpy(newNode->description, description, 199);
    newNode->description[199] = '\0';

    cleanText(newNode->name);
    cleanText(newNode->rollNo);
    cleanText(newNode->category);
    cleanText(newNode->description);

    newNode->status = 0;
    newNode->next = NULL;

    if (head == NULL) {
        head = newNode;
    }
    else {
        temp = head;

        while (temp->next != NULL)
            temp = temp->next;

        temp->next = newNode;
    }

    saveQueries();

    return newNode->id;
}

/* Search query using Linear Search */

struct Query *searchQuery(int id) {
    struct Query *temp = head;

    while (temp != NULL) {
        if (temp->id == id)
            return temp;

        temp = temp->next;
    }

    return NULL;
}

/* Resolve a query */

int resolveQuery(int id) {
    struct Query *temp = searchQuery(id);

    if (temp == NULL)
        return 0;

    temp->status = 1;

    saveQueries();

    return 1;
}

/* Add safe text to JSON output */

void appendText(char *output, int *pos,
                int size, const char *text) {
    int len = (int)strlen(text);

    if (*pos + len >= size)
        return;

    memcpy(output + *pos, text, len);
    *pos += len;
    output[*pos] = '\0';
}

/* Convert text into JSON-safe string */

void appendJsonString(char *output, int *pos,
                      int size, const char *text) {
    char c;
    int i;

    appendText(output, pos, size, "\"");

    for (i = 0; text[i] != '\0'; i++) {
        c = text[i];

        if (c == '"' || c == '\\') {
            char escaped[3];

            escaped[0] = '\\';
            escaped[1] = c;
            escaped[2] = '\0';

            appendText(output, pos, size, escaped);
        }
        else if (c == '\n' || c == '\r') {
            appendText(output, pos, size, " ");
        }
        else if ((unsigned char)c >= 32) {
            char one[2];

            one[0] = c;
            one[1] = '\0';

            appendText(output, pos, size, one);
        }
    }

    appendText(output, pos, size, "\"");
}

/* Display all queries as JSON */

void displayQueries(char *output, int size) {
    struct Query *temp = head;
    char number[50];
    int pos = 0;
    int first = 1;

    output[0] = '\0';

    appendText(output, &pos, size, "[");

    while (temp != NULL && pos < size - 1000) {

        if (!first)
            appendText(output, &pos, size, ",");

        first = 0;

        appendText(output, &pos, size, "{\"id\":");

        sprintf(number, "%d", temp->id);
        appendText(output, &pos, size, number);

        appendText(output, &pos, size, ",\"name\":");
        appendJsonString(output, &pos, size, temp->name);

        appendText(output, &pos, size, ",\"rollNo\":");
        appendJsonString(output, &pos, size, temp->rollNo);

        appendText(output, &pos, size, ",\"category\":");
        appendJsonString(output, &pos, size, temp->category);

        appendText(output, &pos, size, ",\"description\":");
        appendJsonString(output, &pos, size, temp->description);

        sprintf(number, ",\"status\":%d}", temp->status);
        appendText(output, &pos, size, number);

        temp = temp->next;
    }

    appendText(output, &pos, size, "]");
}

/* Handle browser requests */

void handleRequest(SOCKET client, char *request) {
    char method[10];
    char path[256];
    char body[4096] = "";
    char output[64000];

    char name[50];
    char roll[20];
    char category[30];
    char description[200];

    char *bodyStart;
    int id;
    int newId;

    sscanf(request, "%9s %255s", method, path);

    bodyStart = strstr(request, "\r\n\r\n");

    if (bodyStart != NULL) {
        bodyStart += 4;

        strncpy(body, bodyStart, sizeof(body) - 1);
        body[sizeof(body) - 1] = '\0';
    }

    /* GET requests */

    if (strcmp(method, "GET") == 0) {

        if (strcmp(path, "/") == 0) {
            serveFile(client, "frontend/index.html", "text/html");
        }
        else if (strcmp(path, "/style.css") == 0) {
            serveFile(client, "frontend/style.css", "text/css");
        }
        else if (strcmp(path, "/script.js") == 0) {
            serveFile(client, "frontend/script.js",
                      "application/javascript");
        }
        else if (strcmp(path, "/api/queries") == 0) {
            displayQueries(output, sizeof(output));
            sendResponse(client, "application/json", output);
        }
        else {
            sendResponse(client, "text/plain", "Page not found");
        }
    }

    /* POST requests */

    else if (strcmp(method, "POST") == 0) {

        if (strcmp(path, "/api/queries") == 0) {

            getValue(body, "name", name, sizeof(name));
            getValue(body, "roll", roll, sizeof(roll));
            getValue(body, "category", category, sizeof(category));
            getValue(body, "description",
                     description, sizeof(description));

            if (name[0] == '\0' ||
                roll[0] == '\0' ||
                category[0] == '\0' ||
                description[0] == '\0') {

                sendResponse(client, "application/json",
                             "{\"success\":false}");
                return;
            }

            newId = addQuery(name, roll, category, description);

            if (newId == 0) {
                sendResponse(client, "application/json",
                             "{\"success\":false}");
                return;
            }

            sprintf(output,
                "{\"success\":true,\"id\":%d}", newId);

            sendResponse(client, "application/json", output);
        }

        else if (sscanf(path, "/api/resolve?id=%d", &id) == 1) {

            if (resolveQuery(id)) {
                sendResponse(client, "application/json",
                             "{\"success\":true}");
            }
            else {
                sendResponse(client, "application/json",
                             "{\"success\":false}");
            }
        }

        else {
            sendResponse(client, "text/plain", "Invalid request");
        }
    }

    else {
        sendResponse(client, "text/plain", "Invalid method");
    }
}

/* Start the local web server */

void startServer(void) {
    WSADATA wsa;
    SOCKET server;
    SOCKET client;
    struct sockaddr_in address;

    char request[8192];
    char headers[4096];

    char *headerEnd;
    char *lengthHeader;
    char *line;

    int bytes;
    int total;
    int headerLength;
    int contentLength;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock initialization failed!\n");
        return;
    }

    loadQueries();

    server = socket(AF_INET, SOCK_STREAM, 0);

    if (server == INVALID_SOCKET) {
        printf("Socket creation failed!\n");
        WSACleanup();
        return;
    }

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&address,
             sizeof(address)) == SOCKET_ERROR) {

        printf("Port 8080 is unavailable!\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    if (listen(server, 5) == SOCKET_ERROR) {
        printf("Unable to start server!\n");
        closesocket(server);
        WSACleanup();
        return;
    }

    printf("\nServer started successfully!\n");
    printf("Open browser: http://localhost:8080\n");
    printf("Press Ctrl+C to stop the server.\n\n");

    while (1) {

        client = accept(server, NULL, NULL);

        if (client == INVALID_SOCKET)
            continue;

        total = 0;
        headerLength = 0;
        contentLength = 0;

        memset(request, 0, sizeof(request));

        while (total < (int)sizeof(request) - 1) {

            bytes = recv(client, request + total,
                         sizeof(request) - total - 1, 0);

            if (bytes <= 0)
                break;

            total += bytes;
            request[total] = '\0';

            headerEnd = strstr(request, "\r\n\r\n");

            if (headerEnd != NULL) {

                headerLength = (int)(headerEnd - request) + 4;

                lengthHeader = strstr(request, "Content-Length:");

                if (lengthHeader != NULL)
                    contentLength = atoi(lengthHeader + 15);

                if (total >= headerLength + contentLength)
                    break;
            }
        }

        if (headerLength > 0 &&
            total >= headerLength + contentLength) {

            handleRequest(client, request);
        }

        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
}
