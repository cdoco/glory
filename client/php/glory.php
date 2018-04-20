<?php

$br = (php_sapi_name() == "cli") ? "" : "<br />";

if(!extension_loaded('glory')) {
	dl('glory.' . PHP_SHLIB_SUFFIX);
}

$param = [
	'/user/info' => [
		'user_name' => 'libaisan',
		'ext_fields' => 'verification',
		'level' => '1',
		'ver' => '1.0',
		'only' => 'name',
	],
   '/smart/advisor/get_read_category' => [
	   'user_name' => 'libaisan'
   ],
   '/smart/advisor/get_read_category' => [
	   'user_name' => 'libaisan'
   ],
   '/smart/advisor/get_read_category' => [
	   'user_name' => 'libaisan'
   ],
   '/smart/advisor/get_read_category' => [
	   'user_name' => 'libaisan'
   ]
];

echo glory("data", $param, 100) . "\n";
