#include "bt_main.h"
#include "sio.h"
#include "nvs_flash.h"
//#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "console.h"

static const char *PART = "storage";
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

void app_main(void)
{
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
/*
	esp_vfs_spiffs_conf_t spiffs_conf = {
		.base_path = "/a",
		.partition_label = PART,
		.max_files = 5,
		.format_if_mount_failed = true
	};
	ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
*/
	const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 5,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
	};
	esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl("/a", PART,
		&mount_config, &s_wl_handle);
	if (err != ESP_OK) {
		ESP_LOGE("wl", "Failed to mount FATFS (%s)",
			esp_err_to_name(err));
		return;
	}

	init_bt();
	sio_init();
	console_init();

	printf("READY\n");

}
