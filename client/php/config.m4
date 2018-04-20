dnl $Id$
dnl config.m4 for extension glory

PHP_ARG_ENABLE(glory, whether to enable glory support,
[  --enable-glory           Enable glory support])

if test "$PHP_GLORY" != "no"; then
  PHP_NEW_EXTENSION(glory, glory.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
