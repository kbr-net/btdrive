#include <string.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs_fat.h"

static const char *PART = "storage";

extern TaskHandle_t xSioHandle;

void tasklist ()
{
	char *tasklistbuf = malloc(512);
	if (! tasklistbuf) {
		perror("malloc");
		return;
	}

	vTaskList(tasklistbuf);
	printf("Name\t\tStat\tPrio\tStack\tPid\tCPU\n%s", tasklistbuf);
	free(tasklistbuf);
}

void fs_info ()
{
/*
	size_t total = 0, used = 0;
	esp_err_t ret = esp_spiffs_info(PART, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE("spiffs",
			"Failed to get SPIFFS partition information (%s)",
			esp_err_to_name(ret));
	} else {
		printf("Partition size: total: %d, used: %d\n", total, used);
	}
*/
	unsigned long long total, free;
	ESP_ERROR_CHECK(esp_vfs_fat_info("/a", &total, &free));
	printf("Partition total: %llu, free: %llu\n", total, free);

	DIR *dir = opendir("/a/");
	struct dirent *entry;
	struct stat st;
	char name[256];
	unsigned int idx = 0;
	puts("Idx\tSize\tFilename");
	while((entry = readdir(dir))) {
		strcpy(name, "/a/");
		strcat(name, entry->d_name);
		//printf("stat: %s\n", name);
		stat(name, &st);
		printf("%u\t%lu\t%s\n", idx, st.st_size, entry->d_name);
		idx++;
	}
	closedir(dir);
}

void do_command(char *buf)
{
	char c;
	unsigned long v = 0;
	sscanf(buf, "%c%lu", &c, &v);
	//c |= 0b00100000;	// to downcase
	switch (c) {
		case 'n':
			v <<= 16;
			xTaskNotify(xSioHandle, v|1, eSetValueWithoutOverwrite);
			break;
		case 'f':
			puts("formating data partition...");
			//ESP_ERROR_CHECK(esp_spiffs_format(PART));
			ESP_ERROR_CHECK(esp_vfs_fat_spiflash_format_rw_wl("/a",
				PART));
			puts("done");
			break;
		case 't':
			tasklist();
			break;
		case 'l':
			fs_info();
			break;
		case 'u':
			xTaskNotify(xSioHandle, v|2, eSetValueWithoutOverwrite);
			break;
		case 'i':
			xTaskNotify(xSioHandle, v|3, eSetValueWithoutOverwrite);
			//esp_intr_dump(NULL);
	}
}
