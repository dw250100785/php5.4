/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2013 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author:  Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef SAPI_H
#define SAPI_H

#include "zend.h"
#include "zend_API.h"
#include "zend_llist.h"
#include "zend_operators.h"
#ifdef PHP_WIN32
#include "win95nt.h"
#endif
#include <sys/stat.h>

#define SAPI_OPTION_NO_CHDIR 1

#define SAPI_POST_BLOCK_SIZE 4000

#ifdef PHP_WIN32
#	ifdef SAPI_EXPORTS
#		define SAPI_API __declspec(dllexport) 
#	else
#		define SAPI_API __declspec(dllimport) 
#	endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define SAPI_API __attribute__ ((visibility("default")))
#else
#	define SAPI_API
#endif

#undef shutdown

typedef struct {
	char *header;
	uint header_len;
} sapi_header_struct;


typedef struct {
	zend_llist headers;
	int http_response_code;
	unsigned char send_default_content_type;
	char *mimetype;
	char *http_status_line;
} sapi_headers_struct;


typedef struct _sapi_post_entry sapi_post_entry;
typedef struct _sapi_module_struct sapi_module_struct;

BEGIN_EXTERN_C()
extern SAPI_API sapi_module_struct sapi_module;  /* true global */
END_EXTERN_C()

/* Some values in this structure needs to be filled in before
 * calling sapi_activate(). We WILL change the `char *' entries,
 * so make sure that you allocate a separate buffer for them
 * and that you free them after sapi_deactivate().
 * SAPI 请求信息结构体
 *    这个结构体中的某些值需要在调用sapi_activate()前被填充。
 *    我们会改变char *实体，因此要确保为它们分配单独的缓冲区,在sapi_deactivate()后会释放它们。
 */

typedef struct {
	const char *request_method;                           // 请求方法
	char *query_string;                                   // 查询字符串
	char *post_data, *raw_post_data;                      // post数据，原始post数据
	char *cookie_data;                                    // cookie数据
	long content_length;                                  // 内容长度
	uint post_data_length, raw_post_data_length;          // post数据长度， 原始post数据长度

	char *path_translated;                                // 解码后的路径
	char *request_uri;                                    // 请求uri

	const char *content_type;                             // 内容类型

	zend_bool headers_only;                               // 是否仅为头信息
	zend_bool no_headers;                                 // 是否不含头信息
	zend_bool headers_read;                               // 是否读取头信息

	sapi_post_entry *post_entry;                          // post实例

	char *content_type_dup;                               // 内容类型复制

	/* for HTTP authentication */
	char *auth_user;                                      // 认证用户
	char *auth_password;                                  // 认证密码
	char *auth_digest;                                    // 认证摘要

	/* this is necessary for the CGI SAPI module */
	char *argv0;                                          // 参数0， 对于CGI SAPI模块必须

	char *current_user;                                   // 当前用户
	int current_user_length;                              // 当前用户字符串长度

	/* this is necessary for CLI module */
	int argc;                                             // 参数数量 CLI模块必须
	char **argv;                                          // 参数数组 CLI模块必须
	int proto_num;                                        // 
} sapi_request_info;

/* SAPI全局变量结构体
 * 
 */
typedef struct _sapi_globals_struct {
	void *server_context;                                 // 服务器上下文
	sapi_request_info request_info;                       // 请求信息
	sapi_headers_struct sapi_headers;                     // 头信息
	int read_post_bytes;                                  // 读取post字节数
	unsigned char headers_sent;                           // 头信息发送
	struct stat global_stat;                              // 全局状态
	char *default_mimetype;                               // 默认MIME TYPE
	char *default_charset;                                // 默认字符集
	HashTable *rfc1867_uploaded_files;                    // 
	long post_max_size;                                   // post最大尺寸
	int options;                                          // 选项
	zend_bool sapi_started;                               // SAPI是否启动了
	double global_request_time;                           // 全局的请求时间
	HashTable known_post_content_types;                   // 已知post内容类型
	zval *callback_func;                                  // 回调函数
	zend_fcall_info_cache fci_cache;                      // 
	zend_bool callback_run;                               // 回调是否运行
} sapi_globals_struct;


BEGIN_EXTERN_C()
#ifdef ZTS   // SG(v) SAPI Globals
# define SG(v) TSRMG(sapi_globals_id, sapi_globals_struct *, v)
SAPI_API extern int sapi_globals_id;
#else
# define SG(v) (sapi_globals.v)
extern SAPI_API sapi_globals_struct sapi_globals;
#endif

SAPI_API void sapi_startup(sapi_module_struct *sf);
SAPI_API void sapi_shutdown(void);
SAPI_API void sapi_activate(TSRMLS_D);
SAPI_API void sapi_deactivate(TSRMLS_D);
SAPI_API void sapi_initialize_empty_request(TSRMLS_D);
END_EXTERN_C()

/*
 * This is the preferred and maintained API for 
 * operating on HTTP headers.
 */

/*
 * Always specify a sapi_header_line this way:
 *
 *     sapi_header_line ctr = {0};
 */
 
typedef struct {
	char *line; /* If you allocated this, you need to free it yourself */
	uint line_len;
	long response_code; /* long due to zend_parse_parameters compatibility */
} sapi_header_line;

typedef enum {					/* Parameter: 			*/
	SAPI_HEADER_REPLACE,		/* sapi_header_line* 	*/
	SAPI_HEADER_ADD,			/* sapi_header_line* 	*/
	SAPI_HEADER_DELETE,			/* sapi_header_line* 	*/
	SAPI_HEADER_DELETE_ALL,		/* void					*/
	SAPI_HEADER_SET_STATUS		/* int 					*/
} sapi_header_op_enum;

BEGIN_EXTERN_C()
SAPI_API int sapi_header_op(sapi_header_op_enum op, void *arg TSRMLS_DC);

/* Deprecated functions. Use sapi_header_op instead. */
SAPI_API int sapi_add_header_ex(char *header_line, uint header_line_len, zend_bool duplicate, zend_bool replace TSRMLS_DC);
#define sapi_add_header(a, b, c) sapi_add_header_ex((a),(b),(c),1 TSRMLS_CC)


SAPI_API int sapi_send_headers(TSRMLS_D);
SAPI_API void sapi_free_header(sapi_header_struct *sapi_header);
SAPI_API void sapi_handle_post(void *arg TSRMLS_DC);

SAPI_API int sapi_register_post_entries(sapi_post_entry *post_entry TSRMLS_DC);
SAPI_API int sapi_register_post_entry(sapi_post_entry *post_entry TSRMLS_DC);
SAPI_API void sapi_unregister_post_entry(sapi_post_entry *post_entry TSRMLS_DC);
SAPI_API int sapi_register_default_post_reader(void (*default_post_reader)(TSRMLS_D) TSRMLS_DC);
SAPI_API int sapi_register_treat_data(void (*treat_data)(int arg, char *str, zval *destArray TSRMLS_DC) TSRMLS_DC);
SAPI_API int sapi_register_input_filter(unsigned int (*input_filter)(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC), unsigned int (*input_filter_init)(TSRMLS_D) TSRMLS_DC);

SAPI_API int sapi_flush(TSRMLS_D);
SAPI_API struct stat *sapi_get_stat(TSRMLS_D);
SAPI_API char *sapi_getenv(char *name, size_t name_len TSRMLS_DC);

SAPI_API char *sapi_get_default_content_type(TSRMLS_D);
SAPI_API void sapi_get_default_content_type_header(sapi_header_struct *default_header TSRMLS_DC);
SAPI_API size_t sapi_apply_default_charset(char **mimetype, size_t len TSRMLS_DC);
SAPI_API void sapi_activate_headers_only(TSRMLS_D);

SAPI_API int sapi_get_fd(int *fd TSRMLS_DC);
SAPI_API int sapi_force_http_10(TSRMLS_D);

SAPI_API int sapi_get_target_uid(uid_t * TSRMLS_DC);
SAPI_API int sapi_get_target_gid(gid_t * TSRMLS_DC);
SAPI_API double sapi_get_request_time(TSRMLS_D);
SAPI_API void sapi_terminate_process(TSRMLS_D);
END_EXTERN_C()

struct _sapi_module_struct {
	char *name;                                                          // 简单名称
	char *pretty_name;                                                   // 详细名称

	int (*startup)(struct _sapi_module_struct *sapi_module);             // 启动方法 函数指针
	int (*shutdown)(struct _sapi_module_struct *sapi_module);            // 关闭方法 函数指针

	int (*activate)(TSRMLS_D);                                           // 激活方法
	int (*deactivate)(TSRMLS_D);                                         // 关闭方法

	int (*ub_write)(const char *str, unsigned int str_length TSRMLS_DC); // 
	void (*flush)(void *server_context);                                 // 刷新方法
	struct stat *(*get_stat)(TSRMLS_D);                                  // 获取状态方法
	char *(*getenv)(char *name, size_t name_len TSRMLS_DC);              // 获取环境变量方法

	void (*sapi_error)(int type, const char *error_msg, ...);            // sapi错误处理方法

	int (*header_handler)(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers TSRMLS_DC);   // header处理器
	int (*send_headers)(sapi_headers_struct *sapi_headers TSRMLS_DC);    // 发送大量headers方法
	void (*send_header)(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC);    // 发送单个header方法

	int (*read_post)(char *buffer, uint count_bytes TSRMLS_DC);          // 读取post方法
	char *(*read_cookies)(TSRMLS_D);                                     // 读取cookie方法

	void (*register_server_variables)(zval *track_vars_array TSRMLS_DC); // 注册服务器变量方法
	void (*log_message)(char *message TSRMLS_DC);                        // 记录日志信息方法
	double (*get_request_time)(TSRMLS_D);                                // 获取请求时间方法
	void (*terminate_process)(TSRMLS_D);                                 // 终止进程方法

	char *php_ini_path_override;                                         // php.ini覆盖路径

	void (*block_interruptions)(void);                                   // 阻止中断方法
	void (*unblock_interruptions)(void);                                 // 疏通中断方法

	void (*default_post_reader)(TSRMLS_D);                               // 默认post读取方法
	void (*treat_data)(int arg, char *str, zval *destArray TSRMLS_DC);   // 处理数据方法
	char *executable_location;                                           // 可执行路径

	int php_ini_ignore;                                                  // php.ini是否忽略
	int php_ini_ignore_cwd; /* don't look for php.ini in the current directory */  // 当前指令忽略php.ini

	int (*get_fd)(int *fd TSRMLS_DC);                                    // 获取文件描述符

	int (*force_http_10)(TSRMLS_D);                                      // 

	int (*get_target_uid)(uid_t * TSRMLS_DC);                            // 获取目标对象的用户ID
	int (*get_target_gid)(gid_t * TSRMLS_DC);                            // 获取目标对象的用户组ID

	unsigned int (*input_filter)(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC);  // 输入过滤器
	
	void (*ini_defaults)(HashTable *configuration_hash);                 // ini默认值设置方法
	int phpinfo_as_text;                                                 // phpinfo是否作为文本

	char *ini_entries;                                                   // ini实体数组
	const zend_function_entry *additional_functions;                     // 附加函数变量
	unsigned int (*input_filter_init)(TSRMLS_D);                         // 输入过滤器初始化方法
};


struct _sapi_post_entry {
	char *content_type;                                                  // 内容类型
	uint content_type_len;                                               // 内容类型字符串的长度
	void (*post_reader)(TSRMLS_D);                                       // post读取器
	void (*post_handler)(char *content_type_dup, void *arg TSRMLS_DC);   // post处理器
};

/* header_handler() constants */
#define SAPI_HEADER_ADD			(1<<0)


#define SAPI_HEADER_SENT_SUCCESSFULLY	1
#define SAPI_HEADER_DO_SEND				2
#define SAPI_HEADER_SEND_FAILED			3

#define SAPI_DEFAULT_MIMETYPE		"text/html"
#define SAPI_DEFAULT_CHARSET		""
#define SAPI_PHP_VERSION_HEADER		"X-Powered-By: PHP/" PHP_VERSION

#define SAPI_POST_READER_FUNC(post_reader) void post_reader(TSRMLS_D)
#define SAPI_POST_HANDLER_FUNC(post_handler) void post_handler(char *content_type_dup, void *arg TSRMLS_DC)

#define SAPI_TREAT_DATA_FUNC(treat_data) void treat_data(int arg, char *str, zval* destArray TSRMLS_DC)
#define SAPI_INPUT_FILTER_FUNC(input_filter) unsigned int input_filter(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC)

BEGIN_EXTERN_C()
SAPI_API SAPI_POST_READER_FUNC(sapi_read_standard_form_data);
SAPI_API SAPI_POST_READER_FUNC(php_default_post_reader);
SAPI_API SAPI_TREAT_DATA_FUNC(php_default_treat_data);
SAPI_API SAPI_INPUT_FILTER_FUNC(php_default_input_filter);
END_EXTERN_C()

#define STANDARD_SAPI_MODULE_PROPERTIES

#endif /* SAPI_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
