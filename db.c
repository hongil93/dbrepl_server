#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;


int check_db()
{
	DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;

    conn1 = mysql_init(NULL);
    conn2 = mysql_init(NULL);
    
    if(connect_db(conn1, 1)==NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn1));
        gpcb->db01.status = 0;
        printf("Can not connect to DB01\n");
        mysql_close(conn1);
    }else{
        gpcb->db01.status = 1;
        mysql_close(conn1);
    }

    if(connect_db(conn2, 2)==NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn2));
        gpcb->db02.status = 0;
        printf("Can not connect to DB02\n");
        mysql_close(conn2);
    }else{
        gpcb->db02.status = 1;
        mysql_close(conn2);
    }
    if(gpcb->db01.status == 0 && gpcb->db02.status == 0){
        return 0;
    }

    if((gpcb->db01.status) == 0){
        return 1;
    }
    if((gpcb->db02.status) == 0){
        return 2;
    }

    if (gpcb->db01.status == 1 && gpcb->db02.status ==1){
        return 3;
    }
    return -1;

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
        return NULL;
	}

    conn = mysql_init(NULL);
   
    connect_main_db(active_db, conn);

    if (mysql_query(conn, query))
    {
        printf("query fail\n");
        ec_log((DEB_ERROR, ">>>[db] mysql_query_error\n", NULL));
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
    ec_log((DEB_DEBUG, ">>>[DB] SQL Request :: select * from user\n", NULL));

    //free(result_buffer);
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
    int buffer_size = 1024; // �ʱ� ���� ũ��
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        exit(1);
    }

    result_buffer[0] = '\0'; // �� ���ڿ��� �ʱ�ȭ

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {
            // �ʿ��ϴٸ� �� �ʵ忡 ���� ���� ũ�⸦ �������� Ȯ��
            int field_length = row[i] ? strlen(row[i]) : 4; // "NULL"�� ���̴� 4
            int required_length = strlen(result_buffer) + field_length + 2; // ����� '\0' ����

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
void get_repcheck_status(int fd)
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

    // query exception
    if (mysql_query(conn_ptr, "SHOW SLAVE STATUS"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn_ptr);
    // result exception
    if (result == NULL)
    {
        fprintf(stderr, "result error: %s\n", mysql_error(conn_ptr));
        return;
    }

    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)) != NULL) {
        for (int i = 0; i < num_fields; i++) {
            const char *field_name = mysql_fetch_field_direct(result, i)->name;

            if(strcmp(field_name, "Slave_IO_Running") == 0)
            {   
                printf("=======================================\n");
                printf("\tSlave_IO_Running: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
            }

            if (strcmp(field_name, "Slave_SQL_Running") == 0)
			{
				printf("\tSlave_SQL_Running: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_IO_Errno") == 0)
			{
				printf("\tLast_IO_Errno: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_IO_Error") == 0)
			{
                if(strcmp(row[i], "") == 0){
                    strcat(result_buffer, "empty");
                    printf("\tLast_IO_Error: %s\n", "empty");
                }else{
                    strcat(result_buffer, row[i]);
                    printf("\tLast_IO_Error: %s\n", row[i]);
                }
                strcat(result_buffer, ","); // 띄어쓰기로 구분
            }   

            if (strcmp(field_name, "Last_SQL_Errno") == 0)
			{
				printf("\tLast_SQL_Errno: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_SQL_Error") == 0)
			{
                if(strcmp(row[i], "") == 0){
                    strcat(result_buffer, "empty");
                    printf("\tLast_SQL_Error: %s\n", "empty");
                }else{
                    strcat(result_buffer, row[i]);
                    printf("\tLast_SQL_Error: %s\n", row[i]);
                }
                printf("=======================================\n\n");
            }   
        }
    }
    mysql_free_result(result);

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_CHECK; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_status_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
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

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_ON; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
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

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_OFF; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_stop_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}
/* CRUD mysql query */
void get_sql_insert_table(int fd, const char *query)
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
    if(mysql_query(conn_ptr, query))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[db] mysql_query_error\n", NULL));
        return;
    }
    // restore result
    strcat(result_buffer, "INSERT SUCCESS" ? "INSERT SUCCESS" : "NULL");
    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = SQL_INSERT; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[db] mysql_send_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}

