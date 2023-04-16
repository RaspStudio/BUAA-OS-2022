#include "../lib/lib.h"

#define HISTORY_PATH "/.history"
#define HISTORY_MAGIC (0x12344321)
#define MAXHISTORY (32 - 2)
#define HISTORY_FD 2

typedef struct {
    u_int magic;
    int index;
    int max;
    char content[MAXHISTORY][MAXLINE];
} HistoryFile;

#define OFF_MAGIC           ((u_int)(&(((HistoryFile*)0)->magic)))
#define OFF_INDEX           ((u_int)(&(((HistoryFile*)0)->index)))
#define OFF_MAX             ((u_int)(&(((HistoryFile*)0)->max)))
#define OFF_CONTENT         ((u_int)(&(((HistoryFile*)0)->content)))
#define OFF_CONTENTELM(x)  ((u_int)(&(((HistoryFile*)0)->content[x])))



int history_open() {
    int fd;
    if ((fd = open(HISTORY_PATH, O_RDWR)) < 0) {
        writef(B_RED"History"C_RESET": Cannot Open History File!\n");
        return -E_INVAL;
    }

    if (seek(fd, OFF_MAGIC) < 0) {
        writef(B_RED"History"C_RESET": Cannot Find History File!\n");
        return -E_INVAL;
    }

    struct Fd *p;
    if (fd_lookup(fd, &p) < 0) {
        writef(B_RED"History"C_RESET": Cannot Read History File!\n");
        return -E_INVAL;
    }

    HistoryFile *ret = (HistoryFile*)fd2data(p);

    if (ret->magic != HISTORY_MAGIC) {
        writef(B_RED"History"C_RESET": Invalid History File!\n");
        return -E_INVAL;
    }

    return fd;
}

HistoryFile *__history_open(int fd) {
    struct Fd *p;
    if (fd_lookup(fd, &p) < 0) {
        user_panic(B_RED"History"C_RESET": Cannot Read History File!\n");
    }

    return (HistoryFile*)fd2data(p);
}

void history_init() {
    int fd;
    
    if ((fd = open(HISTORY_PATH, O_CREAT | O_RDWR)) < 0) {
        writef(B_RED"History"C_RESET": Cannot Create New File!\n");
        return;
    }
    writef("History is at %d.", fd);
    HistoryFile template = {0x12344321, 0, 0};
    if (seek(fd,0) < 0) {
        writef(B_RED"History"C_RESET": Cannot Find New File!\n");
        return;
    }
    if (write(fd, &template, sizeof(HistoryFile)) < 0) {
        writef(B_RED"History"C_RESET": Cannot Write New File!\n");
        return;
    }
    if (close(fd) < 0) {
        writef(B_RED"History"C_RESET": Cannot Save New File!\n");
        return;
    }
}

void history_add(int fd, const char *op) {
    HistoryFile *p = __history_open(fd);
    int dst = p->max;
    strcpy(p->content[dst%MAXHISTORY], op);
    //writef("(%s)[%d] <- %s", p->content[dst%MAXHISTORY], dst%MAXHISTORY, op);
    p->index = ++p->max;
    p->content[(dst+1)%MAXHISTORY][0] = 0;
}

int history_up(int fd, char *dst) {
    HistoryFile *p = __history_open(fd);
    int ret;
    if ((p->index) > (p->max - (MAXHISTORY - 1)) && p->index > 0) {
        ret = --p->index;
    } else {
        //writef("NO LAST OP(%d/%d)[%d&%d]", p->index, p->max, p->index, (p->max - (MAXHISTORY - 1)));
        return -E_INVAL;
    }

    strcpy(dst, p->content[ret]);
    return 0;
}

int history_down(int fd, char *dst) {
    HistoryFile *p = __history_open(fd);
    int ret;
    if (p->index < p->max) {
        ret = ++p->index;
    } else {
        //writef("NO NEXT OP(%d/%d)[%d&%d]", p->index, p->max, p->index, (p->max - (MAXHISTORY - 1)));
        return -E_INVAL;
    }

    strcpy(dst, p->content[ret]);
    return 0;
}

void history_print() {
    int fd = history_open();
    HistoryFile *p = __history_open(fd);
    //writef("[DEBUG] magic=%x | index=%d | max=%d\n", p->magic, p->index, p->max);
    int i;
    for (i = ((p->max - (MAXHISTORY - 1) < 0) ? 0 : (p->max - (MAXHISTORY-1))); i < p->max; i++) {
        fwritef(1, "%4d\t%s\n", i, p->content[i%MAXHISTORY]);
    }
    close(fd);
}
