/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "i2c-lcd.h"

#define PORT                       	CONFIG_EXAMPLE_PORT 
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT

static const char *TAG = "Remote-LCD";

void str2hex(char *str, int len) { // turns given string into a hex sequence 
	const char hexdig[] = "0123456789ABCDEF"; // the hexidecimal digits
	char str2[len * 3]; // one character needs 3 bytes: two hex digits then a space
	for (int i = 0; i < len*3; i += 3) {
		str2[i] = hexdig[ (*(char*)(str+i) % 0x10) ]; // just a simple little thing 
		str2[i+1] = hexdig[ (*(char*)(str+i) / 0x10) ];
		str2[i+2] = ' ';
	}
	str2[len-1] = 0; // null-terminate it
	ESP_LOGI(TAG, "String to text: %s", str2);
	return;
}

int write_cmd(const int sock) {
	int wd, rd; // the write & read returns
	char buff[8]; // the buffer to which I'll record the command
	// I hardcoded the lengths of these buffers 
	if ((wd = send(sock, "Enter command hex code (example: \"0x01\"): ", 42, 0)) < 0) {
		ESP_LOGE(TAG, "Error at command prompt, errno %d", errno); // return the error number if it fails
	}
	rd = recv(sock, buff, 8, 0); // the command input
	if (rd < 0) { // standard error checking for inputs, should be a way to automate it tbh
		ESP_LOGE(TAG, "Error occured during getting command, errno: %d", errno);
		return -1;
	} else if (rd == 0) { 
		ESP_LOGE(TAG, "Connection terminated before getting command, errno %d", errno);
		return 1;
	}
	buff[rd] = 0; // null terminate the buffer

	// I am assuming that someone sends the input in the format 0xNN and that I cab just directky mess with the two Ns
	int cmd = 0; // actually turn the command into a sendable hex number
	cmd += (buff[2] - '0') << 4; // upper digit
	cmd += (buff[3] - '0'); // lower digit

	lcd_send_cmd(cmd); // sends it to the LCD 
	return 0;
}

int write_data(const int sock) {
	int wd, rd; // write and read returns 
	char buff[32]; // the answer buffer

	if ((wd = send(sock, "Enter max 32 characters of data to write (example: \"hello\"): ", 61, 0)) < 0)
		ESP_LOGE(TAG, "Error at data prompt, errno %d", errno);
	
	rd = recv(sock, buff, 32, 0); // gets string to send 
	if (rd < 0) { 
		ESP_LOGE(TAG, "Error occured during getting data, errno: %d", errno);
		return -1;
	}
	else if (rd == 0) { 
		ESP_LOGE(TAG, "Connection terminated before getting data, errno %d", errno);
		return 1;
	}
	buff[rd] = 0;

	if (rd > 16) { // send it in two parts because the LCD is 2x16 
		lcd_send_string(buff);
		lcd_send_string((buff+16));
	} else 
		lcd_send_string(buff);

	return 0;
}

static void do_retransmit(const int sock) // meat & potatoes of the whole program
{
	enum qState_t { // a weird enum structure for the question, no real use tbh
		Asked=1,
		Answered=2,
		Invalid=3,
		Done=4,
	};
	
	enum qState_t QS;

	const char *question = "Do you want to send a command (Input: \"cmd\") or write data (Input \"write\")?: \n";
	// I made the buffer small specifically assuming that people will send "cmd" or "write" only
	int len; // read return
	char ans_buff[8]; 

    do {
		int wd = send(sock, question, 78, 0); // send return
		if (wd < 0) {
			ESP_LOGE(TAG, "Error occured during asking: errno %d", errno);
			return;
		}
		ESP_LOGI(TAG, "Question Asked");
		QS = Asked;

		while (QS != Answered) { 
			len = recv(sock, ans_buff, 8, 0);
			if (len < 0) 
				ESP_LOGE(TAG, "Error occurred during first input: errno %d", errno);
			else if (len == 0) 
				ESP_LOGE(TAG, "Connection closed before answer: errno %d", errno);
			else 
				ans_buff[len-1] = 0;

			if (!(strcmp("cmd", ans_buff) || !(strcmp("cmd\n", ans_buff)))) { 
				// When using netcat, the newline gets transmitted as well but I'm not sure if the receive of the function works the same
				if (write_cmd(sock) != 0) 
					return;
				QS = Answered;
			}
			else if (!(strcmp("write", ans_buff)) || !(strcmp("write\n", ans_buff))) {
				// same deal
				if (write_data(sock) != 0) 
					return;
				QS = Answered;
			}
			else 
				QS = Invalid;

			memset(ans_buff, 0, 8); // set the answer buffer once more till I get a valid answer
		}

    } while (1); // infinite loop till connection terminates
}

static void tcp_server_task(void *pvParameters) { // this is code taken from the esp-idf socket server exanple
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

#ifdef CONFIG_EXAMPLE_IPV4
    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    if (addr_family == AF_INET6) {
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#if defined(CONFIG_EXAMPLE_IPV4) && defined(CONFIG_EXAMPLE_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
#ifdef CONFIG_EXAMPLE_IPV4
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
#ifdef CONFIG_EXAMPLE_IPV6
        if (source_addr.ss_family == PF_INET6) {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
	
	lcd_init(); // init the LCD to be used
	lcd_clear(); // clear the screen


	// actually start the socket 
#ifdef CONFIG_EXAMPLE_IPV4
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
#endif
#ifdef CONFIG_EXAMPLE_IPV6
    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET6, 5, NULL);
#endif
}
