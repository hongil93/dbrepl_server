#include "main.h"
#define JDR_SECTION "JDR"


void set_config_name(char* config_file_name){
	CFG *gcfg = gpcb->cfg;
	snprintf(gcfg->config_file_name, MAX_FILE_NAME, "%s", config_file_name);
	if(gcfg->config_file_name){
		printf("config file name: %s\n", gcfg->config_file_name);
	}else{
		printf("config fname error\n");
		exit(0);
	}
}

int read_db_cfg()
{
	CFG *gcfg = gpcb->cfg;
	DB_INFO* db01 = &(gpcb->db01);
	DB_INFO* db02 = &(gpcb->db02);
	int cfd;

	cfd = ec_init_inifile(gcfg->config_file_name);
	if (cfd<0){
		printf("cfd error\n");
		exit(0);
	}else{
		printf("succss init_inifile\n");
	}

	ec_get_profile_string(cfd, "DB01", "HOST", "", db01->host);
	ec_get_profile_string(cfd, "DB01", "USERNAME", "", db01->username);
	ec_get_profile_string(cfd, "DB01", "PASSWORD", "", db01->password);
	ec_get_profile_string(cfd, "DB01", "DBNAME", "", db01->dbname);
	db01->port = ec_get_profile_int(cfd, "DB01", "PORT", 1);

	ec_get_profile_string(cfd, "DB02", "HOST", "", db02->host);
	ec_get_profile_string(cfd, "DB02", "USERNAME", "", db02->username);
	ec_get_profile_string(cfd, "DB02", "PASSWORD", "", db02->password);
	ec_get_profile_string(cfd, "DB02", "DBNAME", "", db02->dbname);
	db02->port = ec_get_profile_int(cfd, "DB02", "PORT", 1);

	printf("------DB01-------\n");
	printf("HOST = %s \n", db01->host);
	printf("USERNAME = %s \n", db01->username);
	printf("PASSWORD = %s \n", db01->password);
	printf("DBNAME = %s \n", db01->dbname);
	printf("PORT = %d \n", db01->port);
	printf("\n");

	printf("------DB02-------\n");
	printf("HOST = %s \n", db02->host);
	printf("USERNAME = %s \n", db02->username);
	printf("PASSWORD = %s \n", db02->password);
	printf("DBNAME = %s \n", db02->dbname);
	printf("PORT = %d \n", db02->port);
	printf("\n");

	ec_end_inifile(cfd);

	return 1;
}

int read_log_cfg()
{
	CFG *gcfg = gpcb->cfg;
	int cfd;
	char debug_list[64];

	cfd = ec_init_inifile(gcfg->config_file_name);
	if (cfd<0){
		printf("cfd error\n");
		exit(0);
	}else{
		printf("succss init_inifile\n");
	}
	
	ec_get_profile_string(cfd, "LOG", "LOG_FLAG", "", debug_list);
	ec_strlist2loglevel((const char*)debug_list, &gcfg->debug_level);
	ec_get_profile_string(cfd, "LOG", "LOG_FILE_NAME", "", gcfg->log_file_name);
	gcfg->max_file_size = ec_get_profile_int(cfd, "LOG", "LOG_MAX_SIZE", 1);
	gcfg->max_log_count = ec_get_profile_int(cfd, "LOG", "LOG_MAX_CNT", 1);
	gcfg->max_queue_count = ec_get_profile_int(cfd, "LOG", "LOG_MAX_QUEUE_JOB_COUNT", 1);

	if(ec_init_log(gcfg->log_file_name,
				FN_TYPE_DATE|FN_TYPE_SIZE,
				gcfg->max_file_size,
				gcfg->max_log_count,
				gcfg->debug_level,
				gcfg->max_queue_count)){
		printf("log init success\n");
	}else{
		printf("log init fail\n");
	}

	printf("log_file_name: %s\n", gcfg->log_file_name);

	InitJDRSupport(gcfg->config_file_name , JDR_SECTION);
	
	ec_end_inifile(cfd);

	DwDebugLog(DEB_DEBUG, ">>>[INIT] success log INIT\n");

	return 1;
}