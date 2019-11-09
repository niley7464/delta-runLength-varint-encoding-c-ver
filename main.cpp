#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static const uint32_t matrixNumber = 10000;

// 4 kb = 4096 byte = 1028 * 4 byte

void saveBlock(FILE* fp, uint8_t* tags, uint8_t* tuples, int* total, int* number, uint8_t* tag, int* tag_idx);

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
    fclose(fp);

    // printf("saving delta decoding result.\n");

    /* show delta matrix */
    //for(int i = 0 ; i < matrixNumber ; i++) printf("%x\n",deltaMatrix[i]);

    free(deltaMatrix);

    /* run length encoding */
    fp = fopen("buffer.bin", "rb");
    sp = fopen("length.bin","wb");
    if(fp == NULL){
        printf("rp null pointer");
        return -1;
    }

    fread(runLengthMatrix, sizeof(uint32_t), (size_t)matrixNumber, fp);

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

    fclose(fp);
    fclose(sp);

    //printf("end run length encoding.\n");

    /* varint encoding */

    fp = fopen("length.bin","rb");
    sp = fopen("varint.bin","wb");
    //fseek(sp,160,SEEK_SET); // 160byte (5 * uint32_t) 남겨놓기
    //printf("file pointer position : %ld\n",ftell(sp));

    if(fp==NULL){
        printf("fp null pointer");
        return -1;
    }

    // count_tuple , tuple_idx 합치기

    uint32_t buffer;
    int count_tag = 0; // tag를 저장하는데 쓰이는 uint8_t의 개수
    int count_tuple = 822; // 총 tuple이 4096개 넘으면 block full // number of tuples, tags 로 이미 822 채워둠

    uint8_t* tagMatrix = (uint8_t*) malloc(sizeof(uint32_t) * 818); // maximum number of tuple which can be in one block
    uint8_t* tupleMatrix = (uint8_t*) malloc(sizeof(uint8_t) * 3272);
    int tuple_idx = 0;

    uint8_t tag=0; // real tag
    int tag_idx = 0; // tag Matrix에 넣을 때 index

    while(fread(&buffer,sizeof(uint32_t),1,fp)!=0){ // 4096 넘는지 중간 중간 체크!
        //printf("input : %x\n",buffer);
        
        uint8_t b0 = buffer;
        uint8_t b1 = buffer >> 8;
        uint8_t b2 = buffer >> 16;
        uint8_t b3 = buffer >> 24;

        if(buffer <= 0xFF){
            if(count_tuple + 1 >= 4096) saveBlock(sp,tagMatrix,tupleMatrix,&count_tag,&count_tuple,&tag,&tag_idx);
     
            count_tuple += 1;
            tag <<=2;

            tupleMatrix[tuple_idx++] = b0;
        }
        else if(buffer <= 0xFFFF){
            if(count_tuple + 2 >= 4096) saveBlock(sp,tagMatrix,tupleMatrix,&count_tag,&count_tuple,&tag,&tag_idx);
  
            count_tuple += 2;
            tag <<= 2;
            tag += 1;

            tupleMatrix[tuple_idx++] = b0;
            tupleMatrix[tuple_idx++] = b1;
        }
        else if(buffer <= 0xFFFFFF){
            if(count_tuple + 3 >= 4096) saveBlock(sp,tagMatrix,tupleMatrix,&count_tag,&count_tuple,&tag,&tag_idx);

            count_tuple += 3;
            tag <<= 2;
            tag += 2;

            tupleMatrix[tuple_idx++] = b0;
            tupleMatrix[tuple_idx++] = b1;
            tupleMatrix[tuple_idx++] = b2;
        }
        else{
            if(count_tuple + 4 >= 4096) saveBlock(sp,tagMatrix,tupleMatrix,&count_tag,&count_tuple,&tag,&tag_idx);

            count_tuple += 4;
            tag <<= 2;
            tag += 3;
           
            tupleMatrix[tuple_idx++] = b0;
            tupleMatrix[tuple_idx++] = b1;
            tupleMatrix[tuple_idx++] = b2;  
            tupleMatrix[tuple_idx++] = b3;
        }
        if((++count_tag)%4 == 0){
            tagMatrix[tag_idx++] = tag;
            tag = 0;
        }
    }

    saveBlock(sp,tagMatrix,tupleMatrix,&count_tag,&count_tuple,&tag,&tag_idx);

    fclose(fp);
    fclose(sp);

    /* run length decoding */
    fp = fopen("length.bin","rb");
    sp = fopen("decoded_len.bin","wb");
    uint32_t repeat;
    for(int i = 0 ; i < matrixNumber ; i++){
        if(fread(&repeat,sizeof(uint32_t),1,fp)==0) return -1;
        if(fread(&cnt,sizeof(uint32_t),1,fp)==0) return -1;
        while(cnt--){
            //printf("%x\n",repeat);
            fwrite(&repeat,sizeof(uint32_t),1,sp);
        }
    }

    //printf("run length decoding done.\n");

    fclose(fp);
    fclose(sp);

    /* delta decoding*/

    fp = fopen("decoded_len.bin","rb");
    sp = fopen("decoded_delta.bin","wb");

    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber);
    fread(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,fp);
    
    
    last = 0;
    for(int i = 0 ; i < matrixNumber ; i++){
        uint32_t delta = deltaMatrix[i];
        deltaMatrix[i] = delta+last;
        last = deltaMatrix[i];
    }

    //fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);

    printf("All done.\n");
}

void saveBlock(FILE* fp, uint8_t* tags, uint8_t* tuples, int* total, int* number, uint8_t* tag, int* tag_idx){
    fwrite(&total,sizeof(int),1,fp);

    // for(int i = 0 ; i < *total ; i++){
    //     printf("%x\t",tags[i]);
    // }

    // for(int i = 0 ; i < *number ; i++){
    //     printf("%x\n", tuples[i]);
    // }

    fwrite(tags,sizeof(uint8_t),*total,fp);
    fwrite(tuples,sizeof(uint8_t),*number,fp);

    // 4kb 빈공간 아무값 or 0 채워넣기

    *total = 822;
    *number = 0;
    *tag = 0;
    *tag_idx = 0;

    printf("Block write done\n");
}