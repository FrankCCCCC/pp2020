#include <cstdio>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {

    float buffer[10];
    FILE *ptr;

    ptr = fopen("../out/21.out","rb");  // r for read, b for binary

    fread(buffer,sizeof(buffer),1,ptr); // read 10 bytes to our buffer

    for(int i = 0; i < sizeof(buffer) / sizeof(float); i++){
        printf("%f ", buffer[i]);
    }
    printf("\n");
}