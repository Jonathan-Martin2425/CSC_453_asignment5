#include <stdlib.h>
#include <stdio.h>

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n"

int main(int argc, char *argv[]){
    int option;
    extern int optind;
    extern char *optarget;

    while((option = getopt(argc, argv, OPTSTR)) != EOF){
        switch (option)
        {
        case 'd':
            printf("option -%c wit arg %s\n", option, optarget);
            break;
        case '?':
            printf(USAGE);
            break;
        default:
            printf(USAGE);
            break;
        }
    }
    return 0;
}