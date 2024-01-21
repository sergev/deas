#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "deas.h"
#include "pgplib.h"

#define MAXCLIENTS 64

static clientinfo cltab[MAXCLIENTS];

int debug;
static fd_set openfds;
static long counter;
static void *OK = (void *)&OK; /* abstract non-NULL pointer */

static void fatal(char *msg)
{
    fprintf(stderr, "deas error: %s\n", msg);
    exit(1);
}

long *option_date_1(long *date, clientinfo *clnt)
{
    static long result;

    result = option_date(clnt, *date);
    return &result;
}

long *option_deleted_1(long *on, clientinfo *clnt)
{
    static long result;

    result = option_deleted(clnt, *on);
    return &result;
}

account_reply *chart_first_1(void *voidarg, clientinfo *clnt)
{
    static account_reply result;
    accountinfo *r;

    r = chart_first(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

account_reply *chart_next_1(void *voidarg, clientinfo *clnt)
{
    static account_reply result;
    accountinfo *r;

    r = chart_next(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

account_reply *chart_info_1(long *acn, clientinfo *clnt)
{
    static account_reply result;
    accountinfo *r;

    r = chart_info(clnt, *acn);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

void *chart_add_1(accountinfo *i, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - add chart #%ld (%s)", clnt->u->info.logname, i->acn, i->descr);
    chart_add(clnt, i);
    return OK;
}

void *chart_edit_1(accountinfo *i, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - edit chart #%ld (%s)", clnt->u->info.logname, i->acn, i->descr);
    chart_edit(clnt, i);
    return OK;
}

void *chart_delete_1(long *acn, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - delete chart #%ld", clnt->u->info.logname, *acn);
    chart_delete(clnt, *acn);
    return OK;
}

entry_reply *account_first_1(long *acn, clientinfo *clnt)
{
    static entry_reply result;
    entryinfo *r;

    r = account_first(clnt, *acn);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

entry_reply *account_next_1(void *voidarg, clientinfo *clnt)
{
    static entry_reply result;
    entryinfo *r;

    r = account_next(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

balance_reply *account_balance_1(long *acn, clientinfo *clnt)
{
    static balance_reply result;
    balance *r;

    r = account_balance(clnt, *acn);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

report_reply *account_report_1(long *acn, clientinfo *clnt)
{
    static report_reply result;
    balance *r;

    r = account_report(clnt, *acn);
    memset(&result, 0, sizeof(result));
    if (r)
        memcpy(&result.info, r, sizeof(result.info));
    else
        result.error = 1;
    return &result;
}

entry_reply *journal_first_1(void *voidarg, clientinfo *clnt)
{
    static entry_reply result;
    entryinfo *r;

    r = journal_first(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

entry_reply *journal_next_1(void *voidarg, clientinfo *clnt)
{
    static entry_reply result;
    entryinfo *r;

    r = journal_next(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

entry_reply *journal_info_1(long *eid, clientinfo *clnt)
{
    static entry_reply result;
    entryinfo *r;

    r = journal_info(clnt, *eid);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

void *journal_add_1(entryinfo *e, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - add entry #%ld/#%ld (%s)", clnt->u->info.logname, e->debit, e->credit,
           e->descr);
    journal_add(clnt, e);
    return OK;
}

void *journal_edit_1(entryinfo *e, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - edit entry #%ld/#%ld (%s)", clnt->u->info.logname, e->debit, e->credit,
           e->descr);
    journal_edit(clnt, e);
    return OK;
}

void *journal_delete_1(long *eid, clientinfo *clnt)
{
    syslog(LOG_NOTICE, "%s - delete entry %ld", clnt->u->info.logname, *eid);
    journal_delete(clnt, *eid);
    return OK;
}

menu_reply *menu_first_1(void *voidarg, clientinfo *clnt)
{
    static menu_reply result;
    menuinfo *r;

    r = menu_first(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

menu_reply *menu_next_1(void *voidarg, clientinfo *clnt)
{
    static menu_reply result;
    menuinfo *r;

    r = menu_next(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

user_reply *user_first_1(void *voidarg, clientinfo *clnt)
{
    static user_reply result;
    userinfo *r;

    r = user_first(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

user_reply *user_next_1(void *voidarg, clientinfo *clnt)
{
    static user_reply result;
    userinfo *r;

    r = user_next(clnt);
    memset(&result, 0, sizeof(result));
    if (r)
        result.info = *r;
    else
        result.error = 1;
    return &result;
}

int authenticate(int sock, char *data, int len)
{
    authinfo auth;
    clientinfo *clnt = cltab + sock;
    struct sockaddr_in client;
    char key[16];
    long result;

    if (debug)
        fprintf(stderr, "Authentication started at %d\n", sock);
    memset(clnt, 0, sizeof(*clnt));
    len = private_decrypt(data, &auth, len, deas_E, deas_D, deas_P, deas_Q, deas_U, deas_N);
    if (len != sizeof(authinfo)) {
    failed:
        result = 0;
        send_packet(sock, (char *)&result, sizeof(long));
        if (debug)
            fprintf(stderr, "Registration failed\n");
        len = sizeof(client);
        getsockname(sock, (struct sockaddr *)&client, &len);
        syslog(LOG_NOTICE, "%s - authentication error", inet_ntoa(client.sin_addr));
        memset(clnt, 0, sizeof(*clnt));
        return 0;
    }

    clnt->u = user_register(auth.logname, 0, 0);
    if (!clnt->u)
        goto failed;

    len = public_decrypt(auth.data, key, sizeof(auth.data), user_E(clnt), user_N(clnt));
    if (len != sizeof(key))
        goto failed;

    if (debug)
        fprintf(stderr, "Registration %s at %d\n", auth.logname, sock);

    memcpy(clnt->key, key, sizeof(clnt->key));
    result = 1;
    send_packet(sock, (char *)&result, sizeof(long));
    if (debug)
        fprintf(stderr, "Registration succeeded\n");
    return 1;
}

static void process(int sock)
{
    char data[8192], *result, *name, *(*local)();
    clientinfo *clnt = cltab + sock;
    struct deas_req req;
    struct deas_reply reply;
    struct sockaddr_in client;
    int argsz, resultsz;
    int len;

    len = sizeof(client);
    getsockname(sock, (struct sockaddr *)&client, &len);
    len = receive_packet(sock, data, sizeof(data));
    if (len < 4) {
    closeit:
        close(sock);
        FD_CLR(sock, &openfds);
        if (debug)
            fprintf(stderr, "Close connection %d\n", sock);
        if (len < 0 && clnt->u)
            syslog(LOG_NOTICE, "%s/%d - logout", clnt->u->info.logname, sock);
        else
            syslog(LOG_NOTICE, "%s - protocol error", inet_ntoa(client.sin_addr));
        memset(clnt, 0, sizeof(*clnt));
        return;
    }
    if (!clnt->u) {
        if (!authenticate(sock, data, len))
            goto closeit;
        syslog(LOG_NOTICE, "%s/%d - login from %s", clnt->u->info.logname, sock,
               inet_ntoa(client.sin_addr));
        return;
    }
    block_decrypt(data, &req, len, clnt->key);
    switch (req.op) {
    case OPTION_DATE:
        name = "OPTION_DATE";
        argsz = sizeof(long);
        resultsz = sizeof(long);
        local = (char *(*)())option_date_1;
        break;
    case OPTION_DELETED:
        name = "OPTION_DELETED";
        argsz = sizeof(long);
        resultsz = sizeof(long);
        local = (char *(*)())option_deleted_1;
        break;
    case CHART_FIRST:
        name = "CHART_FIRST";
        argsz = 0;
        resultsz = sizeof(account_reply);
        local = (char *(*)())chart_first_1;
        break;
    case CHART_NEXT:
        name = "CHART_NEXT";
        argsz = 0;
        resultsz = sizeof(account_reply);
        local = (char *(*)())chart_next_1;
        break;
    case CHART_INFO:
        name = "CHART_INFO";
        argsz = sizeof(long);
        resultsz = sizeof(account_reply);
        local = (char *(*)())chart_info_1;
        break;
    case CHART_ADD:
        name = "CHART_ADD";
        argsz = sizeof(accountinfo);
        resultsz = 0;
        local = (char *(*)())chart_add_1;
        break;
    case CHART_EDIT:
        name = "CHART_EDIT";
        argsz = sizeof(accountinfo);
        resultsz = 0;
        local = (char *(*)())chart_edit_1;
        break;
    case CHART_DELETE:
        name = "CHART_DELETE";
        argsz = sizeof(long);
        resultsz = 0;
        local = (char *(*)())chart_delete_1;
        break;
    case ACCOUNT_FIRST:
        name = "ACCOUNT_FIRST";
        argsz = sizeof(long);
        resultsz = sizeof(entry_reply);
        local = (char *(*)())account_first_1;
        break;
    case ACCOUNT_NEXT:
        name = "ACCOUNT_NEXT";
        argsz = 0;
        resultsz = sizeof(entry_reply);
        local = (char *(*)())account_next_1;
        break;
    case ACCOUNT_BALANCE:
        name = "ACCOUNT_BALANCE";
        argsz = sizeof(long);
        resultsz = sizeof(balance_reply);
        local = (char *(*)())account_balance_1;
        break;
    case ACCOUNT_REPORT:
        name = "ACCOUNT_REPORT";
        argsz = sizeof(long);
        resultsz = sizeof(report_reply);
        local = (char *(*)())account_report_1;
        break;
    case JOURNAL_FIRST:
        name = "JOURNAL_FIRST";
        argsz = 0;
        resultsz = sizeof(entry_reply);
        local = (char *(*)())journal_first_1;
        break;
    case JOURNAL_NEXT:
        name = "JOURNAL_NEXT";
        argsz = 0;
        resultsz = sizeof(entry_reply);
        local = (char *(*)())journal_next_1;
        break;
    case JOURNAL_INFO:
        name = "JOURNAL_INFO";
        argsz = sizeof(long);
        resultsz = sizeof(entry_reply);
        local = (char *(*)())journal_info_1;
        break;
    case JOURNAL_ADD:
        name = "JOURNAL_ADD";
        argsz = sizeof(entryinfo);
        resultsz = 0;
        local = (char *(*)())journal_add_1;
        break;
    case JOURNAL_EDIT:
        name = "JOURNAL_EDIT";
        argsz = sizeof(entryinfo);
        resultsz = 0;
        local = (char *(*)())journal_edit_1;
        break;
    case JOURNAL_DELETE:
        name = "JOURNAL_DELETE";
        argsz = sizeof(long);
        resultsz = 0;
        local = (char *(*)())journal_delete_1;
        break;
    case USER_FIRST:
        name = "USER_FIRST";
        argsz = 0;
        resultsz = sizeof(user_reply);
        local = (char *(*)())user_first_1;
        break;
    case USER_NEXT:
        name = "USER_NEXT";
        argsz = 0;
        resultsz = sizeof(user_reply);
        local = (char *(*)())user_next_1;
        break;
    case MENU_FIRST:
        name = "MENU_FIRST";
        argsz = 0;
        resultsz = sizeof(menu_reply);
        local = (char *(*)())menu_first_1;
        break;
    case MENU_NEXT:
        name = "MENU_NEXT";
        argsz = 0;
        resultsz = sizeof(menu_reply);
        local = (char *(*)())menu_next_1;
        break;
    default:
        if (debug)
            fprintf(stderr, "Bad op %d [%d/%d] at %d\n", req.op, req.len, len, sock);
        goto closeit;
    }
    if (req.len != argsz) {
        if (debug)
            fprintf(stderr, "Invalid packet size %d, should be %d\n", req.len, argsz);
        goto closeit;
    }
    if (debug > 1)
        fprintf(stderr, "%s [%d] at %d\n", name, req.len, sock);
    result = (*local)(req.data, clnt);
    if (!result) {
        if (debug)
            fprintf(stderr, "%s failed at %d\n", name, sock);
        return;
    }
    reply.counter = ++counter;
    reply.len = resultsz;
    if (resultsz)
        memcpy(reply.data, result, resultsz);
    len = CALIGN(sizeof(reply) - sizeof(reply.data) + resultsz);
    block_encrypt(&reply, data, len, clnt->key);
    send_packet(sock, data, len);
}

int main(int argc, char **argv)
{
    fd_set readfds;
    int sock, sel, c, i, port = DEASPORT;
    char *dir = 0;
    struct sockaddr_in server, client;

    while ((c = getopt(argc, argv, "Dp:d:")) != EOF)
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'D':
            ++debug;
            break;
        case 'd':
            dir = optarg;
            break;
        default:
        usage:
            fatal("usage: deasd [-D] [-p port] [-d dir]");
        }
    argc -= optind;
    argv += optind;

    if (argc != 0)
        goto usage;

    openlog("deas", LOG_CONS, LOG_DAEMON);
    if (!debug)
        daemon(0, 0);

    deas_init(dir, 0, fatal);
    counter = time((time_t)0);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        fatal("cannot create socket");

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        fatal("cannot bind");
    listen(sock, 5);

    FD_ZERO(&openfds);
    FD_SET(sock, &openfds);
    for (;;) {
        readfds = openfds;
        sel = select(MAXCLIENTS, &readfds, 0, 0, 0);
        if (sel < 0) {
            if (errno != EINTR)
                perror("select failed");
            continue;
        }
        if (!sel)
            continue;
        if (FD_ISSET(sock, &readfds) && (sel = accept(sock, (struct sockaddr *)&client, &i)) >= 0) {
            /* Новое соединение */
            if (debug)
                fprintf(stderr, "New connection at %d\n", sel);
            if (sel >= MAXCLIENTS)
                close(sel);
            else {
                int val = 2;
                setsockopt(sel, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof(val));
                setsockopt(sel, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
                FD_SET(sel, &openfds);
                memset(cltab + sel, 0, sizeof(*cltab));
            }
        }
        for (i = 0; i < MAXCLIENTS; ++i)
            if (i != sock && FD_ISSET(i, &readfds))
                process(i);
    }
}
