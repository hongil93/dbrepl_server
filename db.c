#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;

int connecting_test(MYSQL* conn1, MYSQL* conn2)
{
	printf("start connecting_db\n");
    conn1 = mysql_init(NULL);

	if(mysql_real_connect(conn1, "localhost", "repluser", "repl", "repl_test01", 3306, NULL, 0)==NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(conn1));
		printf("Can not connect to DB01");
		mysql_close(conn1);
	}

	printf("DB01 is ON\n");
	
	mysql_close(conn1);

	conn2 = mysql_init(NULL);

	if(mysql_real_connect(conn2, "10.0.2.4", "repluser", "repl", "repl_test01", 3306, NULL, 0)==NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(conn2));
		printf("Can not connect to DB02");
		mysql_close(conn2);
	}

    printf("DB02 is ON\n");

	mysql_close(conn2);

	return 0;	
}

void* check_db(void* args)
{

	DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	//DB_INFO *db01 = &gpcb->db01;
	//DB_INFO *db02 = &gpcb->db02;
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;
	while(1)
	{
		conn1 = mysql_init(NULL);
		conn2 = mysql_init(NULL);

		if(mysql_real_connect(conn1, 
					db01->host,
					db01->username, 
					db01->password,
					db01->dbname,
					db01->port,
					NULL,
					0)==NULL)
		{
			fprintf(stderr, "%s\n", mysql_error(conn1));
			gpcb->db01.status = 0;
			printf("Can not connect to DB01\n");
			mysql_close(conn1);
		}else{
			gpcb->db01.status = 1;
			printf("DB01 is ON\n");
			mysql_close(conn1);
		}

		if(mysql_real_connect(conn2, 
					db02->host,
					db02->username, 
					db02->password,
					db02->dbname,
					db02->port,
					NULL,
					0)==NULL)
		{
			fprintf(stderr, "%s\n", mysql_error(conn2));
			gpcb->db02.status = 0;
			printf("Can not connect to DB02\n");
			mysql_close(conn2);
		}else{
			gpcb->db02.status = 1;
			printf("DB02 is ON\n");
			mysql_close(conn2);
		}

        if(gpcb->db01.status == 0 && gpcb->db02.status == 0){
            broadcast_message("ALL DB IS DOWN", EVT_WARNING);
            ec_log((DEB_ERROR, ">>>[DB] ALL DB IS DOWN\n", NULL));
            sleep(5);
            continue;
        }

        if((gpcb->db01.status) == 0){
            broadcast_message("DB01 IS DOWN", EVT_WARNING);
            ec_log((DEB_WARN, ">>>[DB] DB01 IS DOWN\n", NULL));
            sleep(5);
            continue;
		}
        if((gpcb->db02.status) == 0){
            broadcast_message("DB02 IS DOWN", EVT_WARNING);
            ec_log((DEB_WARN, ">>>[DB] DB02 IS DOWN\n", NULL));
            sleep(5);
            continue;
		}

        if (gpcb->db01.status == 1 && gpcb->db02.status ==1){
            ec_log((DEB_DEBUG, ">>>[DB] db_check_success\n", NULL));
            sleep(5);
            continue;
		}
	}

}

void get_select_all(int fd)
{
    Packet packet;
	MYSQL* conn;
    MYSQL_RES* res;
    MYSQL_ROW row;
	int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn);
	if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        return;
	}

    conn = mysql_init(NULL);
    int i;

    connect_main_db(active_db, conn);

    if (mysql_query(conn, "select * from user"))
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
        exit(1);
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

    printf("Result:\n%s", result_buffer);
    packet.header.type = SQL_SELECT;
	strncpy(packet.buf, result_buffer, BUF_SIZE -1);
    packet.header.length = strlen(packet.buf);

    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    printf("send select * data\n");
    ec_log((DEB_DEBUG, ">>>[DB] SQL Request :: select * from user\n", NULL));

    free(result_buffer);
    mysql_free_result(res);
    mysql_close(conn);
}

void selectall(MYSQL* conn)
{
    printf("start selectall\n");
    MYSQL_RES* res;
    MYSQL_ROW row;
    conn = mysql_init(NULL);
    int i;

    if (mysql_real_connect(conn, "10.0.2.4", "root", "root", "repl_test01", 3306, NULL, 0) == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(0);
    }
    printf("selectall-connected\n");

    if (mysql_query(conn, "select * from user"))
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
        exit(1);
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

    printf("Result:\n%s", result_buffer);

    free(result_buffer);
    mysql_free_result(res);
    mysql_close(conn);
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
        if(mysql_real_connect(conn, 
			db01->host,
			db01->username, 
			db01->password,
			db01->dbname,
			db01->port,
			NULL,
			0)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }

    if(activated_db == 2){
        if(mysql_real_connect(conn, 
			db02->host,
			db02->username, 
			db02->password,
			db02->dbname,
			db02->port,
			NULL,
			0)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }
}

void print_db_ver()
{
	printf("MySQL Client Version: %s\n", mysql_get_client_info());

}
