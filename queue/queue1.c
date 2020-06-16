#include <stdio.h>
#include <stdlib.h>
#include "q1.h"
 
int main(int argc, const char * argv[]) {
    Queue* q = CreateQueue(6);
    
    AddQ(q, 0);
    AddQ(q, 1);
    AddQ(q, 2);
    AddQ(q, 3);
    AddQ(q, 4);
    AddQ(q, 5);
    PrintQueue(q);
    /*
    for (int i = 0; i < 3; ++i)
    {
    	printf("%d ", DeleteQ(q));
    }*/
    printf("\n");

    if (DeleteQ(q))
    {
        printf("valid\n");
    }else{
        printf("not valid\n");
    }
    
    DeleteQ(q);
    DeleteQ(q);
    DeleteQ(q);
    
    PrintQueue(q);
    
    AddQ(q, 6);
    AddQ(q, 7);
    AddQ(q, 8);
    PrintQueue(q);
 
    return 0;
}