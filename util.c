#include "main.h"


char* time_now() {
    time_t now = time(NULL);
    char* datetime = malloc(16);
    if (datetime == NULL) {
        return NULL;
    }
    strftime(datetime, 16, "%Y%m%d_%H%M", localtime(&now));
    return datetime;
}

int calculate_tps(){
	

}