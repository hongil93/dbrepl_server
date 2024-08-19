#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;


void* check_db(void* args)
{
	DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;

    while(1){
        conn1 = mysql_init(NULL);
        conn2 = mysql_init(NULL);

        if(connect_db(conn1, 1)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn1));
            gpcb->db01.status = 0;
            gpcb->repl_status = 0;
            printf("Can not connect to DB01\n");
            ec_log((DEB_WARN, ">>>[DB] DB01 IS DOWN\n", NULL));
            mysql_close(conn1);
        }else{
            gpcb->db01.status = 1;
            mysql_close(conn1);
        }

        if(connect_db(conn2, 2)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn2));
            gpcb->db02.status = 0;
            gpcb->repl_status = 0;
            printf("Can not connect to DB02\n");
            ec_log((DEB_WARN, ">>>[DB] DB02 IS DOWN\n", NULL));
            mysql_close(conn2);
        }else{
            gpcb->db02.status = 1;
            mysql_close(conn2);
        }

        if (gpcb->db02.status == 0 && gpcb->db01.status ==0){
            ec_log((DEB_ERROR, ">>>[DB] ALL DB DOWN\n", NULL));
            gpcb->repl_status = 0;
        }

        if (gpcb->db02.status == 1 && gpcb->db01.status ==1){
            ec_log((DEB_ERROR, ">>>[DB] ALL DB ON\n", NULL));
            get_repcheck_status();
            gpcb->repl_status = repl_status();
        }

        sleep(5);
    }

}

int repl_status(){
    REPL_SLAVE_STATUS *status_db01 = &gpcb->db01.repl_slave_status;
    REPL_SLAVE_STATUS *status_db02 = &gpcb->db02.repl_slave_status;
    REPL_MASTER_STATUS *m_status_db01 = &gpcb->db01.repl_master_status;
    REPL_MASTER_STATUS *m_status_db02 = &gpcb->db02.repl_master_status;

    if (strcmp(status_db01->Slave_IO_Running, "Yes") == 0 && strcmp(status_db01->Slave_SQL_Running, "Yes") == 0 
    && strcmp(status_db02->Slave_IO_Running, "Yes") == 0 && strcmp(status_db02->Slave_SQL_Running, "Yes")){
        return 1;
    } else {
        return 0;
    }
}


char* get_select_all()
{
	MYSQL* conn;
    MYSQL_RES* res;
    MYSQL_ROW row;
    int i;

	int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn);
	if (active_db == 0){
		printf("all_db_is_down\n");
	}

    conn = mysql_init(NULL);

    connect_main_db(active_db, conn);

    if (mysql_query(conn, "select * from user"))
    {
        printf("query fail\n");
    }

    res = mysql_store_result(conn);

    int num_fields = mysql_num_fields(res);
    int buffer_size = 1024; // √ ±‚ πˆ∆€ ≈©±‚
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        exit(1);
    }

    result_buffer[0] = '\0'; // ∫Û πÆ¿⁄ø≠∑Œ √ ±‚»≠

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {
            // « ø‰«œ¥Ÿ∏È ∞¢ « µÂø° ¥Î«ÿ πˆ∆€ ≈©±‚∏¶ µø¿˚¿∏∑Œ »Æ¿Â
            int field_length = row[i] ? strlen(row[i]) : 4; // "NULL"¿« ±Ê¿Ã¥¬ 4
            int required_length = strlen(result_buffer) + field_length + 2; // ∞¯πÈ∞˙ '\0' ∆˜«‘

            if (required_length > buffer_size)
            {
                buffer_size *= 2;
                result_buffer = (char*)realloc(result_buffer, buffer_size);
                if (result_buffer == NULL)
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    mysql_free_result(res);
                    mysql_close(conn);
                    exit(1);
                }
            }

            strcat(result_buffer, row[i] ? row[i] : "NULL");
            strcat(result_buffer, " ");
        }
        strcat(result_buffer, "\n");
    }
    ec_log((DEB_DEBUG, ">>>[DB] SQL Request :: select * from user\n", NULL));

    mysql_free_result(res);
    mysql_close(conn);
    return result_buffer;
}

int set_main_db(int db01_st, int db02_st, MYSQL* conn)
{
	if((db01_st)==1){
		conn = gpcb->db01.conn;
        printf("main DB is DB01\n");
        return 1;
	}
	if((db02_st)==1){
		conn = gpcb->db02.conn;
        printf("main DB is DB02\n");
        return 2;
	}
	return 0;
}

void connect_main_db(int activated_db, MYSQL* conn)
{
    DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
    if(activated_db == 1){
        if(connect_db(conn, 1)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }

    if(activated_db == 2){
         if(connect_db(conn, 2)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }
}

MYSQL* connect_db(MYSQL* conn, int db_idx){
    DB_INFO *db;
    MYSQL* ret;
    if (db_idx == 1) {
        db = &(gpcb->db01);
    }

    if (db_idx == 2){
        db = &(gpcb->db02);
    }

    ret = mysql_real_connect(conn, 
        db->host,
		db->username, 
		db->password,
		db->dbname,
		db->port,
		NULL,
		0);
    
    return ret;

}

void* check_file(void* args){
	struct stat statbuf;
	char* path = (char*)args;
	while(1){
		if ((stat(path, &statbuf))==0){
			printf("%s file exist\n", path);
			break;
		}
	}
}	

int get_db_data(int db_idx){
	FILE *fp;
	char path[BUF_SIZE];

    //get db02 data csv
	fp = popen("mysql -h 10.0.2.4 -e \"SELECT * FROM user;\" repl_test01 > /home/kim/backup/db02_data.csv", "r");
	if (fp == NULL){
		printf("popen fail\n");
		return 0;
	}

	while (fgets(path, sizeof(path), fp) != NULL) {
		printf("%s", path);
	}
	
	pclose(fp);
	return 0;
}

char* compare_table(int db_index)
{
	MYSQL* conn;
    MYSQL_RES* res;
    MYSQL_ROW row;
    int i;

    const char *sql_truncate = "TRUNCATE user_tmp;";
    const char *sql_load_data = "LOAD DATA LOCAL INFILE '/home/kim/backup/db02_data.csv' "
                                "INTO TABLE user_tmp "
                                "FIELDS TERMINATED BY '\t' "
                                "LINES TERMINATED BY '\n' "
                                "IGNORE 1 ROWS;";
    
    const char *sql_select1 = "SELECT u.* "
                                "FROM user u "
                                "LEFT JOIN user_tmp t ON u.id = t.id AND u.name = t.name "
                                "WHERE t.id IS NULL;";

    const char *sql_select2 = "SELECT t.* "
                                "FROM user_tmp t "
                                "LEFT JOIN user u ON t.id = u.id AND t.name = u.name "
                                "WHERE u.id IS NULL;";
	int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn);
	if (active_db == 0){
		printf("all_db_is_down\n");
        return NULL;
	}

    conn = mysql_init(NULL);
   
    connect_main_db(active_db, conn);

    printf("init, connect\n");

    if (mysql_query(conn, sql_truncate))
    {
        printf("query fail\n");
    }

    if (mysql_query(conn, sql_load_data))
    {
        printf("query fail\n");
    }

	if (db_index == 1){
		if (mysql_query(conn, sql_select1))
		{
			printf("query fail\n");
		}
	}else{
		if (mysql_query(conn, sql_select2))
		{
			printf("query fail\n");
		}
	}

    res = mysql_store_result(conn);

    int num_fields = mysql_num_fields(res);
    int buffer_size = 1024; 
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        exit(1);
    }

    result_buffer[0] = '\0';

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {
            
            int field_length = row[i] ? strlen(row[i]) : 4;
            int required_length = strlen(result_buffer) + field_length + 2;

            if (required_length > buffer_size)
            {
                buffer_size *= 2;
                result_buffer = (char*)realloc(result_buffer, buffer_size);
                if (result_buffer == NULL)
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    mysql_free_result(res);
                    mysql_close(conn);
                    exit(1);
                }
            }

            strcat(result_buffer, row[i] ? row[i] : "NULL");
            strcat(result_buffer, " ");
        }
        strcat(result_buffer, "\n");
    }

    ec_log((DEB_DEBUG, ">>>[DB] Request Check DB data\n", NULL));

    mysql_free_result(res);
    mysql_close(conn);

    return result_buffer;
}


void print_db_ver()
{
	printf("MySQL Client Version: %s\n", mysql_get_client_info());

}

/* hyogi code */
/* 2. replication check */
// char* get_repcheck_status()
// {
//     MYSQL_ROW row;
//     MYSQL_RES *res_db1, *res_db2;
//     MYSQL *conn_ptr_db1 = mysql_init(NULL);
//     MYSQL *conn_ptr_db2 = mysql_init(NULL);

//     // conn init
//     if (conn_ptr_db1 == NULL || conn_ptr_db2 == NULL) {
//         fprintf(stderr, "mysql_init() failed\n");
//         ec_log((DEB_ERROR, ">>>[DB] INIT ERROR\n", NULL));
//         return NULL;
//     }
    
//     /* conn connect */
//     // DB1
//     if (connect_db(conn_ptr_db1, 1) == NULL) {
//         fprintf(stderr, "Failed to connect to DB1: %s\n", mysql_error(conn_ptr_db1));
//         mysql_close(conn_ptr_db1);
//         ec_log((DEB_ERROR, ">>>[DB1] CONNECT ERROR\n", NULL));
//         return NULL;
//     }

//     // DB2
//     if (connect_db(conn_ptr_db2, 2) == NULL) {
//         fprintf(stderr, "Failed to connect to DB2: %s\n", mysql_error(conn_ptr_db2));
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         ec_log((DEB_ERROR, ">>>[DB2] CONNECT ERROR\n", NULL));
//         return NULL;
//     }

//     // buffer allocation
//     char* result_buffer = (char*)malloc(BUF_SIZE);
//     if (result_buffer == NULL)
//     {
//         fprintf(stderr, "Memory allocation failed\n");
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         ec_log((DEB_ERROR, ">>>[SYSTEM] Memory allocation ERROR\n", NULL));
//         return NULL;
//     }
//     result_buffer[0] = '\0';

//     /* SHOW SLAVE STATUS - DB01 */
//     if (mysql_query(conn_ptr_db1, "SHOW SLAVE STATUS"))
//     {
//         printf("DB1 query error: %s\n", mysql_error(conn_ptr_db1));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     res_db1 = mysql_store_result(conn_ptr_db1);
//     if (res_db1 == NULL)
//     {
//         fprintf(stderr, "res_db1 error: %s\n", mysql_error(conn_ptr_db1));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     int num_fields = mysql_num_fields(res_db1);
//     while ((row = mysql_fetch_row(res_db1)) != NULL) {
//         for (int i = 0; i < num_fields; i++) {
//             const char *field_name = mysql_fetch_field_direct(res_db1, i)->name;

//             /* string compare - slave status */
//             if(strcmp(field_name, "Slave_IO_Running") == 0 ||
//                strcmp(field_name, "Slave_SQL_Running") == 0 ||
//                strcmp(field_name, "Last_IO_Errno") == 0 ||
//                strcmp(field_name, "Last_SQL_Errno") == 0 ||
//                strcmp(field_name, "Master_Log_File") == 0 ||
//                strcmp(field_name, "Read_Master_Log_Pos") == 0) {
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] ? row[i] : "NULL");
//                strcat(result_buffer, ",");
//             }
//             /* string compare - slave error*/
//             if(strcmp(field_name, "Last_IO_Error") == 0 ||
//                strcmp(field_name, "Last_SQL_Error") == 0 ){
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] && strcmp(row[i], "") != 0 ? row[i] : "empty");
//                strcat(result_buffer, ",");
//             }
//         }
//     }
//     mysql_free_result(res_db1);

//     /* SHOW MASTER STATUS - DB01 */
//     if (mysql_query(conn_ptr_db1, "SHOW MASTER STATUS"))
//     {
//         printf("DB1 query error: %s\n", mysql_error(conn_ptr_db1));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     res_db1 = mysql_store_result(conn_ptr_db1);
//     if (res_db1 == NULL)
//     {
//         fprintf(stderr, "res_db1 error: %s\n", mysql_error(conn_ptr_db1));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db1);
//         return NULL;
//     }
//     int num_fields2 = mysql_num_fields(res_db1);
//     while ((row = mysql_fetch_row(res_db1)) != NULL) {
//         for (int i = 0; i < num_fields2; i++) {
//             const char *field_name = mysql_fetch_field_direct(res_db1, i)->name;

//             /* string attach in result_buffer */
//             strcat(result_buffer, row[i] ? row[i] : "NULL");
//             strcat(result_buffer, ",");
//             /* string compare */
//             if(strcmp(field_name, "Binlog_Do_DB") == 0 ||
//                strcmp(field_name, "Binlog_Ignore_DB") == 0 ){
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] && strcmp(row[i], "") != 0 ? row[i] : "empty");
//                strcat(result_buffer, ",");
//             }
//         }
//     }
//     mysql_free_result(res_db1);

//     /* SHOW SLAVE STATUS - DB02 */
//     if (mysql_query(conn_ptr_db2, "SHOW SLAVE STATUS"))
//     {
//         printf("DB2 query error: %s\n", mysql_error(conn_ptr_db2));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     res_db2 = mysql_store_result(conn_ptr_db2);
//     if (res_db2 == NULL)
//     {
//         fprintf(stderr, "res_db2 error: %s\n", mysql_error(conn_ptr_db2));
//         free(result_buffer);
//         mysql_close(conn_ptr_db2);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     int num_fields1 = mysql_num_fields(res_db2);
//     while ((row = mysql_fetch_row(res_db2)) != NULL) {
//         for (int i = 0; i < num_fields1; i++) {
//             const char *field_name = mysql_fetch_field_direct(res_db2, i)->name;

//             /* string compare - slave status */
//             if(strcmp(field_name, "Slave_IO_Running") == 0 ||
//                strcmp(field_name, "Slave_SQL_Running") == 0 ||
//                strcmp(field_name, "Last_IO_Errno") == 0 ||
//                strcmp(field_name, "Last_SQL_Errno") == 0 ||
//                strcmp(field_name, "Master_Log_File") == 0 ||
//                strcmp(field_name, "Read_Master_Log_Pos") == 0) {
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] ? row[i] : "NULL");
//                strcat(result_buffer, ",");
//             }
//             /* string compare - slave error*/
//             if(strcmp(field_name, "Last_IO_Error") == 0 ||
//                strcmp(field_name, "Last_SQL_Error") == 0) {
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] && strcmp(row[i], "") != 0 ? row[i] : "empty");
//                strcat(result_buffer, ",");
//             }
//         }
//     }
//     mysql_free_result(res_db2);

//     /* SHOW MASTER STATUS - DB02 */
//     if (mysql_query(conn_ptr_db2, "SHOW MASTER STATUS"))
//     {
//         printf("DB2 query error: %s\n", mysql_error(conn_ptr_db2));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     res_db2 = mysql_store_result(conn_ptr_db2);
//     if (res_db2 == NULL)
//     {
//         fprintf(stderr, "res_db2 error: %s\n", mysql_error(conn_ptr_db2));
//         free(result_buffer);
//         mysql_close(conn_ptr_db1);
//         mysql_close(conn_ptr_db2);
//         return NULL;
//     }

//     int num_fields3 = mysql_num_fields(res_db2);
//     while ((row = mysql_fetch_row(res_db2)) != NULL) {
//         for (int i = 0; i < num_fields3; i++) {
//             const char *field_name = mysql_fetch_field_direct(res_db2, i)->name;

//             /* string attach in result_buffer */
//             strcat(result_buffer, row[i] ? row[i] : "NULL");
//             strcat(result_buffer, ",");
//             /* string compare */
//             if(strcmp(field_name, "Binlog_Do_DB") == 0 ||
//                strcmp(field_name, "Binlog_Ignore_DB") == 0 ){
//                /* string attach in result_buffer */
//                strcat(result_buffer, row[i] && strcmp(row[i], "") != 0 ? row[i] : "empty");
//                strcat(result_buffer, ",");
//             }
//         }
//     }
//     mysql_free_result(res_db2);
//     mysql_close(conn_ptr_db1);
//     mysql_close(conn_ptr_db2);
    
//     return result_buffer;
// }

int get_repcheck_status()
{
    MYSQL_ROW row;
    MYSQL_RES *res_db1, *res_db2;
    MYSQL *conn_ptr_db1 = mysql_init(NULL);
    MYSQL *conn_ptr_db2 = mysql_init(NULL);

    REPL_SLAVE_STATUS *status_db01 = &gpcb->db01.repl_slave_status;
    REPL_MASTER_STATUS *m_status_db01 = &gpcb->db01.repl_master_status;

    REPL_SLAVE_STATUS *status_db02 = &gpcb->db02.repl_slave_status;
    REPL_MASTER_STATUS *m_status_db02 = &gpcb->db02.repl_master_status;

    // conn init
    if (conn_ptr_db1 == NULL || conn_ptr_db2 == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        ec_log((DEB_ERROR, ">>>[DB] INIT ERROR\n", NULL));
        return -1;
    }

    /* conn connect */
    // DB1
    if (connect_db(conn_ptr_db1, 1) == NULL)
    {
        fprintf(stderr, "Failed to connect to DB1: %s\n", mysql_error(conn_ptr_db1));
        mysql_close(conn_ptr_db1);
        ec_log((DEB_ERROR, ">>>[DB1] CONNECT ERROR\n", NULL));
        return -1;
    }

    // DB2
    if (connect_db(conn_ptr_db2, 2) == NULL)
    {
        fprintf(stderr, "Failed to connect to DB2: %s\n", mysql_error(conn_ptr_db2));
        mysql_close(conn_ptr_db1);
        mysql_close(conn_ptr_db2);
        ec_log((DEB_ERROR, ">>>[DB2] CONNECT ERROR\n", NULL));
        return -1;
    }

    /* SHOW SLAVE STATUS - DB01 */
    if (mysql_query(conn_ptr_db1, "SHOW SLAVE STATUS"))
    {
        printf("DB1 query error: %s\n", mysql_error(conn_ptr_db1));
        mysql_close(conn_ptr_db1);
        mysql_close(conn_ptr_db2);
        return -1;
    }

    res_db1 = mysql_store_result(conn_ptr_db1);
    
    int num_fields = mysql_num_fields(res_db1);
    row = mysql_fetch_row(res_db1);

    status_db01->Master_Log_File = row[5];
    status_db01->Read_Master_Log_Pos = row[6];
    status_db01->Slave_IO_Running = row[10];
    status_db01->Slave_SQL_Running = row[11];
    status_db01->Last_IO_Errno = row[34];
    status_db01->Last_IO_Error = row[35];
    status_db01->Last_SQL_Errno = row[36];
    status_db01->Last_SQL_Error = row[37];

    //mysql_free_result(res_db1);

    /* SHOW MASTER STATUS - DB01 */
    if (mysql_query(conn_ptr_db1, "SHOW MASTER STATUS"))
    {
        printf("DB1 query error: %s\n", mysql_error(conn_ptr_db1));
        mysql_close(conn_ptr_db1);
        mysql_close(conn_ptr_db2);
        return -1;
    }

    res_db1 = mysql_store_result(conn_ptr_db1);

    int num_fields2 = mysql_num_fields(res_db1);
    row = mysql_fetch_row(res_db1);
    m_status_db01->File = row[0];
    m_status_db01->Position = row[1];
    m_status_db01->Binlog_Do_DB = row[2];
    m_status_db01->Binlog_Ignore_DB = row[3];
    
    //mysql_free_result(res_db1);
    
    /* SHOW SLAVE STATUS - DB02 */
    if (mysql_query(conn_ptr_db2, "SHOW SLAVE STATUS"))
    {
        printf("DB2 query error: %s\n", mysql_error(conn_ptr_db2));
        mysql_close(conn_ptr_db1);
        mysql_close(conn_ptr_db2);
        return -1;
    }

    res_db2 = mysql_store_result(conn_ptr_db2);
    if (res_db2 == NULL)
    {
        fprintf(stderr, "res_db2 error: %s\n", mysql_error(conn_ptr_db2));
        mysql_close(conn_ptr_db2);
        mysql_close(conn_ptr_db2);
        return -1;
    }

    int num_fields1 = mysql_num_fields(res_db2);
    row = mysql_fetch_row(res_db2);

    status_db02->Master_Log_File = row[5];
    status_db02->Read_Master_Log_Pos = row[6];
    status_db02->Slave_IO_Running = row[10];
    status_db02->Slave_SQL_Running = row[11];
    status_db02->Last_IO_Errno = row[34];
    status_db02->Last_IO_Error = row[35];
    status_db02->Last_SQL_Errno = row[36];
    status_db02->Last_SQL_Error = row[37];
    
    //mysql_free_result(res_db2);

    /* SHOW MASTER STATUS - DB02 */
    if (mysql_query(conn_ptr_db2, "SHOW MASTER STATUS"))
    {
        printf("DB2 query error: %s\n", mysql_error(conn_ptr_db2));
        mysql_close(conn_ptr_db1);
        mysql_close(conn_ptr_db2);
        return -1;
    }

    res_db2 = mysql_store_result(conn_ptr_db2);

    int num_fields3 = mysql_num_fields(res_db2);
    row = mysql_fetch_row(res_db2);

    m_status_db02->File = row[0];
    m_status_db02->Position = row[1];
    m_status_db02->Binlog_Do_DB = row[2];
    m_status_db02->Binlog_Ignore_DB = row[3];

    //mysql_free_result(res_db2);
    mysql_close(conn_ptr_db1);
    mysql_close(conn_ptr_db2);
    
    return 0;
}




/* 3. ON OFF replication */
void get_replication_on(int fd)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // accept query
    if(mysql_query(conn_ptr, "start slave"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[REPLIE] Replication_start_error\n", NULL));
        return;
    }

    // restore result
    strcat(result_buffer, "START Replication" ? "START Replication" : "NULL");

    // ?å®?Ç∑ Ï§?Îπ?
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_ON; // ?å®?Ç∑ ????ûÖ ?Ñ§?†ï
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // Í≤∞Í≥º Î≤ÑÌçº Î≥µÏÇ¨
    packet.header.length = strlen(packet.buf); // ?å®?Ç∑ Í∏∏Ïù¥ ?Ñ§?†ï

    // ?ç∞?ù¥?Ñ∞ ?†Ñ?Ü°
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_start_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}

void get_replication_off(int fd)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // accept query
    if(mysql_query(conn_ptr, "stop slave"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[REPLIE] Replication_stop_error\n", NULL));
        return;
    }

    // restore result
    strcat(result_buffer, "STOP Replication" ? "STOP Replication" : "NULL");

    // ?å®?Ç∑ Ï§?Îπ?
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_OFF; // ?å®?Ç∑ ????ûÖ ?Ñ§?†ï
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // Í≤∞Í≥º Î≤ÑÌçº Î≥µÏÇ¨
    packet.header.length = strlen(packet.buf); // ?å®?Ç∑ Í∏∏Ïù¥ ?Ñ§?†ï

    // ?ç∞?ù¥?Ñ∞ ?†Ñ?Ü°
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_stop_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}
