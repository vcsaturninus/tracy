#include <stdio.h>

#include "tracy.h"

static void f1(void);
static void f2(void);
static void f3(void);
static void f4(void);
static void f5(void);

void f1(void){
    __tracy_push;

    f2();

    TRETURN;
}

void f2(void){
    __tracy_push;

    f3();

    TRETURN;
}

void f3(void){
    __tracy_push;

    f4();

    TRETURN;
}

void f4(void){
    __tracy_push;

    f5();

    TRETURN;
}

void f5(void){
    __tracy_push;

    __tracy_dump;

    TRETURN;
}


int main(int argc, char *argv[]){
    __tracy_init;
    __tracy_push;
    f1();

    __tracy_destroy;
}





