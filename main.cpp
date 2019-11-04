#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static const uint32_t matrixSize = 16;
static const uint32_t matrixNumber = 10000;

int main(){
    /* genarate random matrix*/
    // srand(time(NULL));
    // FILE *fp = fopen("matrix.bin","wb");

    // for(int i = 0 ; i < matrixNumber ; i++) {
    //     uint32_t t = rand();
    //     // fprintf(fp,"%x",t);
    //     fwrite(&t,sizeof(uint32_t),1,fp);
    //     printf("%x\n",t);
    // }

    FILE *fp = fopen("matrix.bin","rb");
    if(fp==NULL) {
        printf("fp null pointer\n");
        return -1;
    }

    uint32_t* originMatrix = NULL;
    uint32_t* deltaMatrix = NULL;
    unsigned char* runLengthMatrix = NULL;

    originMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber);
    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber);
    runLengthMatrix = (unsigned char*)malloc(sizeof(unsigned char) * matrixNumber * 8);

    fread(originMatrix, sizeof(uint32_t), (size_t)matrixNumber, fp);
    // for(int i = 0 ; i < matrixNumber ; i++){
    //     printf("%x\n",originMatrix[i]);
    // }

    /* delta encoding */
    uint32_t last = 0;
    for(int i = 0 ; i < matrixNumber ; i++){
        uint32_t current = originMatrix[i];
        deltaMatrix[i] = current - last;
        last = current;
    } 

    //printf("end delta encoding\n");

    /* saving delta matrix*/
    FILE *sp = fopen("buffer.bin", "wb");
    if(sp==NULL) {
        printf("sp null pointer");
        return -1;
    }
    fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);

    /* show delta matrix */
    //for(int i = 0 ; i < matrixNumber ; i++) printf("%x\n",deltaMatrix[i]);

    /* run length encoding */
    FILE *rp = fopen("buffer.bin", "rb");
    if(rp == NULL){
        printf("rp null pointer");
        return -1;
    }

    fread(runLengthMatrix, sizeof(unsigned char), (size_t)matrixNumber * 8, rp);

    //unsigned char last = 0;
    for(int i = 0 ; i < matrixNumber * 4 ; i++){
        printf("%02x",runLengthMatrix[i]);
    }

}