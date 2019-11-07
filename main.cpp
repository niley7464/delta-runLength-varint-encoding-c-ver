#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static const uint32_t matrixSize = 16;
static const uint32_t matrixNumber = 10000;

// 4 kb = 4096 byte = 128 * 32 = 512 * 8

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
    runLengthMatrix = (uint32_t*)malloc(2 * sizeof(uint32_t) * matrixNumber); // to save the result of run length encoding

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

    // printf("end delta encoding.\n");

    fclose(fp);

    /* saving delta matrix*/
    FILE *sp = fopen("buffer.bin", "wb");
    if(sp==NULL) {
        printf("sp null pointer");
        return -1;
    }
    fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);
    fclose(sp);

    // printf("saving delta decoding result.\n");

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
    uint32_t cnt = 1;

    for(int i = 1 ; i < matrixNumber; i++){
        if(prev==runLengthMatrix[i]) cnt++;
        else{
            fwrite(&prev,sizeof(uint32_t),1,sp);
            //printf("%x ",prev);
            fwrite(&cnt,sizeof(uint32_t),1,sp);
            //printf("%d\n",cnt);
            prev = runLengthMatrix[i];
            cnt = 1;
        }
    }

    fwrite(&prev,sizeof(uint32_t),1,sp);
    fwrite(&cnt,sizeof(uint32_t),1,sp);

    free(runLengthMatrix);

    fclose(rp);
    fclose(sp);

    //printf("end run length encoding.\n");

    /* varint encoding */

    fp = fopen("length.bin","rb");
    sp = fopen("varint.bin","wb");
    fseek(sp,160,SEEK_SET); // 160byte (5 * uint32_t) 남겨놓기
    printf("file pointer position : %ld\n",ftell(sp));

    if(fp==NULL){
        printf("fp null pointer");
        return -1;
    }

    uint32_t buffer;
    int count_tag = 0; // tag에 쓰이는 uint32_t의 개수가 늘어나는지 확인
    int count_tuple = 5; // 총 tuple이 512개 넘으면 block full // tuple개수랑 tag들 32비트짜리 4개 해서 총 5개 미리

    uint32_t* tagMatrix = (uint32_t*) malloc(sizeof(uint32_t) * 4); // maximum number of tuple which can be in one block
    uint8_t* tupleMatrix = (uint8_t*) malloc(sizeof(uint8_t)*4*100);
    int tuple_idx = 0;

    uint32_t tag=0;
    int tag_idx = 0;

    while(fread(&buffer,sizeof(uint32_t),1,fp)!=0){ // 512 넘는지 중간 중간 체크!
        printf("input : %x\n",buffer);
        if(buffer <= 0xFF){
            count_tuple += 1;
            tag <<=2;
            uint8_t b = buffer;
            // printf("%x\n",b);
            // fwrite(&b,sizeof(uint8_t),1,sp);
        }
        else if(buffer <= 0xFFFF){
            count_tuple += 2;
            tag <<= 2;
            tag += 1;
            uint8_t b0 = buffer;
            uint8_t b1 = buffer >> 8;
            // printf("%x %x\n",b0,b1);
            // fwrite(&b0,sizeof(uint8_t),1,sp);
            // fwrite(&b1,sizeof(uint8_t),1,sp);
        }
        else if(buffer <= 0xFFFFFF){
            count_tuple += 3;
            tag <<= 2;
            tag += 2;
            uint8_t b0 = buffer;
            uint8_t b1 = buffer >> 8;
            uint8_t b2 = buffer >> 16;
            // printf("%x %x %x\n",b0,b1,b2);
            // fwrite(&b0,sizeof(uint8_t),1,sp);
            // fwrite(&b1,sizeof(uint8_t),1,sp);
            // fwrite(&b2,sizeof(uint8_t),1,sp);
        }
        else{
            count_tuple += 4;
            tag <<= 2;
            tag += 3;
            uint8_t b0 = buffer;
            uint8_t b1 = buffer >> 8;
            uint8_t b2 = buffer >> 16;
            uint8_t b3 = buffer >> 24;
            // printf("%x %x %x %x\n",b0,b1,b2,b3);
            // fwrite(&b0,sizeof(uint8_t),1,sp);
            // fwrite(&b1,sizeof(uint8_t),1,sp);
            // fwrite(&b2,sizeof(uint8_t),1,sp);
            // fwrite(&b3,sizeof(uint8_t),1,sp);
        }
        if(++count_tag == 128){
            tagMatrix[tag_idx++] = tag;
            tag = 0;
        }
    }

    

    /* run length decoding */
    rp = fopen("length.bin","rb");
    sp = fopen("decoded_len.bin","wb");
    uint32_t repeat;
    for(int i = 0 ; i < matrixNumber ; i++){
        if(fread(&repeat,sizeof(uint32_t),1,rp)==0) return -1;
        if(fread(&cnt,sizeof(uint32_t),1,rp)==0) return -1;
        while(cnt--){
            //printf("%x\n",repeat);
            fwrite(&repeat,sizeof(uint32_t),1,sp);
        }
    }

    //printf("run length decoding done.\n");

    fclose(rp);
    fclose(sp);

    /* delta decoding*/

    rp = fopen("decoded_len.bin","rb");
    sp = fopen("decoded_delta.bin","wb");

    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber);
    fread(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,rp);
    
    
    last = 0;
    for(int i = 0 ; i < matrixNumber ; i++){
        uint32_t delta = deltaMatrix[i];
        deltaMatrix[i] = delta+last;
        last = deltaMatrix[i];
    }

    //fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);

    printf("All done.\n");
}