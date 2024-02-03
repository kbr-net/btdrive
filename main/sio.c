#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include "sio.h"

#define TRUE 1
#define FALSE 0
#define SIO_ACK_DELAY 250	// in µs
//#define portTICK_PERIOD_US ( ( TickType_t ) 1000000 / configTICK_RATE_HZ )
#define ATARI_SPEED_STD 19200
#define ATARI_FREQ 1778400
#define high_baud ATARI_FREQ/2/(pokey_div+7)
#define LIST_BUF_SIZE 512

char disk_buffer[256];
unsigned char pokey_div = 6;
TaskHandle_t xSioHandle;
cmd_buf_t cmd_buf;	// warning! sizeof(cmd_buf_t) = 6,
			// maybe due to even boundary allocation

sio_status_t sio_status = { //0x00,0xff,0xe0,0x00
	.controller_busy = 1,
	.drq = 1,
	.data_lost = 1,
	.crc_error = 1,
	.record_not_found = 1,
	.record_type = 1,
	.wd_write_protect = 1,
	.not_ready = 1,
	.timeout = 0xe0,
};

struct disk_flags {
	bool ATR;
	bool HIGH;
} flags;

char sio_crc (char* buffer, unsigned short len)
{
        unsigned short i;
        unsigned char sumo,sum;
        sum=sumo=0;
        for(i=0;i<len;i++)
        {
                sum+=buffer[i];
                if(sum<sumo) sum++;
                sumo = sum;
        }
        return sum;
}

bool sio_get_command ()
{
	if (serial_rx((char *) &cmd_buf, 5) <= 0)
		return(FALSE);
	// check crc
	unsigned char crc = sio_crc((char *)&cmd_buf, 4);
	if (crc != cmd_buf.cksum) {
		printf("sio crc error: %x != %x\n", cmd_buf.cksum, crc);
		if (! flags.HIGH) {
			serial_change_baud(high_baud);
			flags.HIGH = 1;
			printf("baud: %u\n", high_baud);
		}
		else {
			serial_change_baud(ATARI_SPEED_STD);
			flags.HIGH = 0;
			printf("baud: %u\n", ATARI_SPEED_STD);
		}
		return(FALSE);
	}
	return(TRUE);
}

void sio_send_byte (char cmd)
{
	serial_tx(&cmd, 1);
}

void sio_send_ack ()
{
	sio_send_byte('A');
	ets_delay_us(SIO_ACK_DELAY);
}

FILE * sio_insert_disk (FILE *fd, unsigned short idx)
{
	FILE *newfd;
	DIR *dir;
	struct dirent *de;
	char name[256];

	dir = opendir("/a/");
	seekdir(dir, idx);
	de = readdir(dir);
	if (! de) {
		puts("file not found");
		goto err;
	}

	strcpy(name, "/a/");
	strcat(name, de->d_name);
	printf("open: %s\n", name);

	newfd = fopen(name, "r");
	if (! newfd) {
		perror("open");
	}
	else {
		if (fd) fclose(fd);
		fd = newfd;

		// detect file type and set flags
		struct stat st;
		fstat(fileno(fd), &st);
		printf("size: %lu\n", st.st_size);
		flags.ATR = 1;	//no other supported yet
		if (st.st_size > 1040*128+16) {
			puts("double");
			sio_status.medium_density = 0;
			sio_status.sector_size = 1;
		}
		else if (st.st_size > 720*128+16) {
			puts("medium");
			sio_status.medium_density = 1;
			sio_status.sector_size = 0;
		}
		else {
			puts("single");
			sio_status.medium_density = 0;
			sio_status.sector_size = 0;
		}
	}
err:
	closedir(dir);
	return(fd);
}

void sio_get_files ()
{
	char *buf = calloc(1, LIST_BUF_SIZE);
	if (! buf) {
		perror("malloc");
		return;
	}
	char *bufptr = buf;

        DIR *dir = opendir("/a/");
        struct dirent *entry;
        struct stat st;
        char name[256];
        unsigned int idx = 0;
        bufptr += sprintf(bufptr, "Idx\x7fSize\x7fName\x9b");
        while((entry = readdir(dir))) {
                strcpy(name, "/a/");
                strcat(name, entry->d_name);
                //printf("stat: %s\n", name);
                stat(name, &st);
		bufptr += sprintf(bufptr, "%u\x7f%lu\x7f%s\x9b", idx,
			st.st_size, entry->d_name);
                idx++;
        }
        closedir(dir);

        bufptr += sprintf(bufptr, "\x9bSelect: ");
	sio_send_byte('C');
	serial_tx(buf, LIST_BUF_SIZE);
	sio_send_byte(sio_crc(buf, LIST_BUF_SIZE));

	free(buf);
}

void sio_command_isr ()
{
	BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	xTaskNotifyFromISR(xSioHandle, 0, eSetValueWithoutOverwrite,
		&xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

FILE *disk_fd[4];

void sio_task ()
{
	unsigned long notify;
	unsigned short fileindex;
	unsigned char drive = 0;
	unsigned int sector_max;
	unsigned int sector_size;
	unsigned long offset;

	// insert the first image on flash, normally "boot.atr"
	disk_fd[drive] = sio_insert_disk(0, 0);

	for(;;) {
		xTaskNotifyWait(0, 0, &notify, portMAX_DELAY);
		switch (notify & 0x0000ffff) {
			case 0:
				//puts("got command");
				break;
			case 1:
				fileindex = (notify & 0xffff0000) >> 16;
				printf("change disk to index %u\n", fileindex);
				disk_fd[drive] = sio_insert_disk(disk_fd[drive],
					fileindex);
				continue;
			case 2:
				pokey_div++;
				printf("Pokey: %u\n", pokey_div);
				continue;
			case 3:
				pokey_div--;
				printf("Pokey: %u\n", pokey_div);
				continue;
			default:
				printf("ignoring notify %lx\n", notify);
				continue;
		}

		// process command
		serial_flush();	// drop previous data on the line
		bzero(&cmd_buf, sizeof(cmd_buf));
		if (sio_get_command() == FALSE) {
			printf("sio error: ");
			printf("%x %x %x %x\n", cmd_buf.dev, cmd_buf.cmd,
				cmd_buf.aux, cmd_buf.cksum);
			continue;
		}

		printf("%x %x %x %x\n", cmd_buf.dev, cmd_buf.cmd, cmd_buf.aux,
			cmd_buf.cksum);

		// wait command goes high(or low in inversed mode)
		unsigned int c = 0;
#ifdef CONFIG_SIO_CMD_INV
		while (gpio_get_level(CONFIG_SIO_CMD_PIN) != 0) {
#else
		while (gpio_get_level(CONFIG_SIO_CMD_PIN) != 1) {
#endif
			//vTaskDelay(1 / portTICK_PERIOD_MS);
			c++;
			if (c > 3000) {
				printf("cmd timeout\n");
				break;
			}
		}
		printf("c: %u\n", c);

		// only drive D1: actually supported
		if (cmd_buf.dev != 0x31) continue;
		//vTaskDelay(1 / portTICK_PERIOD_MS);	// T2

		if (sio_status.sector_size) sector_size = 0x100;
		else sector_size = 0x80;
		if (sio_status.medium_density) sector_max = 1040;
		else sector_max = 720;

		switch (cmd_buf.cmd) {
			case 'S':
				//printf("get status\n");
				sio_send_ack();
				sio_send_byte('C');
				serial_tx((char *) &sio_status,
					sizeof(sio_status));
				sio_send_byte(sio_crc((char *) &sio_status,
					sizeof(sio_status)));
				break;
			case 'R':
				//printf("read\n");
				sio_send_ack();
				if (cmd_buf.aux > sector_max) {
					sio_send_byte('E');
					goto bad_sector;
				}
				if (cmd_buf.aux <= 3) {
					sector_size = 0x80;
					offset = sector_size * (cmd_buf.aux - 1);
				}
				else
					offset = sector_size *
						(cmd_buf.aux - 4) + 128 * 3;
				if ( flags.ATR )
					offset += 16;
				fseek(disk_fd[drive], offset, SEEK_SET);
				fread(disk_buffer, sector_size, 1,
					disk_fd[drive]);
				sio_send_byte('C');
bad_sector:			serial_tx(disk_buffer, sector_size);
				sio_send_byte(sio_crc(disk_buffer,
					sector_size));
				break;
			case 'W':
				printf("write\n");
				sio_send_byte('N');
				break;
			case '?':
				sio_send_ack();
				sio_send_byte('C');
				sio_send_byte(pokey_div);
				//sio_send_byte(sio_crc((char *)&pokey_div, 1));
				//for only one byte, cksum is the same, so we
				//can skip the calc routine here
				sio_send_byte(pokey_div);
				break;
			case 'p':
				sio_send_ack();
				sio_get_files();
				break;
			case 'q':
				sio_send_ack();
				disk_fd[drive] = sio_insert_disk(disk_fd[drive],
					cmd_buf.aux);
				sio_send_byte('C');
				break;
			default:
				sio_send_byte('N');
		}
	}
}

void sio_init ()
{
	// port setup
	gpio_config_t config = {
		.pin_bit_mask = (1 << CONFIG_SIO_CMD_PIN),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
#ifdef CONFIG_SIO_CMD_INV
		.intr_type = GPIO_INTR_POSEDGE,
#else
		.intr_type = GPIO_INTR_NEGEDGE,
#endif
	};
	ESP_ERROR_CHECK(gpio_config(&config));

	// enable interrupt, when command pin goes low
	ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
	ESP_ERROR_CHECK(gpio_isr_handler_add(CONFIG_SIO_CMD_PIN,
		&sio_command_isr, NULL));
	ESP_ERROR_CHECK(gpio_intr_enable(CONFIG_SIO_CMD_PIN));

	serial_init(ATARI_SPEED_STD);

	// create sio task
	xTaskCreatePinnedToCore(&sio_task, "sio", 4096, NULL, 20,
		&xSioHandle, 0);

}
