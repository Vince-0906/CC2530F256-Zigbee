#ifndef DHT11_H
#define DHT11_H

void DHT11_Init(void);

/*
 * Read temperature and humidity from DHT11.
 * temp: pointer to store temperature (integer part, 0-50 C)
 * humi: pointer to store humidity (integer part, 20-90 %)
 * Returns: 0 = success, 1 = no response, 2 = checksum error
 */
unsigned char DHT11_Read(unsigned char *temp, unsigned char *humi);

#endif
