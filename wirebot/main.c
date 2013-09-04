/* $Id$ */

/*
 *  Copyright (c) 2004-2011 Axel Andersson
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <wired/wired.h>

#include "chats.h"
#include "client.h"
#include "commands.h"
#include "ignores.h"
#include "main.h"
#include "messages.h"
#include "spec.h"
#include "terminal.h"
#include "topic.h"
#include "users.h"
#include "windows.h"
#include "files.h"
#include "transfers.h"
#include "settings.h"

static void							wr_cleanup(void);
static void							wr_usage(void);
static void							wr_version(void);

static void							wb_write_pid(void);
static void							wb_delete_pid(void);

static void							wr_signals_init(void);
static void							wd_block_signals(void);
static int							wd_wait_signals(void);
static void							wd_signal_thread(wi_runtime_instance_t *);
static void							wr_sig_pipe(int);
static void							wr_sig_int(int);
static void							wr_sig_crash(int);

static void							wr_log_callback(wi_log_level_t, wi_string_t *);

static wi_integer_t					wr_runloop(wi_array_t *, wi_time_interval_t);
static wi_boolean_t					wr_runloop_stdin_callback(wi_socket_t *);


static wi_mutable_array_t			*wr_runloop_sockets;

volatile sig_atomic_t				wr_running = 1;

wi_boolean_t						wr_debug;
wi_date_t							*wr_start_date;
wi_string_t							*wr_config_path;

wi_string_t							*wb_hostname;
wi_string_t							*wb_login;
wi_string_t							*wb_password;
wi_integer_t						wb_port;

wb_bot_t							*wb_bot;


int main(int argc, const char **argv) {
	wi_pool_t				*pool;
	wi_mutable_array_t		*arguments;
	wi_string_t				*homepath, *wirepath, *path, *component;
	wi_file_t				*file;	
	wi_boolean_t			daemonize;
	int						ch;

	wi_string_t *hostname, *login, *password;
	wi_uinteger_t port;

	wi_initialize();
	wi_load(argc, argv);
	
	pool = wi_pool_init(wi_pool_alloc());
	
	wi_log_syslog			= true;
	wi_log_syslog_facility	= LOG_DAEMON;

	wr_start_date 			= wi_date_init(wi_date_alloc());
	arguments				= wi_array_init(wi_mutable_array_alloc());

	daemonize				= true;

	wr_spec_init();

	while((ch = getopt(argc, (char * const *) argv, "Ddc:hVvXx")) != -1) {

		switch(ch) {
			case 'D':
				daemonize = false;
				wi_log_stderr = true;
				break;

			case 'd':
				wr_debug = true;

				wi_log_level = WI_LOG_DEBUG;
				wi_log_file = true;
				wi_log_path = WI_STR("wire.out");
				wi_log_callback = NULL;
				break;

			case 'V':
			case 'v':
				wr_version();
				break;

			case 'X':
			case 'x':
				daemonize = false;
				break;

			case '?':
			case 'h':
			default:
				wr_usage();
				break;
		}

		wi_mutable_array_add_data(arguments, wi_string_with_format(WI_STR("-%c"), ch));
		
		if(optarg)
			wi_mutable_array_add_data(arguments, wi_string_with_cstring(optarg));
	}

	// argc -= optind;
	// argv += optind;

	if(daemonize) {
		wi_mutable_array_add_data(arguments, WI_STR("-X"));
		
		switch(wi_fork()) {
			case -1:
				wi_log_fatal(WI_STR("Could not fork: %m"));
				break;
				
			case 0:
				if(!wi_execv(wi_string_with_cstring(argv[0]), arguments))
					wi_log_fatal(WI_STR("Could not execute %s: %m"), argv[0]);
				break;
				
			default:
				_exit(0);
				break;
		}
	}

	wi_release(arguments);

	wi_log_open();
	
	homepath = wi_user_home();
	wirepath = wi_string_by_appending_path_component(homepath, WI_STR(WB_WIREBOT_USER_PATH));
	wi_fs_create_directory(wirepath, 0700);

	wr_config_path = wi_retain(wi_string_by_appending_path_component(homepath, WI_STR(WB_WIREBOT_CONFIG_PATH)));

	// init config
	wd_settings_initialize();
	if(!wd_settings_read_config()) {
		exit(1);
	}

	// set icon path from confog
	wr_icon_path = wi_retain(wi_string_by_appending_path_component(wirepath, wi_config_path_for_name(wd_config, WI_STR("icon path"))));

	wb_bot_initialize();
	wb_outputs_init();
	wb_inputs_init();
	wb_services_init();
	wb_watchers_init();
	wb_rules_init();
	wb_commands_init();

	wr_readline_init();
	wr_chats_init();
	wr_commands_initialize();

	wr_client_init();
	wr_messages_init();
	wr_runloop_init();
	wr_users_init();

	wb_bot_init();

	wr_topics_init();
	wr_servers_init();

	wb_write_pid();

	wr_signals_init();
	wd_block_signals();

	// connect
	wb_hostname 	= wi_retain(wi_config_string_for_name(wd_config, WI_STR("hostname")));
	wb_login		= wi_retain(wi_config_string_for_name(wd_config, WI_STR("login")));
	wb_password		= wi_retain(wi_config_string_for_name(wd_config, WI_STR("password")));
	wb_port			= wi_config_port_for_name(wd_config, WI_STR("port"));

	wr_client_connect(wb_hostname, wb_port, wb_login, wb_password);	
	wi_pool_drain(pool);

	wi_thread_create_thread(wd_signal_thread, NULL);
	wr_runloop_run();

	wr_cleanup();
	wi_release(pool);
	
	return 0;
}



static void wr_cleanup(void) {
	wb_delete_pid();

	wi_release(wb_hostname);
	wi_release(wb_login);
	wi_release(wb_password);
}



static void wr_usage(void) {
	fprintf(stderr,
"Usage: wirebot [-ucDdhv]\n\
\n\
Options:\n\
    -u             wired URL for server to connect\n\
    -c             wiredbot config file\n\
    -D             do not daemonize\n\
    -d             enable debug mode\n\
    -h             display this message\n\
    -v             display version information\n\
\n\
By Rafael Warnault <%s>\n", WR_BUGREPORT);

	exit(2);
}



static void wr_version(void) {
	fprintf(stderr, "Wirebot %s (%s), protocol %s %s\n",
		WR_VERSION,
		WI_REVISION,
		wi_string_cstring(wi_p7_spec_name(wr_p7_spec)),
		wi_string_cstring(wi_p7_spec_version(wr_p7_spec)));

	exit(2);
}



#pragma mark -

static void wr_signals_init(void) {
	signal(SIGILL, wr_sig_crash);
	signal(SIGABRT, wr_sig_crash);
	signal(SIGFPE, wr_sig_crash);
	signal(SIGBUS, wr_sig_crash);
	signal(SIGSEGV, wr_sig_crash);
	signal(SIGPIPE, wr_sig_pipe);
}





static void wr_sig_pipe(int sigraised) {
}



static void wr_sig_int(int sigraised) {
	wr_running = 0;
}



static void wr_sig_crash(int sigraised) {
	wr_cleanup();
	
	signal(sigraised, SIG_DFL);
}

static void wd_block_signals(void) {
	wi_thread_block_signals(SIGHUP, SIGUSR1, SIGUSR2, SIGINT, SIGTERM, SIGQUIT, SIGPIPE, 0);
}



static int wd_wait_signals(void) {
	return wi_thread_wait_for_signals(SIGHUP, SIGUSR1, SIGUSR2, SIGINT, SIGTERM, SIGQUIT, SIGPIPE, 0);
}



void wd_signal_thread(wi_runtime_instance_t *arg) {
	wi_pool_t		*pool;
	int				signal;

	pool = wi_pool_init(wi_pool_alloc());

	while(wr_running == 1) {
		signal = wd_wait_signals();
		
		switch(signal) {
			case SIGPIPE:
				wi_log_warn(WI_STR("Signal PIPE received, ignoring"));
				break;
				
			case SIGHUP:
				wi_log_info(WI_STR("Signal HUP received, reloading configuration"));

				wd_settings_read_config();
				wr_client_reload_icon();
				
				if(wb_bot) {
					wb_bot_reload_configuration(wb_bot);
				}

				// wd_schedule();
				break;
				
			case SIGUSR1:
				wi_log_info(WI_STR("Signal USR1 received, TBD"));
				//wd_trackers_register();
				break;

			case SIGUSR2:
				wi_log_info(WI_STR("Signal USR2 received, TBD"));
				//wd_index_index_files(false);
				break;

			case SIGINT:
				wi_log_info(WI_STR("Signal INT received, quitting"));
				wr_running = 0;
				wr_client_disconnect();
				break;

			case SIGQUIT:
				wi_log_info(WI_STR("Signal QUIT received, quitting"));
				wr_running = 0;
				wr_client_disconnect();
				break;

			case SIGTERM:
				wi_log_info(WI_STR("Signal TERM received, quitting"));
				wr_running = 0;
				wr_client_disconnect();
				break;
		}
		
		wi_pool_drain(pool);
	}
	
	wi_release(pool);
}



#pragma mark -

static void wr_log_callback(wi_log_level_t level, wi_string_t *string) {
	wr_printf_prefix(WI_STR("%@"), string);
}



#pragma mark -

char * wr_readline_bookmark_generator(const char *text, int state) {
	static wi_array_t		*bookmarks;
	static wi_uinteger_t	index, count, length;
	wi_string_t				*path, *string, *bookmark;
	char					*match = NULL;
	
	if(state == 0) {
		wi_release(bookmarks);

		path		= wi_string_by_appending_path_component(wi_user_home(), WI_STR(WB_WIREBOT_USER_PATH));
		bookmarks	= wi_retain(wi_fs_directory_contents_at_path(path));
		index		= 0;
		count		= wi_array_count(bookmarks);
		length		= strlen(text);
	}
	
	string = wi_string_with_cstring(text);

	while(index < count) {
		bookmark = wi_array_data_at_index(bookmarks, index);
		index++;

		if(wi_string_index_of_string(bookmark, string, WI_STRING_SMART_CASE_INSENSITIVE) == 0) {
			match = strdup(wi_string_cstring(bookmark));
			
			break;
		}
	}

	return match;
}



#pragma mark -

wi_string_t * wr_string_for_bytes(wi_file_offset_t bytes) {
	double			kb, mb, gb, tb, pb;

	if(bytes < 1024)
		return wi_string_with_format(WI_STR("%llu bytes"), bytes);

	kb = (double) bytes / 1024.0;

	if(kb < 1024.0)
		return wi_string_with_format(WI_STR("%.1f KB"), kb);

	mb = (double) kb / 1024.0;

	if(mb < 1024.0)
		return wi_string_with_format(WI_STR("%.1f MB"), mb);

	gb = (double) mb / 1024.0;

	if(gb < 1024.0)
		return wi_string_with_format(WI_STR("%.1f GB"), gb);

	tb = (double) gb / 1024.0;

	if(tb < 1024.0)
		return wi_string_with_format(WI_STR("%.1f TB"), tb);

	pb = (double) tb / 1024.0;

	if(pb < 1024.0)
		return wi_string_with_format(WI_STR("%.1f PB"), pb);

	return NULL;
}


#pragma mark -

static void wb_write_pid(void) {
	wi_string_t		*homepath, *path, *string;

	homepath	= wi_user_home();
	path 		= wi_string_by_appending_path_component(homepath, WI_STR(".wirebot/wirebot.pid"));

	wi_log_info(WI_STR("Writting PID file: %@"), path);

	string 		= wi_string_with_format(WI_STR("%d\n"), getpid());
	
	if(!wi_string_write_to_file(string, path))
		wi_log_error(WI_STR("Could not write to \"%@\": %m"), path);
}



static void wb_delete_pid(void) {
	wi_string_t		*homepath, *path;
	
	homepath	= wi_user_home();
	path 		= wi_string_by_appending_path_component(homepath, WI_STR(".wirebot/wirebot.pid"));

	if(!wi_fs_delete_path(path))
		wi_log_error(WI_STR("Could not delete \"%@\": %m"), path);
}




#pragma mark -

void wr_runloop_init(void) {
	wr_runloop_sockets = wi_array_init(wi_mutable_array_alloc());
}



void wr_runloop_add_socket(wi_socket_t *socket, wr_runloop_callback_func_t *callback) {
	wi_socket_set_data(socket, callback);
	wi_mutable_array_add_data(wr_runloop_sockets, socket);
}



void wr_runloop_remove_socket(wi_socket_t *socket) {
	if(wr_runloop_sockets && socket && wi_array_contains_data(wr_runloop_sockets, socket))
		wi_mutable_array_remove_data(wr_runloop_sockets, socket);
}



static void wr_runloop_run(void) {
	wi_pool_t			*pool;
	wi_socket_t			*socket;
	wi_time_interval_t	interval, ping_interval;
	wi_uinteger_t		i = 0;
	wi_boolean_t		result;
	
	pool = wi_pool_init(wi_pool_alloc());

	socket = wi_socket_init_with_descriptor(wi_socket_alloc(), STDIN_FILENO);
	wi_socket_set_direction(socket, WI_SOCKET_READ);
	wr_runloop_add_socket(socket, &wr_runloop_stdin_callback);
	wi_release(socket);
	
	ping_interval = wi_time_interval();
	
	while(wr_running) {
		result = wr_runloop(wr_runloop_sockets, 30.0);
		
		if(!result && wr_connected) {
			interval = wi_time_interval();
			
			if(interval - ping_interval > 60.0) {
				wr_client_send_message(wi_p7_message_with_name(WI_STR("wired.send_ping"), wr_p7_spec));
				
				ping_interval = interval;
			}
			
			wi_pool_drain(pool);
		}
		
		if(++i % 100 == 0)
			wi_pool_drain(pool);
	}
	
	wi_release(pool);
}



void wr_runloop_run_for_socket(wi_socket_t *socket, wi_time_interval_t timeout, wi_uinteger_t message) {
	wi_array_t		*array;
	
	array = wi_array_init_with_data(wi_array_alloc(), socket, NULL);
	
	while(wr_running) {
		if(!wr_runloop(array, timeout))
			break;
	}
	
	wi_release(array);
}



#pragma mark -

static wi_integer_t wr_runloop(wi_array_t *array, wi_time_interval_t timeout) {
	wi_socket_t					*socket;
	wr_runloop_callback_func_t	*callback;
	
	socket = wi_socket_wait_multiple(array, timeout);
	
	if(socket) {
		callback = wi_socket_data(socket);
		
		return (*callback)(socket);
	}
	
	return false;
}



static wi_boolean_t wr_runloop_stdin_callback(wi_socket_t *socket) {
	//wr_readline_read();
	
	return true;
}
