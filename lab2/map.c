#include "stdint.h"
#include "stdio.h"

char HDI_to_ASCII (uint8_t hdi){
    char hash_map [53] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        13, 27, 127, 9, ' ', '-', '=', '[', ']', 92, 35, ';', 39, '`', ',', //next line first ? is a caps
        '.', '/'};
    //printf("%c \n ", hash_map[hdi]); 
    return hash_map[hdi];
}

