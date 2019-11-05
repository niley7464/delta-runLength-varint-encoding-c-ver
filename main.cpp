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
    uint32_t* runLengthMatrix = NULL;

    originMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber); // to save original matrix
    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber); // to save the result of delta encoding
    runLengthMatrix = (uint32_t*)malloc((sizeof(uint32_t) + sizeof(int)) * matrixNumber); // to save the result of run length encoding

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

    //printf("end delta encoding.\n");

    /* saving delta matrix*/
    FILE *sp = fopen("buffer.bin", "wb");
    if(sp==NULL) {
        printf("sp null pointer");
        return -1;
    }
    fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);
    fclose(sp);

    //printf("saving delta decoding result.\n");

    /* show delta matrix */
    //for(int i = 0 ; i < matrixNumber ; i++) printf("%x\n",deltaMatrix[i]);

    free(deltaMatrix);

    /* run length encoding */
    FILE *rp = fopen("buffer.bin", "rb");
    sp = fopen("length.bin","wb");
    if(rp == NULL){
        printf("rp null pointer");
        return -1;
    }

    fread(runLengthMatrix, sizeof(uint32_t), (size_t)matrixNumber, rp);

    uint32_t prev = runLengthMatrix[0];
    int cnt = 1;

    for(int i = 1 ; i < matrixNumber; i++){
        if(prev==runLengthMatrix[i]) cnt++;
        else{
            fwrite(&prev,sizeof(uint32_t),1,sp);
            //printf("%x ",prev);
            fwrite(&cnt,sizeof(int),1,sp);
            //printf("%d\n",cnt);
            prev = runLengthMatrix[i];
            cnt = 1;
        }
    }
    //마지막 처리!!!

    free(runLengthMatrix);

    fclose(rp);
    fclose(sp);

    //printf("end run length encoding.\n");

    /* run length decoding */
    rp = fopen("length.bin","rb");
    sp = fopen("decoded_len.bin","wb");
    uint32_t repeat;
    for(int i = 0 ; i < matrixNumber-1 ; i++){ // 나중에 마지막 처리하고 1 다시 더하기!!!!
        if(fread(&repeat,sizeof(uint32_t),1,rp)==0) return -1;
        if(fread(&cnt,sizeof(int),1,rp)==0) return -1;
        while(cnt--){
            //printf("%x\n",repeat);
            fwrite(&repeat,sizeof(uint32_t),1,sp);
        }
    }

    //printf("run length decoding done.\n");

    fclose(rp);
    fclose(sp);

    rp = fopen("decoded_len.bin","rb");
    sp = fopen("decoded_delta.bin","wb");

    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber);
    fread(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,rp);
    last = 0;

    for(int i = 0 ; i < matrixNumber ; i++){
        uint32_t delta = deltaMatrix[i];
        deltaMatrix[i] = delta+last;
        last = deltaMatrix[i];
        printf("%x\n",deltaMatrix[i]);
    }

    printf("All done.\n");
}