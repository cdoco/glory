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

#ifndef PHP_GLORY_H
#define PHP_GLORY_H

extern zend_module_entry glory_module_entry;
#define phpext_glory_ptr &glory_module_entry

#define PHP_GLORY_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_GLORY_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_GLORY_API __attribute__ ((visibility("default")))
#else
#	define PHP_GLORY_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define GLORY_G(v) TSRMG(glory_globals_id, zend_glory_globals *, v)
#else
#define GLORY_G(v) (glory_globals.v)
#endif

ZEND_BEGIN_MODULE_GLOBALS(glory)
	char *protocol;
  char *ip;
  size_t port;
ZEND_END_MODULE_GLOBALS(glory)

#if defined(ZTS) && defined(COMPILE_DL_GLORY)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_GLORY_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
