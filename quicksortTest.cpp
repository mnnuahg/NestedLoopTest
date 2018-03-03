#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "testLoop.h"

int *Array;
int ArraySize;

/* Return the size of the array whose elements are smaller than pivot */
int Partition(int base, int size) {
    int pivot = Array[base];

    int left = base+1;
    int right = base+size-1;

    while(1) {
        while(left<right && Array[left]<pivot)
            left++;

        while(right>left && Array[right]>=pivot)
            right--;

        if(left < right) {
            int temp = Array[right];
            Array[right] = Array[left];
            Array[left] = temp;

            left++;
            right--;
        }
        else {
            if(Array[left] >= pivot)
                left--;
            if(Array[right] < pivot)
                right++;

            int temp = Array[left];
            Array[left] = pivot;
            Array[base] = temp;

            break;
        }
    }

    return left-base;
}

void QuickSort(int base, int size) {
    printf("QuickSort(");
    for(int i=0; i<base; i++) {
        printf("   ");
    }
    printf("[");
    for(int i=0; i<size; i++) {
        printf("%2d,", Array[base+i]);
    }
    printf("]");
    for(int i=base+size; i<ArraySize; i++) {
        printf("   ");
    }
    printf(");\n");
    
    if(size > 1) {
        int ltSize = Partition(base, size);

        LOOP_BEGIN(i, 0, 2) {
            QuickSort(i==0 ? base : base+ltSize+1, 
                      i==0 ? ltSize : size-ltSize-1);
        } LOOP_END;
    }
}

void genTest(int *array, int size, int randomSeed) {
    for(int i=0; i<size; i++) {
        array[i] = i;
    }

    srand(randomSeed);
    for(int i=0; i<size; i++) {
        int des = rand()%size;
        int temp = array[i];
        array[i] = array[des];
        array[des] = temp;
    }
}

int main(int argc, char* argv[]) {
    if(argc < 3) {
        printf("%s <Array size> <Random seed>\n", argv[0]);
        return 0;
    }

    ArraySize = atoi(argv[1]);
    if(ArraySize > 100) {
        printf("Arrays larger than 100 elements will corrupt the printing format\n");
        return 0;
    }
   
    Array = (int*)malloc(sizeof(int)*ArraySize);

    int seed = atoi(argv[2]);
    genTest(Array, ArraySize, seed);

    // This random seed determines how the execution order of QuickSort is randomized 
    srand(time(0));
    
    QuickSort(0, ArraySize);

    printf("Result:   [");
    for(int i=0; i<ArraySize; i++) {
        printf("%2d,", Array[i]);
    }
    printf("]\n");

    free(Array);

    return 0;
}
