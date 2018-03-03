#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "testLoop.h"

int main() {
    srand(time(0));
    
    printf("Task A\n");
    LOOP_BEGIN(i, 0, 2) {
        printf(" Task B%d\n", i);
        
        LOOP_BEGIN(j, 0, 2) {
            printf("  Task C%d%d\n", i, j);

            LOOP_BEGIN(k, 0, 2) {
                printf("   Task D%d%d%d\n", i, j, k);
            } LOOP_END;

            printf("  Task E%d%d\n", i, j);
        } LOOP_END;
        
        printf(" Task F%d\n", i);
    } LOOP_END;
    printf("Task G\n");

    return 0;
}
