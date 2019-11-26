#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static const uint32_t matrixNumber = 10000;

// 4 kb = 4096 byte = 1028 * 4 byte

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

    uint32_t* originMatrix = NULL;
    uint32_t* deltaMatrix = NULL;
    uint32_t* runLengthMatrix = NULL;

    originMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber); // to save original matrix
    deltaMatrix = (uint32_t *)malloc(sizeof(uint32_t) * matrixNumber); // to save the result of delta encoding
    runLengthMatrix = (uint32_t*)malloc(2 * sizeof(uint32_t) * matrixNumber); // to save the result of run length encoding

    /* delta encoding */

    FILE *fp = fopen("matrix.bin","rb");
    FILE *sp = fopen("buffer.bin", "wb");

    fread(originMatrix, sizeof(uint32_t), (size_t)matrixNumber, fp);

    uint32_t last = 0;
    for(int i = 0 ; i < matrixNumber ; i++){
        uint32_t current = originMatrix[i];
        deltaMatrix[i] = current - last;
        last = current;
    } 

    fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);

    fclose(fp);
    fclose(sp);

    /* show delta matrix */
    //for(int i = 0 ; i < matrixNumber ; i++) printf("%x\n",deltaMatrix[i]);

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

    fclose(fp);
    fclose(sp);

    //printf("end run length encoding.\n");

    /* varint encoding */

    fp = fopen("length.bin","rb");
    sp = fopen("varint.bin","wb");

    if(fp==NULL){
        printf("fp null pointer");
        return -1;
    }

    uint32_t buffer; // input save buffer
    uint8_t* varintMatrix = (uint8_t*) malloc(sizeof(uint8_t) * 4096);

    int count_tag = 0; // check one tag byte is full
    int tag_idx = 0; // save tag index in varintMatrix
    int block_idx = 0; // index for varintMatrix
    int tag_counter = 0;
    int flag = 0;

    while(fread(&buffer,sizeof(uint32_t),1,fp)!=0){
        //printf("input : %x\n",buffer);

        uint8_t b[4] = {buffer, buffer >> 8, buffer >> 16, buffer >> 24};

        if(buffer <= 0xFF) flag = 1;
        else if(buffer <= 0xFFFF) flag = 2;
        else if(buffer <= 0xFFFFFF) flag = 3;
        else flag = 4;

        int newTagByte = ((count_tag++)%4==0);

        // save 4kb if block is full
        if(block_idx + flag + newTagByte >= 4096){
            // block change
            int t = tag_counter%4;

            switch(t){
                case 0:
                    break;
                case 1:
                    varintMatrix[tag_idx] <<= 6;
                    break;
                case 2:
                    varintMatrix[tag_idx] <<= 4;
                    break;
                case 3:
                    varintMatrix[tag_idx] <<= 2;
                    break;
            }
            //for(int i = 0 ; i < 4096 ; i++) printf("%x\n",varintMatrix[i]);
            fwrite(varintMatrix,sizeof(uint8_t),4096,sp);
            block_idx=0;
            tag_idx = 0;
            count_tag = 1;
            newTagByte = 1;
            // printf("Block Change.\n");
        }

        if(newTagByte){ // 새로운 태그 바이트
            tag_idx = block_idx++;
            varintMatrix[tag_idx] = (flag-1);
        }
        else{
            varintMatrix[tag_idx] <<= 2;
            varintMatrix[tag_idx] += (flag-1);
        }

        for(int i = 0 ; i < flag ; i++) varintMatrix[block_idx++] = b[i];
    }
    int t = tag_counter%4;

    switch(t){
        case 0:
            break;
        case 1:
            varintMatrix[tag_idx] <<= 6;
            break;
        case 2:
            varintMatrix[tag_idx] <<= 4;
            break;
        case 3:
            varintMatrix[tag_idx] <<= 2;
            break;
    }
    //for(int i = 0 ; i < 4096 ; i++) printf("%x\n",varintMatrix[i]);
    fwrite(varintMatrix,sizeof(uint8_t),4096,sp);

    fclose(fp);
    fclose(sp);

    //printf("end varint encoding\n");

    /* varint decoding */
    fp = fopen("varint.bin","rb");
    sp = fopen("decoded_varint.bin","wb");

    //uint8_t* varintMatrix = (uint8_t*) malloc(sizeof(uint8_t) * 4096);
    int idx,sum;
    uint8_t tag;
    
    // 4kb block 단위로 계속 읽어오기
    while(fread(varintMatrix,sizeof(uint8_t),4096,fp)!=0){
        for(int i = 0 ; i < 4096 ; i++){
            printf("%x\n",varintMatrix[i]);
        }
        idx = 0;
        while(true){
        tag = varintMatrix[idx++];
        int tagMatrix[4] = {(0xC0&tag)>>6, (0x30&tag)>>4,(0x0C&tag)>>2, 0x03&tag };
        sum = ((0x03&tag) + ((0x0C&tag)>>2) + ((0x30&tag)>>4) + ((0xC0&tag)>>6) )+ 4; // 앞으로 가져와야 할 8kb 블록의 개수
        //printf("%d\n",sum);
        if(idx + sum >= 4096){
            //특수 케이스
            if(idx + tagMatrix[0] + 1 < 4096){
                buffer = 0;
                for(int j = 0 ; j <= tagMatrix[0] ; j++){
                    buffer |= varintMatrix[idx++]<<(8*j);
                }
                //printf("%x\n",buffer);
                fwrite(&buffer,sizeof(uint32_t),1,sp);
            }
            if(idx + tagMatrix[1] + 1 < 4096){
                buffer = 0;
                for(int j = 0 ; j <= tagMatrix[1] ; j++){
                    buffer |= varintMatrix[idx++]<<(8*j);
                }
                //printf("%x\n",buffer);
                fwrite(&buffer,sizeof(uint32_t),1,sp);
            }
            if(idx + tagMatrix[2] + 1 < 4096){
                buffer = 0;
                for(int j = 0 ; j <= tagMatrix[2] ; j++){
                    buffer |= varintMatrix[idx++]<<(8*j);
                }
                //printf("%x\n",buffer);
                fwrite(&buffer,sizeof(uint32_t),1,sp);
            }
            break;
        }
        else{
            for(int i = 0 ; i < 4 ; i++){
                buffer = 0;
                for(int j = 0 ; j <= tagMatrix[i] ; j++){
                    buffer |= varintMatrix[idx++]<<(8*j);
                }
                //printf("%x\n",buffer);
                fwrite(&buffer,sizeof(uint32_t),1,sp);
            }
        }
        }
    }
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
        //printf("%x\n",deltaMatrix[i]);
    }

    fwrite(deltaMatrix,sizeof(uint32_t),(size_t)matrixNumber,sp);
}