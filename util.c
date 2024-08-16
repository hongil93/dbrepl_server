#include "main.h"


char* time_now(){
	time_t now = time(NULL);
	char datetime[30];
	strftime(datetime, sizeof(datetime), "[%Y-%m-%d]%H:%M:%S", localtime(&now));
	return datetime;
}


