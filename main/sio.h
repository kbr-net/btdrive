typedef struct {
	unsigned char dev;
	unsigned char cmd;
	union {
		struct {
			unsigned char aux1;
			unsigned char aux2;
		};
		unsigned short aux;
	};
	unsigned char cksum;
} cmd_buf_t;

// warning! sizeof(cmd_buf_t) = 6, 
// maybe due to even boundary allocation,
// should add -fpack-struct ?

typedef struct {
	char cmd_frame_error : 1;
	char cksum_error : 1;
	char write_error : 1;
	char write_protect : 1;
	char motor : 1;
	char sector_size : 1;
	char unused : 1;
	char medium_density : 1;

	char controller_busy : 1;
	char drq : 1;
	char data_lost : 1;
	char crc_error : 1;
	char record_not_found : 1;
	char record_type : 1;
	char wd_write_protect : 1;
	char not_ready : 1;

	char timeout;
	char wd_master_status;
} sio_status_t;

struct disk_flags {
	bool ATR;
};

struct s_device {
	sio_status_t status;
	struct disk_flags flags;
	FILE *fd;
};

void sio_init();
