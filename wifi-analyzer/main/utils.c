/*
 * utils.c
 *
 *  Created on: 19 mars 2025
 *      Author: HP
 */


#include <stdio.h>
#include "utils.h"

void print_auth_mode(int authmode) {
    switch(authmode) {
        case 0: printf(" Reseau ouvert"); break;
        case 1: printf(" WEP"); break;
        case 2: printf(" WPA"); break;
        case 3: printf(" WPA2"); break;
        case 4: printf(" WPA/WPA2"); break;
        case 5: printf(" WPA3"); break;
        default: printf(" Inconnu");
    }
}
