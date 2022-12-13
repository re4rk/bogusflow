#include <stdio.h>
#include <time.h>

int a = 0;
int test (){ 
    for(int i = 0; i < 100000000; i++){
        a += a + i;
    }
    return 0;
}

int main (int argc, char *argv[]) {
    clock_t start_clk = clock();
    test();
    printf("%s - Processor time used by program: %lg sec.\n", argv[0], (clock() - start_clk) / (double) CLOCKS_PER_SEC);
    return 0;
}