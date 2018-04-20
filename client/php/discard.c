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

PHP_FUNCTION(glory)
{
	zval *args = NULL, ja;
	smart_str json = {0};
	size_t i, total_len, send_len, argc = ZEND_NUM_ARGS(), offset = 0, param_total = 0;
	char name_bytes[4], total_bytes[4], param_bytes[4], param_type[4], param_len[4];

	if (zend_parse_parameters(argc, "+", &args, &argc) == FAILURE) {
		return;
	}

	//the first parameter is the name
	zval *name = &(args[0]);

	//name bytes
    ((int *)name_bytes)[0] = htonl(Z_STRLEN_P(name));

	//total length
	total_len = Z_STRLEN_P(name) + 4;

	array_init(&ja);

	//come up with the rest of the parameters
	for (i = 1; i < argc; i++) {
		//param zval
		zval *param = &(args[i]);

		switch(Z_TYPE_P(param)) {
			case IS_LONG:
				param_total += 8;
				break;
			case IS_STRING:
				param_total += 8 + Z_STRLEN_P(param);
				break;
			case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
			case IS_CONSTANT_ARRAY:
#endif
				php_json_encode(&json, param);

				//insert a global array
				add_index_stringl(&ja, i, ZSTR_VAL(json.s), ZSTR_LEN(json.s));
				param_total += 8 + ZSTR_LEN(json.s);
				smart_str_free(&json);
				break;
			default:
            	php_error(E_ERROR, "glory wrong zval type");
            	break;
		}
	}

	char param_data[param_total];

	for (i = 1; i < argc; i++) {
		//param zval
		zval *param = &(args[i]), *pa;
		char *tmp;

		switch(Z_TYPE_P(param)) {
			case IS_LONG:
				((int *)param_type)[0] = htonl(1);
				((int *)param_len)[0] = htonl(Z_LVAL_P(param));

				tmp = emalloc(8);
				memcpy(tmp, param_type, 4);
				memcpy(tmp + 4, param_len, 4);

				//copy tmp
				memcpy(param_data + offset, tmp, 8);
				offset += 8;
				efree(tmp);
				break;
			case IS_STRING:

				((int *)param_type)[0] = htonl(2);
				((int *)param_len)[0] = htonl(Z_STRLEN_P(param));

				tmp = emalloc(8 + Z_STRLEN_P(param));
				memcpy(tmp, param_type, 4);
				memcpy(tmp + 4, param_len, 4);
				memcpy(tmp + 4 + 4, Z_STRVAL_P(param), Z_STRLEN_P(param));

				//copy tmp
				memcpy(param_data + offset, tmp, 8 + Z_STRLEN_P(param));
				offset += 8 + Z_STRLEN_P(param);
				efree(tmp);
				break;
			case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
			case IS_CONSTANT_ARRAY:
#endif
				pa = zend_hash_index_find(Z_ARRVAL(ja), i);

				((int *)param_type)[0] = htonl(3);
				((int *)param_len)[0] = htonl(Z_STRLEN_P(pa));

				tmp = emalloc(8 + Z_STRLEN_P(pa));
				memcpy(tmp, param_type, 4);
				memcpy(tmp + 4, param_len, 4);
				memcpy(tmp + 4 + 4, Z_STRVAL_P(pa), Z_STRLEN_P(pa));

				//copy tmp
				memcpy(param_data + offset, tmp, 8 + Z_STRLEN_P(pa));
				offset += 8 + Z_STRLEN_P(pa);
				efree(tmp);
				break;
			default:
            	php_error(E_ERROR, "glory wrong zval type");
            	break;
		}
    }

	zval_ptr_dtor(&ja);

	if (offset) {
		((int *)param_bytes)[0] = htonl(sizeof(param_data));
		total_len += sizeof(param_data) + 4;
	}

	//total bytes
	((int *)total_bytes)[0] = htonl(total_len);

	//send data
	char send_data[4 + total_len];

    memcpy(send_data, total_bytes, 4);
	memcpy(send_data + 4, name_bytes, 4);
	memcpy(send_data + 4 + 4, Z_STRVAL_P(name), Z_STRLEN_P(name));

	if (offset) {
		memcpy(send_data + 4 + 4 + Z_STRLEN_P(name), param_bytes, 4);
		memcpy(send_data + 4 + 4 + Z_STRLEN_P(name) + 4, param_data, sizeof(param_data));
	}

	send_len = send(sockfd, send_data, sizeof(send_data), 0);

	//receive data
	if (sizeof(send_data) == send_len) {
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