#include "main.h"


char* time_now(){
	time_t now = time(NULL);
	char* datetime;
	strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M", localtime(&now));
	return datetime;
}


