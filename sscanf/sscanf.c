#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main () {
   int day, year;
   char weekday[20], month[20], dtm[100];

   strcpy( dtm, "Saturday March 25 1989" );
   int n_var = sscanf( dtm, "%s %s %d  %d", weekday, month, &day, &year );
   printf("%d variable(s) find\n", n_var);

   printf("%s %d, %d = %s\n", month, day, year, weekday );

   int a=1,b=1;
   int i = a++, f = ++b;
   printf("%d %d\n",i,f );
    
   return(0);
}