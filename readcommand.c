#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "readcommand.h"
void split(char *str, int *count, char *words[]) 
{
	int i = 0;
	const char *sep = " ";
	char *p;
	p = strtok(str, sep);
	while(p){
		if(i == MAX_PARA){
			printf("Error:too many parameters!\n"); 
			return;
		}
		words[i] = p;
		p = strtok(NULL, sep);
		i++;
	}
	*count = i;
	return;
}

void findchannels(char *str, int *seg, int channel_pos[])
{
    int i = 0;
    *seg = 0;
    for(i = 0 ; i < MAX_STR; i++)
    {
        if(str[i] == '\0') break; //tail
        if(str[i] == '<' || str[i] == '>' || str[i] == '|'){
            channel_pos[*seg] = i;
            (*seg)++;
        }
    }
    return;
}



int test(void)
{
	char s[] = "ls -l | wc -l";
	char *words[MAX_PARA];
	int *count = (int*)malloc(sizeof(int));
	int i;
	split(s, count, words);
	
	/*
    printf("%d\n", *count);
	
	for(i = 0; i < *count; i++){
		printf("%s\n", words[i]);
	}

	printf("\n");
	*/
	char s2[] = "efg, uioef flejoi";
	split(s2, count, words);
	
    /*
	printf("%d\n", *count);
	
	for(i = 0; i < *count; i++){
		printf("%s\n", words[i]);
	}

	printf("%s\n", words[3]);
    */

	return 0;
}
