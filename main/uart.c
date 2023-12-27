#include <driver/uart.h>
#include <driver/gpio.h>

const uart_port_t uart_num = CONFIG_SIO_UART_PORT_NUM;

void serial_init (unsigned int speed) {
	uart_config_t uart_config = {
		.baud_rate = speed,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};

	// uninstall if allready installed
	if (uart_is_driver_installed(uart_num))
		uart_driver_delete(uart_num);

	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	// Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
	//ESP_ERROR_CHECK(uart_set_pin(uart_num, 4, 5, 18, 19));
	ESP_ERROR_CHECK(uart_set_pin(uart_num, CONFIG_SIO_UART_TXD,
		CONFIG_SIO_UART_RXD, -1, -1));

#if defined(CONFIG_SIO_UART_TXD_INV) && defined(CONFIG_SIO_UART_RXD_INV)
	// Invert Pins(for simple level shifter)
	ESP_ERROR_CHECK(uart_set_line_inverse(uart_num,
		(UART_SIGNAL_TXD_INV | UART_SIGNAL_RXD_INV)));
	ESP_ERROR_CHECK(gpio_pullup_en(CONFIG_SIO_UART_RXD));
#elif defined CONFIG_SIO_UART_TXD_INV
	// Invert Pins(for simple level shifter)
	ESP_ERROR_CHECK(uart_set_line_inverse(uart_num, UART_SIGNAL_TXD_INV));
#elif defined CONFIG_SIO_UART_RXD_INV
	// Invert Pins(for simple level shifter)
	ESP_ERROR_CHECK(uart_set_line_inverse(uart_num, UART_SIGNAL_RXD_INV));
	ESP_ERROR_CHECK(gpio_pullup_en(CONFIG_SIO_UART_RXD));
#endif
	// Setup UART buffered IO with event queue
	const int uart_buffer_size = (256);
/*
	QueueHandle_t uart_queue;
	// Install UART driver using an event queue here
	ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size,
		uart_buffer_size, 5, &uart_queue, 0));
*/
	ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size * 2,
					0, 0, NULL, 0));

	// tuning
	ESP_ERROR_CHECK(uart_set_tx_idle_num(uart_num, 15));	//default 10
	//ESP_ERROR_CHECK(uart_set_tx_empty_threshold(uart_num, 10));
	ESP_ERROR_CHECK(uart_set_rx_full_threshold(uart_num, 5));
	ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, 3));
}

void serial_tx (char *data, unsigned int len) {
	uart_write_bytes(uart_num, data, len);
	uart_wait_tx_done(uart_num, 50 / portTICK_PERIOD_MS);
}

unsigned int serial_rx (char *data, unsigned int len) {
	int r = uart_read_bytes(uart_num, data, len, 50 / portTICK_PERIOD_MS);
	//printf("rx: %i\n", r);
	return(r);
}

void serial_flush()
{
	uart_flush(uart_num);
}

void serial_change_baud(unsigned int baud)
{
	ESP_ERROR_CHECK(uart_set_baudrate(uart_num, baud));
}
