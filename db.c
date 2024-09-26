#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x


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
    && strcmp(status_db02->Slave_IO_Running, "Yes") == 0 && strcmp(status_db02->Slave_SQL_Running, "Yes") == 0
    && strcmp(m_status_db01->File, status_db02->Master_Log_File) == 0 && strcmp(m_status_db01->Position, status_db02->Read_Master_Log_Pos) == 0
    && strcmp(m_status_db02->File, status_db01->Master_Log_File) == 0 && strcmp(m_status_db02->Position, status_db01->Read_Master_Log_Pos) == 0){
        return 1;
    } else if(strcmp(m_status_db01->File, status_db02->Master_Log_File) != 0 || strcmp(m_status_db01->Position, status_db02->Read_Master_Log_Pos) != 0
    || strcmp(m_status_db02->File, status_db01->Master_Log_File) != 0 || strcmp(m_status_db02->Position, status_db01->Read_Master_Log_Pos) != 0){
        return -1;
    }

    return 0;
}


char* get_select_all()
{
	MYSQL* conn = NULL;
    MYSQL_RES* res;
    MYSQL_ROW row;
    int i;

	int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn);
	if (active_db == 0){
		printf("all_db_is_down\n");
	}

    conn = mysql_init(NULL);

    connect_main_db(active_db, conn);

    if (mysql_query(conn, "select * from T_EXT_SYSTEM"))
    {
        printf("query fail\n");
    }

    res = mysql_store_result(conn);

    int num_fields = mysql_num_fields(res);
    int buffer_size = 1024; // 초기 버퍼 크기
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        return NULL;
    }

    result_buffer[0] = '\0'; // 빈 문자열로 초기화

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {
            // 필요하다면 각 필드에 대해 버퍼 크기를 동적으로 확장
            int field_length = row[i] ? strlen(row[i]) : 4; // "NULL"의 길이는 4
            int required_length = strlen(result_buffer) + field_length + 2; // 공백과 '\0' 포함

            if (required_length > buffer_size)
            {
                buffer_size *= 2;
                char* temp_buffer = (char*)realloc(result_buffer, buffer_size);
                if (temp_buffer == NULL)
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    free(result_buffer);
                    mysql_free_result(res);
                    mysql_close(conn);
                    return NULL;
                }
                result_buffer = temp_buffer;
            }

            strcat(result_buffer, row[i] ? row[i] : "NULL");
            strcat(result_buffer, " ");
        }
        strcat(result_buffer, "\n");
    }
   
    ec_log((DEB_DEBUG, ">>>[DB] SQL Request :: select * from T_EXT_SYSTEM\n", NULL));

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

int connect_main_db(int activated_db, MYSQL* conn)
{
    DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
    if(activated_db == 1){
        if(connect_db(conn, 1)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            return 0;
        }

    }

    if(activated_db == 2){
         if(connect_db(conn, 2)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            return 0;
        }

    }

    return 1;
}

MYSQL* connect_db(MYSQL* conn, int db_idx){
    DB_INFO *db = NULL;
    MYSQL* ret = NULL;
    if (db_idx == 1) {
        db = &(gpcb->db01);
    }else{
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

char* compare_table(int db_index)
{
	DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;
    MYSQL_RES* res1;
    MYSQL_RES* res2;
    MYSQL_ROW row1;
    MYSQL_ROW row2;
    char* result_buffer = (char*)malloc(BUF_SIZE);
    int buffer_size = BUF_SIZE;
    int diff_row;
    int count = 0;
    int num_fields;
    int i;

    const char *sql_select = "SELECT * FROM T_EXT_SYSTEM";

    conn1 = mysql_init(NULL);
    conn2 = mysql_init(NULL);
    
    if(connect_db(conn1, 1)==NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn1));
        mysql_close(conn1);
        free(result_buffer);
        return NULL;
    }

    if(connect_db(conn2, 2)==NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn2));
        mysql_close(conn2);
        free(result_buffer);
        return NULL;
    }

    if (mysql_query(conn1, sql_select))
    {
        printf("select query fail\n");
    }

    if (mysql_query(conn2, sql_select))
    {
        printf("select query fail\n");
    }

    res1 = mysql_store_result(conn1);
    res2 = mysql_store_result(conn2);
    if (mysql_num_fields(res1) != mysql_num_fields(res2)){
        printf("do not match column\n");
        free(result_buffer);
        return NULL;
    }
    num_fields = mysql_num_fields(res1);

    memset(result_buffer, 0x00, BUF_SIZE);
    // printf("exist diff row\n");
#if 0
    while ((row1 = mysql_fetch_row(res1)) && (row2 = mysql_fetch_row(res2))) {
#else
    while( 1 ) {
        row1 = mysql_fetch_row(res1);
        row2 = mysql_fetch_row(res2);
        if( ( row1 == NULL ) || ( row2 == NULL ) )
        {
            break;
        }
#endif
        diff_row = 0;
        for (i = 0; i < num_fields; i++) {
            // 각 열을 비교하여 다른 경우 diff_row를 1로 설정
            if ((row1[i] && row2[i] && strcmp(row1[i], row2[i]) != 0) || (row1[i] && !row2[i]) || (!row1[i] && row2[i])) {
                diff_row = 1;
                printf("Difference found at column %d: row1[%d] = %s, row2[%d] = %s\n", i, i, row1[i], i, row2[i]);
                break;
            }
        }

        // // 디버그 출력: 비교된 행의 상태를 확인하기 위해 각 행 출력
        // printf("row1: ");
        // for (i = 0; i < num_fields; i++) {
        //     printf("%s\t", row1[i] ? row1[i] : "NULL");
        // }
        // printf("\nrow2: ");
        // for (i = 0; i < num_fields; i++) {
        //     printf("%s\t", row2[i] ? row2[i] : "NULL");
        // }
        // printf("\ndiff_row: %d", diff_row);
        // printf("\n");

        // diff_row가 1인 경우에만 버퍼에 추가
        if (db_index == 1){
            if (diff_row == 1) {
                for (i = 0; i < num_fields; i++) {
                    int field_length = row1[i] ? strlen(row1[i]) : 4; // "NULL"의 길이는 4
                    int required_length = strlen(result_buffer) + field_length + 2; // 공백과 '\0' 포함
                    buffer_size = BUF_SIZE;

                    if (required_length > buffer_size)
                    {
                        buffer_size *= 2;
                        char* temp_buffer = (char*)realloc(result_buffer, buffer_size);
                        if (temp_buffer == NULL)
                        {
                            fprintf(stderr, "Memory reallocation failed\n");
                            free(result_buffer);
                            mysql_free_result(res1);
                            mysql_free_result(res2);
                            mysql_close(conn1);
                            mysql_close(conn2);
                            return NULL;
                        }
                        result_buffer = temp_buffer;
                    }
                    strncat(result_buffer, row1[i] ? row1[i] : "NULL", BUF_SIZE - strlen(result_buffer) - 1);
                    strncat(result_buffer, "\t", BUF_SIZE - strlen(result_buffer) - 1);
                }
                strncat(result_buffer, "\n", BUF_SIZE - strlen(result_buffer) - 1);
            }
        }else{
            if (diff_row == 1) {
                for (i = 0; i < num_fields; i++) {
                    int field_length = row2[i] ? strlen(row2[i]) : 4; // "NULL"의 길이는 4
                    int required_length = strlen(result_buffer) + field_length + 2; // 공백과 '\0' 포함
                    buffer_size = BUF_SIZE;

                    if (required_length > buffer_size)
                    {
                        buffer_size *= 2;
                        char* temp_buffer = (char*)realloc(result_buffer, buffer_size);
                        if (temp_buffer == NULL)
                        {
                            fprintf(stderr, "Memory reallocation failed\n");
                            free(result_buffer);
                            mysql_free_result(res1);
                            mysql_free_result(res2);
                            mysql_close(conn1);
                            mysql_close(conn2);
                            return NULL;
                        }
                        result_buffer = temp_buffer;
                    }
                    strncat(result_buffer, row2[i] ? row2[i] : "NULL", BUF_SIZE - strlen(result_buffer) - 1);
                    strncat(result_buffer, "\t", BUF_SIZE - strlen(result_buffer) - 1);
                }
                strncat(result_buffer, "\n", BUF_SIZE - strlen(result_buffer) - 1);
            }
        }
        count++;
    }
    // 남은 행을 처리하는 코드
    // printf("count: %d\n", count);
    // printf("result_buffer_len: %d\n", strlen(result_buffer));
    if (strlen(result_buffer) == 0 ) {
        printf("mysql_data_seek clear\n");
        mysql_data_seek(res1, count); 
        mysql_data_seek(res2, count); 
    }
    // printf("do not exist row\n");
    if (db_index == 1){
        while (row1 = mysql_fetch_row(res1)) {
            for (i = 0; i < num_fields; i++) {
                strncat(result_buffer, row1[i] ? row1[i] : "NULL", BUF_SIZE - strlen(result_buffer) - 1);
                strncat(result_buffer, "\t", BUF_SIZE - strlen(result_buffer) - 1);
            }
            strncat(result_buffer, "\n", BUF_SIZE - strlen(result_buffer) - 1);
        }
    }else{
        while (row2 = mysql_fetch_row(res2)) {
            for (i = 0; i < num_fields; i++) {
                strncat(result_buffer, row2[i] ? row2[i] : "NULL", BUF_SIZE - strlen(result_buffer) - 1);
                strncat(result_buffer, "\t", BUF_SIZE - strlen(result_buffer) - 1);
            }
            strncat(result_buffer, "\n", BUF_SIZE - strlen(result_buffer) - 1);
        }
    }
    

    ec_log((DEB_DEBUG, ">>>[DB] Request Check DB data\n", NULL));

    mysql_free_result(res1);
    mysql_free_result(res2);
    mysql_close(conn1);
    mysql_close(conn2);

    printf("==last result_buffer== \n %s \n",result_buffer);

    return result_buffer;
}



void* db_sync(void* args){
    printf("=================== START DB SYNC ===================\n");
    int db_id = atoi(args);
    FILE *fp;
    char path[BUF_SIZE];
    char* file_name;
    int ret;
    int t_ret = 1;
    
    DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;

    MYSQL_RES* res;
    MYSQL_ROW row;

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
    }
    
     /*db01, db02 stop slave*/
    if (mysql_query(conn1, "stop slave")){
        printf("db01 stop slave fail\n");
        return NULL;
    }

    printf("db01 stop slave success\n");

    if (mysql_query(conn2, "stop slave")){
        printf("db02 stop slave fail\n");
        return NULL;
    }

    printf("db02 stop slave success\n");


    if (db_id == 1){
        printf("Sync to DB01.\n");
        /*db01 reset Master*/
        if (mysql_query(conn1, "reset master")){
            printf("db01 reset master fail\n");
            return NULL;
        }

        if (mysql_query(conn1, "FLUSH TABLES WITH READ LOCK")){
            printf("db01 lock master table fail\n");
            return NULL;
        }

        /*db01 make dump*/
        fp = popen("mysqldump repl_test01 T_EXT_SYSTEM > /home/kim/backup/db01/master_dump.sql", "r");
        if (fp == NULL){
            printf("popen fail\n");
            return NULL;
        }

        printf("Make Dump Success!\n");

        if (mysql_query(conn1, "UNLOCK TABLES")){
            printf("db01 unlock master table fail\n");
            return NULL;
        }

        pclose(fp);
        fp = NULL;

        file_name = "/home/kim/backup/db01/master_dump.sql";        //check dump file name

        //printf("old-path(Len:%02d)=[%s]\n", strlen(path), path);

        /*check dump file exist*/
        while(1){
            fp = popen("ls /home/kim/backup/db01/master_dump.sql", "r");
  
            if (fgets(path, sizeof(path), fp) != NULL) {
                path[37] = 0;
                //printf("new-path(Len:%02d)=[%s] [%d(0x%X)]\n", strlen(path), path, path[37], path[37]);
                //printf("cmp-file(Len:%02d)=[%s]\n", strlen(file_name), file_name);
                if(strcmp(path, file_name) == 0){
                    printf("master_dump.sql is exist!\n");
                    break;
                }
            }
        }

        pclose(fp);
        fp = NULL;
        
        /*start slaveDB work*/
        /*rename original table to backup table*/
        if (mysql_query(conn2, "DROP TABLE IF EXISTS T_EXT_SYSTEM_backup"))
        {
            printf("drop query fail\n");
            return NULL;
        }

        printf("db02 drop backup table success!\n");

        if (mysql_query(conn2, "RENAME TABLE T_EXT_SYSTEM TO T_EXT_SYSTEM_backup")){
            printf("rename query fail\n");
            return NULL;
        }

        printf("db02 rename table success!\n");

        /*exec db01 dump*/
        ret = system("ssh root@10.0.2.4 mysql repl_test01 < /home/kim/backup/db01/master_dump.sql");
        if (ret != 0) {
            printf("sshError\n");
            return NULL;
        }

        printf("exec sql file success!\n");

        /*db02 reset master, slave*/
        if (mysql_query(conn2, "reset master")){
            printf("db02 reset master fail\n");
            return NULL;
        }

        printf("db02 reset master success!\n");

        if (mysql_query(conn2, "reset slave")){
            printf("db02 reset slave fail\n");
            return NULL;
        }

        printf("db02 reset slave success!\n");

    }

    if (db_id == 2){
        printf("Sync to DB02.\n");
        /*db02 reset Master*/
        if (mysql_query(conn2, "reset master")){
            printf("db02 reset master fail\n");
            return NULL;
        }

        if (mysql_query(conn2, "FLUSH TABLES WITH READ LOCK")){
            printf("db02 lock master table fail\n");
            return NULL;
        }

        /*db02 make dump*/
        fp = popen("ssh root@10.0.2.4 mysqldump repl_test01 T_EXT_SYSTEM > /home/kim/backup/db02/slave_dump.sql", "r");
        if (fp == NULL){
            printf("popen fail\n");
            return NULL;
        }

        printf("Make Dump Success!\n");

        if (mysql_query(conn2, "UNLOCK TABLES")){
            printf("db02 unlock master table fail\n");
            return NULL;
        }

        pclose(fp);
        fp = NULL;

        file_name = "/home/kim/backup/db02/slave_dump.sql";

        //printf("old-path(Len:%02d)=[%s]\n", strlen(path), path);

        /*check dump file exist*/
        while(1){
            fp = popen("ls /home/kim/backup/db02/slave_dump.sql", "r");
  
            if (fgets(path, sizeof(path), fp) != NULL) {
                path[36] = 0;
                //printf("new-path(Len:%02d)=[%s] [%d(0x%X)]\n", strlen(path), path, path[36], path[36]);
                //printf("cmp-file(Len:%02d)=[%s]\n", strlen(file_name), file_name);
                if(strcmp(path, file_name) == 0){
                    printf("slave_dump.sql is exist!\n");
                    break;
                }
            }
        }

        pclose(fp);
        fp = NULL;

        /*start slaveDB work*/
        /*rename original table to backup table*/
        if (mysql_query(conn1, "DROP TABLE IF EXISTS T_EXT_SYSTEM_backup"))
        {
            printf("drop query fail\n");
            return NULL;
        }

        printf("db01 drop backup table success!\n");

        if (mysql_query(conn1, "RENAME TABLE T_EXT_SYSTEM TO T_EXT_SYSTEM_backup"))
        {
            printf("rename query fail\n");
            return NULL;
        }

        printf("db01 rename table success!\n");

        /*exec db02_dump*/
        ret = system("mysql repl_test01 < /home/kim/backup/db02/slave_dump.sql");
        if (ret != 0) {
            printf("sshError\n");
            return NULL;
        }

        printf("exec sql file success!\n");

        /*db01 reset master, slave*/

        if (mysql_query(conn1, "reset master")){
            printf("db01 reset master fail\n");
            return NULL;
        }

        printf("db01 reset master success!\n");

        if (mysql_query(conn1, "reset slave")){
            printf("db01 reset slave fail\n");
            return NULL;
        }

        printf("db01 reset slave success!\n");

    }

    /*update replication info*/
    get_repcheck_status();

    /*set replication info*/
    if(repl_prepare(conn1, conn2)==-1){
        printf("repl_prepare query error\n");
        return NULL;
    }

    /*db01, db02 start slave*/
    if (mysql_query(conn1, "start slave")){
        printf("db01 start slave fail\n");
        return NULL;
    }

    printf("db01 start slave\n");

    if (mysql_query(conn2, "start slave")){
        printf("db02 start slave fail\n");
        return NULL;
    }

    printf("db02 start slave\n");

    mysql_close(conn1);
    mysql_close(conn2);

    printf("=================== END DB SYNC ===================\n");

    return (void*)1;
}

int repl_prepare(MYSQL* conn1, MYSQL* conn2){
    char change_master01[BUF_SIZE];
    char change_master02[BUF_SIZE];
    snprintf(change_master01, BUF_SIZE, \
    "CHANGE MASTER TO MASTER_HOST = '%s' "\
    ", MASTER_PORT=%d "\
    ", MASTER_USER='%s' "\
    ", MASTER_PASSWORD='%s' "\
    ", MASTER_LOG_FILE='%s' "\
    ", MASTER_LOG_POS=%s "\
    ", MASTER_CONNECT_RETRY=10; "\
    , gpcb->db02.host, gpcb->db02.port, gpcb->db02.username, gpcb->db02.password, gpcb->db02.repl_master_status.File, gpcb->db02.repl_master_status.Position);

    snprintf(change_master02, BUF_SIZE, \
    "CHANGE MASTER TO MASTER_HOST = '%s' "\
    ", MASTER_PORT=%d "\
    ", MASTER_USER='%s' "\
    ", MASTER_PASSWORD='%s' "\
    ", MASTER_LOG_FILE='%s' "\
    ", MASTER_LOG_POS=%s "\
    ", MASTER_CONNECT_RETRY=10; "\
    , gpcb->db01.host, gpcb->db01.port, gpcb->db01.username, gpcb->db01.password, gpcb->db01.repl_master_status.File, gpcb->db01.repl_master_status.Position);

    if (mysql_query(conn1, change_master01)){
        printf("change_master01 fail\n");
        return -1;
    }
    
    if (mysql_query(conn2, change_master02)){
        printf("change_master02 fail\n");
        return -1;
    }

    return 1;
}

void print_db_ver()
{
	printf("MySQL Client Version: %s\n", mysql_get_client_info());

}

/* hyogi code */
/* 2. replication check */

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
    status_db01->Last_IO_Error = strcmp(row[35], "") == 0 ? "NULL" : row[35];
    status_db01->Last_SQL_Errno = row[36];
    status_db01->Last_SQL_Error = strcmp(row[37], "") == 0 ? "NULL" : row[37];

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
    status_db02->Last_IO_Error = strcmp(row[35], "") == 0 ? "NULL" : row[35];
    status_db02->Last_SQL_Errno = row[36];
    status_db02->Last_SQL_Error = strcmp(row[37], "") == 0 ? "NULL" : row[37];
    
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




// /* 3. ON OFF replication */
// void get_replication_on(int fd)
// {
//     Packet packet;
//     MYSQL *conn_ptr = mysql_init(NULL);

//     // conn init
//     if (conn_ptr == NULL) {
//         fprintf(stderr, "mysql_init() failed\n");
//         return; 
//     }
    
//     // conn exception
//     int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
//     if (active_db == 0){
// 		printf("all_db_is_down\n");
//         send_message(fd, REQ_SVCL_EVT_WARNING, "ALL DB IS DOWN");
//         mysql_close(conn_ptr);
//         return;
// 	}

//     // conn connect
//     connect_main_db(active_db, conn_ptr);

//     // buffer exception
//     char* result_buffer = (char*)malloc(BUF_SIZE);
//     if (result_buffer == NULL)
//     {
//         fprintf(stderr, "Memory allocation failed\n");
//         mysql_close(conn_ptr);
//         return;
//     }

//     // buffer last string
//     result_buffer[0] = '\0';

//     // buffer init
//     memset(result_buffer, 0, BUF_SIZE);

//     // accept query
//     if(mysql_query(conn_ptr, "start slave"))
//     {
//         printf("query error: %s\n", mysql_error(conn_ptr));
//         ec_log((DEB_ERROR, ">>>[REPLIE] Replication_start_error\n", NULL));
//         return;
//     }

//     // restore result
//     strcat(result_buffer, "START Replication" ? "START Replication" : "NULL");

//     // ?뙣?궥 以?鍮?
//     printf("Result:\n%s\n", result_buffer);
//     packet.header.type = REP_ON; // ?뙣?궥 ????엯 ?꽕?젙
//     strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 寃곌낵 踰꾪띁 蹂듭궗
//     packet.header.length = strlen(packet.buf); // ?뙣?궥 湲몄씠 ?꽕?젙

//     // ?뜲?씠?꽣 ?쟾?넚
//     send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
//     ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_start_success\n", NULL));
//     free(result_buffer);
//     mysql_close(conn_ptr);
// }

// void get_replication_off(int fd)
// {
//     Packet packet;
//     MYSQL *conn_ptr = mysql_init(NULL);

//     // conn init
//     if (conn_ptr == NULL) {
//         fprintf(stderr, "mysql_init() failed\n");
//         return; 
//     }
    
//     // conn exception
//     int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
//     if (active_db == 0){
// 		printf("all_db_is_down\n");
//         send_message(fd, REQ_SVCL_EVT_WARNING, "ALL DB IS DOWN");
//         mysql_close(conn_ptr);
//         return;
// 	}

//     // conn connect
//     connect_main_db(active_db, conn_ptr);

//     // buffer exception
//     char* result_buffer = (char*)malloc(BUF_SIZE);
//     if (result_buffer == NULL)
//     {
//         fprintf(stderr, "Memory allocation failed\n");
//         mysql_close(conn_ptr);
//         return;
//     }

//     // buffer last string
//     result_buffer[0] = '\0';

//     // buffer init
//     memset(result_buffer, 0, BUF_SIZE);

//     // accept query
//     if(mysql_query(conn_ptr, "stop slave"))
//     {
//         printf("query error: %s\n", mysql_error(conn_ptr));
//         ec_log((DEB_ERROR, ">>>[REPLIE] Replication_stop_error\n", NULL));
//         return;
//     }

//     // restore result
//     strcat(result_buffer, "STOP Replication" ? "STOP Replication" : "NULL");

//     // ?뙣?궥 以?鍮?
//     printf("Result:\n%s\n", result_buffer);
//     packet.header.type = REP_OFF; // ?뙣?궥 ????엯 ?꽕?젙
//     strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 寃곌낵 踰꾪띁 蹂듭궗
//     packet.header.length = strlen(packet.buf); // ?뙣?궥 湲몄씠 ?꽕?젙

//     // ?뜲?씠?꽣 ?쟾?넚
//     send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
//     ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_stop_success\n", NULL));
//     free(result_buffer);
//     mysql_close(conn_ptr);
// }
