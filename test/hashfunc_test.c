#include <stdio.h>
#include <stdlib.h>

#define test_size 5000
#define table_size 10

int pointer_hash( int key){
    int hash=0;
    unsigned int k = key * 0x9E3779B9;
    while(k){
        hash += k & 0xf;
        k >>= 8;
    }
    return hash % table_size;
}


int main()
{
    int freq[table_size] = {0};
    int nums[test_size];
    int i;
    for(i=0; i <test_size; i++){
        int h = pointer_hash((void*)(&nums[i]));
        freq[h]++;
    }
    for(i=0;i<table_size;i++){
        printf("freq[%d]=%d\n",i, freq[i]);
    }
    
    return 0;
}


/* output for test_size 5000
freq[0]=492                                                                                                                                                                          
freq[1]=503                                                                                                                                                                          
freq[2]=503                                                                                                                                                                          
freq[3]=510                                                                                                                                                                          
freq[4]=492                                                                                                                                                                          
freq[5]=506                                                                                                                                                                          
freq[6]=496                                                                                                                                                                          
freq[7]=484                                                                                                                                                                          
freq[8]=508                                                                                                                                                                          
freq[9]=506
*/