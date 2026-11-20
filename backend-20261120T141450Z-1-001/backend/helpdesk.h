
#ifndef HELPDESK_H
#define HELPDESK_H

struct Query {
    int id;
    char name[50];
    char rollNo[20];
    char category[30];
    char description[200];
    int status;

    struct Query *next;
};

void startServer(void);

#endif
