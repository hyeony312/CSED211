//20220778 표승현
#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 캐시 구조
typedef struct{
    int valid;
    int tag;
    int order;
}line;

line** cache;

// miss, hit, evict 수
int miss_count=0;
int hit_count=0;
int evict_count=0;

int recent_order = 0;  // 전역변수

// 커맨드 변수
int s=0;
int s_=0;
int E=0;
int b=0;
int b_=0;
int check_v=0;

//trace 정보
char* trace;

void caching(unsigned long long address);

int main(int argc, char* argv[])
{
    char opt;
    while ((opt = getopt(argc, argv, "s:E:b:t: ")) != -1)
   {
    switch (opt)
    {
    case 'h':
        printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
        return 0;
    case 'v':
        check_v=1;
        break;
    case 's':
        s = atoi(optarg);
        break;
    case 'E':
        E = atoi(optarg);
        break;
    case 'b':
        b = atoi(optarg);
        break;
    case 't':
        trace = optarg;
        break;
    default:
        return 0;
        }
    }
    
    //initialize cache
    s_ = 1<<s;
    cache = (line**)malloc(sizeof(line*) * s_);
    for (int i = 0; i < s_; i++){
        cache[i] = (line*)malloc(sizeof(line) * E);
    }

    for(int i=0;i<s_;i++){
        for(int j=0;j<E;j++){
            cache[i][j].valid=0;
            cache[i][j].tag=0;
            cache[i][j].order=s_*E;
        }
    }

    //read trace
    FILE* trace_p= fopen(trace, "r");
    char cmd;
    unsigned long long address;
    int size;

    if(trace_p!=NULL){
    while (fscanf(trace_p, " %c %llx %d", &cmd, &address, &size) != EOF)
      {
        switch(cmd){
            case 'I':
                continue;
            case 'L':
                caching(address);
                break;
            case 'S':
                caching(address);
                break;
            case 'M':
                caching(address);
                caching(address);
                break;
            default:
                break;
        }
      }
    }
    else{
        return 0;
    }
    fclose(trace_p);

    for(int i = 0; i < s_; i++)
    {
        free(cache[i]);
    }
    free(cache);

    printSummary(hit_count, miss_count, evict_count);
    return 0;
}

void caching(unsigned long long address)
{  
    int t_bit = address >> (s + b);
    int s_index = (address >> b) & (s_ - 1);

    recent_order ++; // Update the most recently used line

    for (int i = 0; i < E; i++)
    {
        if (cache[s_index][i].valid)
        {
            if (cache[s_index][i].tag == t_bit) // Hit
            {
                hit_count++;
                cache[s_index][i].order = recent_order;
                if (check_v == 1)
                {
                    printf(" hit");
                }
                return;  // Return early on a hit
            }
        }
    }

    // Miss
    miss_count++;

    for (int i = 0; i < E; i++)
    {
        if (!cache[s_index][i].valid)
        {
            // Set is not full, insert the data
            cache[s_index][i].valid = 1;
            cache[s_index][i].tag = t_bit;
            cache[s_index][i].order=recent_order;
            if (check_v == 1)
            {
                printf(" miss");
            }
            return;
        }
    }

    // Eviction
    evict_count++;
    int min_order=recent_order;
    int position=0;
    for (int i = 0; i < E; i++)
    {
        if (cache[s_index][i].order<min_order)
        {
            min_order=cache[s_index][i].order;
            position=i;
        }
    }
    cache[s_index][position].tag = t_bit;  // Replace the least recently used line
    cache[s_index][position].order=recent_order;

    if (check_v == 1)
    {
        printf(" miss eviction");
    }
}