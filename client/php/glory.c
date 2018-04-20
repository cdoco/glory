/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: ZiHang Gao <gaozihang@zhangyue.com>                          |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_glory.h"
#include "zend_smart_str.h"
#include "ext/standard/info.h"

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

ZEND_DECLARE_MODULE_GLOBALS(glory)

static int le_glory, sockfd;

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("glory.protocol",      "tcp", PHP_INI_ALL, OnUpdateString, protocol, zend_glory_globals, glory_globals)
    STD_PHP_INI_ENTRY("glory.ip", "0.0.0.0", PHP_INI_ALL, OnUpdateString, ip, zend_glory_globals, glory_globals)
	STD_PHP_INI_ENTRY("glory.port", "9999", PHP_INI_ALL, OnUpdateLong, port, zend_glory_globals, glory_globals)
PHP_INI_END()

PHP_GINIT_FUNCTION(glory){
	glory_globals->protocol = "tcp";
	glory_globals->ip = "0.0.0.0";
	glory_globals->port = 9999;
}

//create socket
static int create_socket() {
	struct sockaddr_in addr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    	php_error(E_ERROR, "create socket error");
		return FAILURE;
    }

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
    addr.sin_port = htons(GLORY_G(port));
    addr.sin_addr.s_addr = inet_addr(GLORY_G(ip));

    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        php_error(E_ERROR, "connect socket error");
		return FAILURE;
    }

	return SUCCESS;
}

PHP_FUNCTION(glory)
{
	zend_string *buf;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &buf) == FAILURE) {
		return;
	}

	ssize_t send_len = send(sockfd, ZSTR_VAL(buf), ZSTR_LEN(buf), 0);
	
	//failure in link
	if (send_len == FAILURE) {
		close(sockfd);
		//to create links
		if (create_socket() == FAILURE) {
			return;
		}
		//to send data
		send_len = send(sockfd, ZSTR_VAL(buf), ZSTR_LEN(buf), 0);
	}

	//receive data
	if (ZSTR_LEN(buf) == send_len) {
		//recv_total
		uint32_t recv_total;
		recv(sockfd, &recv_total, 4, 0);

		//recv_data
		char recv_data[ntohl(recv_total)];
		recv(sockfd, recv_data, ntohl(recv_total), 0);

		RETURN_STRING(recv_data);
	}

	RETURN_FALSE;
}

PHP_MINIT_FUNCTION(glory)
{
	REGISTER_INI_ENTRIES();

	//create socket
	return create_socket();
}

PHP_MSHUTDOWN_FUNCTION(glory)
{
	//close socket
	close(sockfd);
	return SUCCESS;
}

PHP_MINFO_FUNCTION(glory)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "glory support", "enabled");
	php_info_print_table_row(2, "Version", PHP_GLORY_VERSION);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

const zend_function_entry glory_functions[] = {
	PHP_FE(glory,	NULL)
	PHP_FE_END
};

zend_module_entry glory_module_entry = {
	STANDARD_MODULE_HEADER,
	"glory",
	glory_functions,
	PHP_MINIT(glory),
	PHP_MSHUTDOWN(glory),
	NULL,
	NULL,
	PHP_MINFO(glory),
	PHP_GLORY_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_GLORY
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(glory)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
