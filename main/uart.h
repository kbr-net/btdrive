void serial_init(unsigned int speed);
void serial_tx (char *data, unsigned int len);
unsigned int serial_rx (char *data, unsigned int len);
void serial_flush();
void serial_change_baud(unsigned int baud);
