// gcc -g -lusb main.c
#include <stdio.h>
#include <unistd.h>
#include <linux/can.h>
#include <pthread.h>
#include <sqlite3.h>

#define NUM_THREADS 2


/* create thread argument struct for thr_func() */
typedef struct _thread_data_t {
  FILE *fp;
  struct libusb_device_handle *devh;
  int write_ready;
  int read_ready;
  unsigned char buf_write[BUF_SIZE];	
  unsigned char buf_read[BUF_SIZE];
  time_t time;
} thread_data_t;


 
thread_data_t thdata; 

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_write = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_read = PTHREAD_COND_INITIALIZER;

int write_index = 0;



static int callback(void *arg, int argc, char **argv, char **azColName)
{
	int i;
	char sqlbuf[128];
	sqlite3 *db = (sqlite3 *)arg;
	int rc;

	for(i=0; i<argc; i++)
	{
		//printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");

		sprintf(sqlbuf, "delete from cmd where id = %s;", argv[0]);
		rc = sqlite3_exec(db, sqlbuf, 0, 0, 0);
		char candata[127];
		int slot;
		int boardtype;
		
		
	}
	
	printf("\n");
	return 0;
}


/* 发送处理线程 */
void *thr_send(void *arg) 
{
		int sock;
		struct sockaddr_can addr;
		struct ifreq ifr;
		int ret;
		int recv_own_msgs = 0; //set loop back:  1 enable 0 disable
	
		sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
		if(sock < 0) {
			printf("fts_can_send_and_recv socket error\n");
			exit(1);
		}
	
		addr.can_family = AF_CAN;
		strcpy(ifr.ifr_name, "can0"); //?? which can should i use
		ret = ioctl(sock, SIOCGIFINDEX, &ifr);	//get index
		if(ret && ifr.ifr_ifindex == 0)
		{
			printf("Can't get interface index for can0, code= %d, can0 ifr_ifindex value: %d, name: %s\n", ret, ifr.ifr_ifindex, ifr.ifr_name);
			close(sock);
			exit(1);
		}
		addr.can_ifindex = ifr.ifr_ifindex;

		sqlite3 *db;
		char *zErrMsg = 0;
		int rc;
		rc = sqlite3_open(ST_DATABASE_FILE_NAME, &db);
		if( rc )
		{
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			exit(1);
		}
		sqlite3_stmt *pStmt = 0;
		char *zSql;

		rc = sqlite3_exec(db, "select * from cmd", callback, (void *)db, 0);
		if (rc != SQLITE_OK)
		{
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
		sqlite3_close(db);

		while(1)
		{
				rc = sqlite3_open(ST_DATABASE_FILE_NAME, &db);
				if( rc )
				{
					fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
					sqlite3_close(db);
					exit(1);
				}
				rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
				if( rc!=SQLITE_OK )
				{
					fprintf(stderr, "SQL error: %s\n", zErrMsg);
					sqlite3_free(zErrMsg);
				}
				sqlite3_close(db);
	
			ret = write(sock, (void *)send_frame, sizeof(struct can_frame));
		}
	
		close(sock);
}
void can_send_packet(int nID, int protocal, unsigned char *data, unsigned int len)
{
	int sock;
	struct sockaddr_can addr;
	struct ifreq ifr;
	int ret;
	int recv_own_msgs = 0; //set loop back:  1 enable 0 disable
	
	sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(sock < 0) {
		printf("fts_can_send_and_recv socket error\n");
		exit(1);
	}
	
	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, "can0"); //?? which can should i use
	ret = ioctl(sock, SIOCGIFINDEX, &ifr);	//get index
	if(ret && ifr.ifr_ifindex == 0)
	{
		printf("Can't get interface index for can0, code= %d, can0 ifr_ifindex value: %d, name: %s\n", ret, ifr.ifr_ifindex, ifr.ifr_name);
		close(sock);
		exit(1);
	}
	addr.can_ifindex = ifr.ifr_ifindex;

	int frame_cnt;
	int frame_index = 0;
	char chksum = 0;
	struct can_frame frame;
	frame_cnt = len/8+1; 
	frame->can_dlc = 8;
	while(frame_index < frame_cnt)
	{
		int offset;
		
		offset = frame_index * 8;
		memset(frame->data, 0, 8);
		frame->can_id = ST_CAN_PROTOCAL_VERSION|protocal<<24|frame_cnt<<20|frame_index<<16|nID<<8|0xF0;
		frame->can_dlc = 1;
		if (len - frame_index * 8 < 8)
			memcpy(frame->data, data+offset, len-offset);
		else
			memcpy(frame->data, data+offset, 8);
		chksum += frame->data[0]+frame->data[1]+frame->data[2]+frame->data[3]+
			frame->data[4]+frame->data[5]+frame->data[6]+frame->data[7];
		chksum = ~chksum;
		frame_index++;
		ret = write(sock, (void *)send_frame, sizeof(struct can_frame));
	}
	
	close(sock);
}


void *thr_send1(void *arg) 
{
		int sock;
		struct sockaddr_can addr;
		struct ifreq ifr;
		int ret;
		int recv_own_msgs = 0; //set loop back:  1 enable 0 disable
	
		sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
		if(sock < 0) {
			printf("fts_can_send_and_recv socket error\n");
			exit(1);
		}
	
		addr.can_family = AF_CAN;
		strcpy(ifr.ifr_name, "can0"); //?? which can should i use
		ret = ioctl(sock, SIOCGIFINDEX, &ifr);	//get index
		if (ret && ifr.ifr_ifindex == 0)
		{
			printf("Can't get interface index for can0, code= %d, can0 ifr_ifindex value: %d, name: %s\n", ret, ifr.ifr_ifindex, ifr.ifr_name);
			close(sock);
			exit(1);
		}
		addr.can_ifindex = ifr.ifr_ifindex;

		sqlite3 *db;
		char *zErrMsg = 0;
		int rc;
		rc = sqlite3_open(ST_DATABASE_FILE_NAME, &db);
		if( rc )
		{
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			exit(1);
		}
		sqlite3_stmt *stmt_select = 0;
		sqlite3_stmt *stmt_del = 0;
		int board_type;
		int nID;
		int id;
		int cmd_type;
		char *zSql;
		unsigned char *blob;
		int blob_len;
		unsigned char packet_data[127];
		int packet_len;

		sqlite3_prepare(db, "select * from cmd;", -1, &stmt_select, 0);

		while (sqlite3_step(stmt_select) != SQLITE_DONE)
		{
			id = sqlite3_column_int(stmt_select, 0);
			sqlite3_prepare(db, "del from cmd where id=:id;", -1, &stmt_del, 0);
			sqlite3_bind_int(stmt_del, 0, id);
			sqlite3_step(stmt_del);
			board_type = sqlite3_column_int(stmt_select, 1);
			nID = sqlite3_column_int(stmt_select, 2);
			cmd_type = sqlite3_column_int(stmt_select, 3);
			blob = sqlite3_column_blob(stmt_select, 4);
			blob_len = sqlite3_column_bytes(stmt_select, 4);
			packet_data[0] = board_type;
			packet_data[1] = cmd_type;
			memcpy(packet_data+2, blob, blob_len);
			packet_len = blob_len+2;
			can_send_packet(nID, ST_CAN_PROTOCAL_PARAM_SET, packet_data, packet_len);
		}
		sqlite3_finalize(pStmt);
		sqlite3_close();

	
}
/* some other thread code that signals a waiting thread that MAX_COUNT has been reached */
int read_index = 0;

/* 接收处理线程 */

void *thr_recv(void *arg) 
{
	thread_data_t *td = (thread_data_t *)arg;
	int read_size;


	while (1)
	{
			pthread_mutex_lock(&lock);
			/* some code here that does interesting stuff and modifies count */
			while(1)
			{
				if (td->read_ready == 0)
				{
				pthread_cond_wait(&cond_read, &lock);
				}
				else
					break;
			}
			pthread_mutex_unlock(&lock);
			
			libusb_bulk_transfer(td->devh, EP_IN, td->buf_read, BUF_SIZE, &read_size, 1000); 
			if (read_size != BUF_SIZE)
			{
				printf("read_size:%d\n", read_size);
				return;
			}
			
			pthread_mutex_lock(&lock);
			printf("r:%d\n", ++read_index);
			td->write_ready = 1;
			td->read_ready = 0;
			pthread_mutex_unlock(&lock);
			pthread_cond_signal(&cond_write);

			
			if (time(NULL)-td->time == READ_TIME)
			{
				exit(0);
			}
	}
} 

void *thr_manage(void *arg)
{

}

int main() 
{
	pthread_t thr[2];
	int ret;
	


	thdata.time = time(NULL);
	thdata.write_ready = 0;
	thdata.read_ready = 1;
	thdata.devh = devh;
	thdata.fp = fopen("usbdada.data", "w+");
	
	ret = pthread_create(&thr[0], NULL, thr_recv, &thdata);
	if (ret != 0)
	{
		fprintf(stderr, "error: pthread_create thr_func0, rc: %d\n", ret);
		return 1;
	}
	ret = pthread_create(&thr[1], NULL, thr_send, &thdata);
	if (ret != 0)
	{
		fprintf(stderr, "error: pthread_create thr_func1, rc: %d\n", ret);
		return 1;
	}
	ret = pthread_create(&thr[1], NULL, thr_manage, &thdata);
	if (ret != 0)
	{
		fprintf(stderr, "error: pthread_create thr_func1, rc: %d\n", ret);
		return 1;
	}

	
	pthread_join(thr[0], NULL);
	pthread_join(thr[1], NULL);
	 

	return 0;
}




