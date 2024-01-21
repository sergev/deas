#define MAXCALLB 10

struct callBack {
    int key;
    void (*func)(int);
};

struct KeyPrivate {
    int keyback;
    struct Keytab *keymap;
    struct callBack callBackTable[MAXCALLB];
    int ncallb;
};
