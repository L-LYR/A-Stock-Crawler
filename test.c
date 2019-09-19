#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_NUMBER_NUM 100
const int stockCodeLen = 8;
const int curPriceLoc = 3;
void test(char *str)
{
    char target[MAX_NUMBER_NUM * 64], slice[512], *p, temp[64], part[16];

    int i, j;
    for (i = 0; i < 1; ++i)
    {
        memset(temp, 0, sizeof(temp));
        sscanf(str, "%[^\n]", slice);
        
        p = strstr(slice, "=");
        p -= stockCodeLen;
        sscanf(p, "%[^=]", part);
        strcat(temp, part);
        strcat(temp, " ");

        p = strstr(slice, "\"");
        p++;
        sscanf(p, "%[^,]", part);
        strcat(temp, part);
        strcat(temp, " ");

        for (j = 0; j < curPriceLoc; ++j)
        {
            p = strstr(p, ",");
            p++;
        }
        sscanf(p, "%[^,]", part);
        strcat(temp, part);
        strcat(temp, " ");

        p = strstr(p, "-");
        p -= 4;
        sscanf(p, "%[^,]", part);
        strcat(temp, part);
        strcat(temp, " ");

        p = strstr(p, ",");
        p++;
        sscanf(p, "%[^\"]", part);
        strcat(temp, part);
        strcat(temp, "\n");

        strcat(target, temp);

        str = strstr(str, "\n");
        str++;
    }
    printf("%s", target);
}
int main(void)
{
    FILE *data = fopen("./data/file0", "r");
    char str[40960];
    fgets(str, 40960 - 1, data);
    test(str);
    return 0;
}