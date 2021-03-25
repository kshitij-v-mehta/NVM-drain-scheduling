#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
    int i, a;
    char a_str[8];
    char hostname[32];
    char hostid_str[64];
    strcpy(hostname, "h30n17");
    hostname[6] = '\0';

    for (i=0;i<6;i++){
        //printf("%c ", hostname[i]);
        a = hostname[i];
        printf("%d ", a);
        sprintf(a_str, "%d", a);
        strcat(hostid_str, a_str);
    }
    printf("\n");
    printf("%s\n", hostid_str);
    return 0;
}

