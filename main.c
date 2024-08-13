#include <stdio.h>
#include <stdlib.h>
#include "main.h"

void make_thread(pthread_t thread)
{
	//dbcheck_thread
	if(pthread_create(&thread, NULL, check_db, NULL)!=0){
		printf("cannot create dbcheck thread\n");
	}else{
		printf("dbcheck thread created\n");
	}
}


int main(int argc, char* argv[])
{
	pthread_t check_db_status_t;
	print_db_ver();
	set_config_name(argv[1]);
	read_db_cfg();
	read_log_cfg();
	//connecting_test(gpcb->db01.conn, gpcb->db02.conn);
	//selectall(gpcb->db01.conn);
	make_thread(check_db_status_t);
	make_connection();

	if(pthread_join(check_db_status_t, NULL)!=0){
		printf("join_error\n");
	}
	
	return 0;
}
