/*
		while(1) {
			printf("wait...\n");
			//accept process
			errno = 0;
			recv_msg = read(clfd, buffer, 1024);
			if( recv_msg <= 0 )
			{
				//printf( "Close by Peer!! read_size=%d/errno = %d(%s)\n", recv_msg, errno, strerror(errno) );
				printf( "Close by Peer!!\n" );
				close( clfd );
				break;
			}
			printf("received msg: %s\n", buffer);
		}
		*/

// recv_msg = recv(clfd, buffer, sizeof(buffer), 0);
				// if( recv_msg <= 0 )
				// {
				// 	//printf( "Close by Peer!! read_size=%d/errno = %d(%s)\n", recv_msg, errno, strerror(errno) );
				// 	printf( "Close by Peer!!\n" );
				// 	close( clfd );
				// 	break;
				// }

				// if (recv_msg > 0) {
				// 	buffer[recv_msg] = '\0'; // �� ���� �߰�
				// 	printf("Received message: %s", buffer);

				// 	// ���� ���� ���ڰ� ������ �߰�
				// 	if (buffer[recv_msg - 1] != '\n') {
				// 		printf("\n");
				// 	}
				// }