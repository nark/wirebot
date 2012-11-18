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

#include <stdio.h>
#include <readline/readline.h>
#include <wired/wired.h>

#include "client.h"
#include "terminal.h"
#include "users.h"
#include "transfers.h"
#include "windows.h"

static void							wr_window_dealloc(wi_runtime_instance_t *);

static wr_wid_t						wr_window_wid(void);


struct _wr_window {
	wi_runtime_base_t				base;
	
	wr_wid_t						wid;
	wr_window_type_t				type;
	wr_window_status_t				status;
	
	wr_chat_t						*chat;
	wr_user_t						*user;
	wr_topic_t						*topic;
	
	wi_terminal_buffer_t			*buffer;
};


wi_mutable_array_t					*wr_windows;

wr_window_t							*wr_console_window;
wr_window_t							*wr_current_window;

static wi_runtime_id_t				wr_window_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wr_window_runtime_class = {
	"wr_window_t",
	wr_window_dealloc,
	NULL,
	NULL,
	NULL,
	NULL
};


void wr_windows_initialize(void) {
	wr_window_runtime_id = wi_runtime_register_class(&wr_window_runtime_class);

	wr_windows = wi_array_init(wi_mutable_array_alloc());

	wr_console_window = wr_window_init_with_chat(wr_window_alloc(), wr_public_chat);
	wr_windows_add_window(wr_console_window);

	wr_windows_show_window(wr_console_window);

	wr_windows_set_timestamp_format(WI_STR("%H:%M"));
}



void wr_windows_clear(void) {
	wr_window_t		*window;
	wi_uinteger_t	i, count;

	count = wi_array_count(wr_windows);
	
	for(i = 0; i < count; i++) {
		window = WI_ARRAY(wr_windows, i);
		
		if(window != wr_console_window) {
			wr_windows_close_window(window);
			
			i--;
			count--;
		}
	}
	
	wr_windows_show_window(wr_console_window);
}



#pragma mark -

void wr_windows_set_timestamp_format(wi_string_t *timestamp_format) {
	if(wi_is_equal(timestamp_format, wr_timestamp_format))
		return;
	
	wi_release(wr_timestamp_format);
	wr_timestamp_format = wi_retain(timestamp_format);

	wr_printf_prefix(WI_STR("Using timestamp format %@"), wr_timestamp_format);
}



#pragma mark -

void wr_windows_add_window(wr_window_t *window) {
	wi_mutable_array_add_data(wr_windows, window);
	wi_terminal_add_buffer(wr_terminal, window->buffer);

	if(wr_window_is_private_chat(window) || wr_window_is_user(window)) {
		if(wr_window_is_user(window))
			wr_wprintf_prefix(window, WI_STR("Opened new window for private messages with %@"), wr_user_nick(window->user));
		else
			wr_wprintf_prefix(window, WI_STR("Opened new window for private chat"));
		
		wr_wprintf_prefix(window, WI_STR("Use ctrl-N/ctrl-P to cycle windows, and /close to exit"));
	}
}



void wr_windows_close_window(wr_window_t *window) {
	wi_uinteger_t	index, previous_index;
	
	index = wi_array_index_of_data(wr_windows, window);
	
	if(index == 0)
		previous_index = wi_array_count(wr_windows) - 1;
	else
		previous_index = index - 1;
	
	wr_windows_show_window(WI_ARRAY(wr_windows, previous_index));
	
	wi_terminal_remove_buffer(wr_terminal, window->buffer);

	wi_mutable_array_remove_data_at_index(wr_windows, index);
}



wr_window_t * wr_windows_window_with_chat(wr_chat_t *chat) {
	wi_enumerator_t	*enumerator;
	wr_window_t		*window;

	enumerator = wi_array_data_enumerator(wr_windows);
	
	while((window = wi_enumerator_next_data(enumerator))) {
		if(wr_window_is_chat(window) && wi_is_equal(window->chat, chat))
			return window;
	}
	
	return NULL;
}



wr_window_t * wr_windows_window_with_user(wr_user_t *user) {
	wi_enumerator_t	*enumerator;
	wr_window_t		*window;
	
	enumerator = wi_array_data_enumerator(wr_windows);
	
	while((window = wi_enumerator_next_data(enumerator))) {
		if(wr_window_is_user(window) && wi_is_equal(window->user, user))
			return window;
	}
	
	return NULL;
}



void wr_windows_show_next(void) {
	wi_uinteger_t	index, next_index;
	
	index = wi_array_index_of_data(wr_windows, wr_current_window);
	
	if(index == wi_array_count(wr_windows) - 1)
		next_index = 0;
	else
		next_index = index + 1;
	
	if(index != next_index)
		wr_windows_show_window(WI_ARRAY(wr_windows, next_index));
}



void wr_windows_show_previous(void) {
	wi_uinteger_t	index, previous_index;
	
	index = wi_array_index_of_data(wr_windows, wr_current_window);
	
	if(index == 0)
		previous_index = wi_array_count(wr_windows) - 1;
	else
		previous_index = index - 1;
	
	if(index != previous_index)
		wr_windows_show_window(WI_ARRAY(wr_windows, previous_index));
}



void wr_windows_show_window(wr_window_t *window) {
	if(wr_current_window != window) {
		wr_current_window = window;
		wr_current_window->status = WR_WINDOW_STATUS_IDLE;
		
		wi_terminal_clear_screen(wr_terminal);
		wi_terminal_set_active_buffer(wr_terminal, wr_current_window->buffer);
		wi_terminal_buffer_redraw(wr_current_window->buffer);

		wr_draw_header();
		wr_draw_divider();
	}
}



#pragma mark -

wr_window_t * wr_window_alloc(void) {
	return wi_runtime_create_instance(wr_window_runtime_id, sizeof(wr_window_t));
}



wr_window_t * wr_window_init(wr_window_t *window) {
	window->buffer		= wi_terminal_buffer_init_with_terminal(wi_terminal_buffer_alloc(), wr_terminal);
	window->wid			= wr_window_wid();
	window->status		= WR_WINDOW_STATUS_IDLE;
	
	return window;
}



wr_window_t * wr_window_init_with_chat(wr_window_t *window, wr_chat_t *chat) {
	window = wr_window_init(window);

	window->type		= WR_WINDOW_TYPE_CHAT;
	window->chat		= wi_retain(chat);
	
	return window;
}



wr_window_t * wr_window_init_with_user(wr_window_t *window, wr_user_t *user) {
	window = wr_window_init(window);
	
	window->type		= WR_WINDOW_TYPE_USER;
	window->user		= wi_retain(user);
	window->topic		= wr_topic_init_with_user(wr_topic_alloc(), user);
	
	return window;
}



static void wr_window_dealloc(wi_runtime_instance_t *instance) {
	wr_window_t		*window = instance;
	
	wi_release(window->chat);
	wi_release(window->user);

	wi_release(window->topic);

	wi_release(window->buffer);
}



#pragma mark -

static wr_wid_t wr_window_wid(void) {
	wr_window_t		*window;

	if(wi_array_count(wr_windows) > 0) {
		window = wi_array_last_data(wr_windows);
		
		return window->wid + 1;
	}

	return 1;
}



#pragma mark -

wr_chat_t * wr_window_chat(wr_window_t *window) {
	return window->chat;
}



wi_boolean_t wr_window_is_chat(wr_window_t *window) {
	return (window->type == WR_WINDOW_TYPE_CHAT);
}



wi_boolean_t wr_window_is_public_chat(wr_window_t *window) {
	return (window->type == WR_WINDOW_TYPE_CHAT && wi_is_equal(window->chat, wr_public_chat));
}



wi_boolean_t wr_window_is_private_chat(wr_window_t *window) {
	return (window->type == WR_WINDOW_TYPE_CHAT && !wi_is_equal(window->chat, wr_public_chat));
}



wi_boolean_t wr_window_is_user(wr_window_t *window) {
	return (window->type == WR_WINDOW_TYPE_USER);
}



void wr_window_update_status(wr_window_t *window) {
	wi_uinteger_t	line, lines;
	
	line = wi_terminal_buffer_current_line(window->buffer);
	lines = wi_terminal_buffer_lines(window->buffer);

	if(line == lines)
		window->status = WR_WINDOW_STATUS_IDLE;
}



#pragma mark -

wr_user_t * wr_window_user(wr_window_t *window) {
	return window->user;
}



#pragma mark -

wi_terminal_buffer_t * wr_window_buffer(wr_window_t *window) {
	return window->buffer;
}



#pragma mark -

void wr_window_set_topic(wr_window_t *window, wr_topic_t *topic) {
	wi_retain(topic);
	wi_release(window->topic);
	
	window->topic = topic;
}



wr_topic_t * wr_window_topic(wr_window_t *window) {
	return window->topic;
}



#pragma mark -

void wr_printf(wi_string_t *fmt, ...) {
	wi_string_t		*string;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	wr_wprint(wr_current_window, string);

	wi_release(string);
}



void wr_wprintf(wr_window_t *window, wi_string_t *fmt, ...) {
	wi_string_t		*string;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	wr_wprint(window, string);
	
	wi_release(string);
}



void wr_printf_prefix(wi_string_t *fmt, ...) {
	wi_string_t		*string;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	wr_wprintf_prefix(wr_current_window, WI_STR("%@"), string);

	wi_release(string);
}



void wr_wprintf_prefix(wr_window_t *window, wi_string_t *fmt, ...) {
	wi_string_t		*string;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	wr_wprintf(window, WI_STR("%s%s%s%@"),
		WR_PREFIX_COLOR,
		WR_PREFIX,
		WR_TERMINATE_COLOR,
		string);

	wi_release(string);
}



void wr_printf_block(wi_string_t *fmt, ...) {
	wi_string_t		*string;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	wr_wprintf_block(wr_current_window, WI_STR("%@"), string);
	
	wi_release(string);
}



void wr_wprintf_block(wr_window_t *window, wi_string_t *fmt, ...) {
	wi_enumerator_t	*enumerator;
	wi_string_t		*string, *line;
	va_list			ap;

	va_start(ap, fmt);
	string = wi_string_init_with_format_and_arguments(wi_string_alloc(), fmt, ap);
	va_end(ap);

	enumerator = wi_array_data_enumerator(wi_string_components_separated_by_string(string, WI_STR("\n")));
	
	while((line = wi_enumerator_next_data(enumerator)))
		wr_wprintf(window, WI_STR("   %@"), line);

	wi_release(string);
}



void wr_wprint(wr_window_t *window, wi_string_t *string) {
	wi_string_t		*timestamp, *line;

	timestamp = wi_date_string_with_format(wi_date(), wr_timestamp_format);
	
	if(wi_string_length(timestamp) > 0)
		line = wi_string_with_format(WI_STR("%@ %@"), timestamp, string);
	else
		line = string;

	if(wi_terminal_buffer_printf(window->buffer, WI_STR("%@"), line)) {
		window->status = WR_WINDOW_STATUS_IDLE;
	} else {
		if(window->status < WR_WINDOW_STATUS_ACTION)
			window->status = WR_WINDOW_STATUS_ACTION;
	}

	wr_terminal_reset_location();
	
	wr_draw_divider();
}



void wr_wprint_say(wr_window_t *window, wi_string_t *nick, wi_string_t *chat) {
	wi_string_t		*prefix;
	const char		*color;
	wi_uinteger_t	length;

	length = wi_string_length(wr_nick);

	if(length > 4)
		length = 4;

	prefix = wi_string_substring_to_index(wr_nick, length);
	
	if(wi_string_has_prefix(chat, prefix)) {
		color = WR_HIGHLIGHT_COLOR;
		
		if(window->status < WR_WINDOW_STATUS_HIGHLIGHT)
			window->status = WR_WINDOW_STATUS_HIGHLIGHT;
	} else {
		color = WR_NICK_COLOR;
		
		if(window->status < WR_WINDOW_STATUS_CHAT)
			window->status = WR_WINDOW_STATUS_CHAT;
	}
		
	wr_wprintf(window, WI_STR("%s<%s%s%@%s%s>%s %@"),
		WR_SAY_COLOR,
		WR_TERMINATE_COLOR,
		color,
		nick,
		WR_TERMINATE_COLOR,
		WR_SAY_COLOR,
		WR_TERMINATE_COLOR,
		chat);
}



void wr_wprint_me(wr_window_t *window, wi_string_t *nick, wi_string_t *chat) {
	wi_string_t		*prefix;
	const char		*color;
	wi_uinteger_t	length;

	length = wi_string_length(wr_nick);

	if(length > 4)
		length = 4;

	prefix = wi_string_substring_to_index(wr_nick, length);
	
	if(wi_string_has_prefix(chat, prefix)) {
		color = WR_HIGHLIGHT_COLOR;
		
		if(window->status < WR_WINDOW_STATUS_HIGHLIGHT)
			window->status = WR_WINDOW_STATUS_HIGHLIGHT; 
	} else {
		color = NULL;
		
		if(window->status < WR_WINDOW_STATUS_CHAT)
			window->status = WR_WINDOW_STATUS_CHAT;
	}

	wr_wprintf(window, WI_STR("%s*%s %s%@%s %@"),
		WR_ME_COLOR,
		WR_TERMINATE_COLOR,
		color ? color : "",
		nick,
		color ? WR_TERMINATE_COLOR : "",
		chat);
}



void wr_wprint_msg(wr_window_t *window, wi_string_t *nick, wi_string_t *message) {
	if(window->status < WR_WINDOW_STATUS_HIGHLIGHT)
		window->status = WR_WINDOW_STATUS_HIGHLIGHT;
	
	wr_wprintf(window, WI_STR("%s<%s%@%s>%s %@"),
		WR_SAY_COLOR,
		WR_TERMINATE_COLOR,
		nick,
		WR_SAY_COLOR,
		WR_TERMINATE_COLOR,
		message);
}



void wr_print_topic(void) {
	wr_printf_prefix(WI_STR("Topic: %#@"), wr_topic_topic(wr_current_window->topic));
	
	if(wr_topic_user_nick(wr_current_window->topic)) {
		wr_printf_prefix(WI_STR("Topic set by %@ - %@"),
			wr_topic_user_nick(wr_current_window->topic),
			wi_date_string_with_format(wr_topic_time(wr_current_window->topic), WI_STR("%a %b %e %T %Y")));
	}
}



void wr_print_users(wr_window_t *window) {
	wi_enumerator_t	*enumerator;
	wi_array_t		*users;
	wr_user_t       *user;
	wi_uinteger_t    max_length = 0;
	
	if(window->chat == wr_public_chat)
		wr_printf_prefix(WI_STR("Users currently online:"));
	else
		wr_printf_prefix(WI_STR("Users in chat:"));
	
	users = wr_chat_users(window->chat);
	enumerator = wi_array_data_enumerator(users);
	
	while((user = wi_enumerator_next_data(enumerator)))
		max_length = WI_MAX(max_length, wi_string_length(wr_user_nick(user)));

	enumerator = wi_array_data_enumerator(users);

	while((user = wi_enumerator_next_data(enumerator)))
		wr_print_user(user, max_length);
}



void wr_print_user(wr_user_t *user, wi_uinteger_t max_length) {
	const char		*color;

	switch(wr_user_color(user)) {
		case WR_USER_COLOR_BLACK:
		default:
			color = wr_user_is_idle(user) ? WR_WHITE_COLOR : WR_BRIGHT_WHITE_COLOR;
			break;
			
		case WR_USER_COLOR_RED:
		case WR_USER_COLOR_ORANGE:
			color = wr_user_is_idle(user) ? WR_RED_COLOR : WR_BRIGHT_RED_COLOR;
			break;
			
		case WR_USER_COLOR_GREEN:
			color = wr_user_is_idle(user) ? WR_GREEN_COLOR : WR_BRIGHT_GREEN_COLOR;
			break;
			
		case WR_USER_COLOR_BLUE:
			color = wr_user_is_idle(user) ? WR_BLUE_COLOR : WR_BRIGHT_BLUE_COLOR;
			break;
			
		case WR_USER_COLOR_PURPLE:
			color = wr_user_is_idle(user) ? WR_MAGENTA_COLOR : WR_BRIGHT_MAGENTA_COLOR;
			break;
	}
	
	wr_printf_block(WI_STR("%s%@%s%*s%@"),
		color,
		wr_user_nick(user),
		WR_TERMINATE_COLOR,
		max_length - wi_string_length(wr_user_nick(user)) + 4,
		" ",
		wr_user_status(user));
}



#pragma mark -

void wr_draw_header(void) {
	wi_mutable_string_t		*topic;
	wi_size_t				size;
	
	if(wr_current_window->topic)
		topic = wi_mutable_copy(wr_topic_topic(wr_current_window->topic));
	else
		topic = wi_string_init(wi_mutable_string_alloc());

	wi_terminal_adjust_string_to_fit_width(wr_terminal, topic);
	
	size = wi_terminal_size(wr_terminal);
	
	wi_terminal_move_printf(wr_terminal, wi_make_point(0, 0), WI_STR("%s%@%s"),
	   WR_INTERFACE_COLOR,
	   topic,
	   WR_TERMINATE_COLOR);
	wi_terminal_move(wr_terminal, wi_make_point(rl_point % size.width, size.height - 2));
	
	wi_release(topic);
}


void wr_draw_transfers(wi_boolean_t force) {
	static wi_time_interval_t	update;
	wi_enumerator_t				*enumerator;
	wi_mutable_string_t			*string;
    wi_string_t					*status;
	wr_transfer_t				*transfer;
	wi_time_interval_t			interval;
	wi_uinteger_t				i = 0;
	
	interval = wi_time_interval();
	
	if(!force && interval - update < 1.0)
		return;
	
	update = interval;
	
	wi_terminal_set_scroll(wr_terminal, wi_make_range(1 + wi_array_count(wr_transfers),
													  wi_terminal_size(wr_terminal).height - 4));
	
	enumerator = wi_array_data_enumerator(wr_transfers);
	
	while((transfer = wi_enumerator_next_data(enumerator))) {
		wi_terminal_move(wr_terminal, wi_make_point(0, i + 1));
		wi_terminal_clear_line(wr_terminal);
		
		if(transfer->state == WR_TRANSFER_RUNNING && interval - transfer->start_time > 0.0) {
			transfer->speed = ((double) transfer->total_transferred - transfer->total_offset) / (interval - transfer->start_time);
			
			status = wi_string_with_format(WI_STR("%@/%@, %@/s"),
                                           wr_files_string_for_size(transfer->total_transferred),
                                           wr_files_string_for_size(transfer->total_size),
                                           wr_files_string_for_size(transfer->speed));
		}
		else if(transfer->state == WR_TRANSFER_QUEUED) {
			status = wi_string_with_format(WI_STR("queued at %u"),
                                           transfer->queue);
		}
		else {
			status = wi_string_with_cstring("waiting");
		}
		
		string = wi_mutable_string_with_format(WI_STR("%u %3.0f%%  %@"),
                                               transfer->tid,
                                               transfer->total_size > 0
                                               ? 100 * ((double) transfer->total_transferred / (double) transfer->total_size)
                                               : 0,
                                               transfer->name);
		
		wi_terminal_adjust_string_to_fit_width(wr_terminal, string);
		wi_mutable_string_delete_characters_from_index(string, wi_string_length(string) - wi_string_length(status));
		wi_mutable_string_append_string(string, status);
		
		wi_terminal_printf(wr_terminal, WI_STR("%@"), string);
		
		i++;
	}
	
	wr_terminal_reset_location();
}



void wr_draw_divider(void) {
	wi_enumerator_t			*enumerator;
	wi_mutable_string_t		*string, *action;
	wi_string_t				*position;
	wr_window_t				*window;
	wi_size_t				size;
	wi_range_t				scroll;
	wi_point_t				location;
	const char				*color;
	wi_uinteger_t			users, line, lines, windows;
	
	string = wi_mutable_string_with_format(WI_STR("%s%@"), WR_PREFIX, wr_nick);
	
	if(wr_connected && wr_server) {
		users = wi_array_count(wr_chat_users(wr_console_window->chat));
		
		wi_mutable_string_append_format(string, WI_STR(" - %@ - %u %@"),
										wr_server_name(wr_server),
										users,
										users == 1
											? WI_STR("user")
											: WI_STR("users"));
	}
	
	action = wi_mutable_string();
	windows = 0;
	
	enumerator = wi_array_data_enumerator(wr_windows);
	
	while((window = wi_enumerator_next_data(enumerator))) {
		switch(window->status) {
			case WR_WINDOW_STATUS_ACTION:
				color = WR_INTERFACE_COLOR;
				break;

			case WR_WINDOW_STATUS_CHAT:
				color = WR_STATUS_COLOR;
				break;

			case WR_WINDOW_STATUS_HIGHLIGHT:
				color = WR_HIGHLIGHT_COLOR;
				break;
				
			default:
				color = NULL;
				break;
		}
		
		if(color) {
			if(windows > 0)
				wi_mutable_string_append_string(action, WI_STR(","));
			
			wi_mutable_string_append_format(action, WI_STR("%s%s%s%u%s%s"),
				WR_TERMINATE_COLOR,
				WR_INTERFACE_COLOR,
				color,
				window->wid,
				WR_TERMINATE_COLOR,
				WR_INTERFACE_COLOR);
			
			windows++;
		}
	}

	if(windows > 0)
		wi_mutable_string_append_format(string, WI_STR(" [%@]"), action);
	
	line = wi_terminal_buffer_current_line(wr_current_window->buffer);
	lines = wi_terminal_buffer_lines(wr_current_window->buffer);
	scroll = wi_terminal_scroll(wr_terminal);
	
	if(lines == 0 || line == lines)
		position = NULL;
	else if(line <= scroll.length)
		position = wi_string_with_cstring("TOP");
	else
		position = wi_string_with_format(WI_STR("%.0f%%"), 100 * ((double) (line - scroll.length)  / (double) lines));
	
	wi_terminal_adjust_string_to_fit_width(wr_terminal, string);
	
	if(position) {
		wi_mutable_string_delete_characters_from_index(string, wi_string_length(string) - wi_string_length(position));
		wi_mutable_string_append_string(string, position);
	}

	size = wi_terminal_size(wr_terminal);

	location = wi_terminal_location(wr_terminal);
	wi_terminal_move_printf(wr_terminal, wi_make_point(0, size.height - 3), WI_STR("%s%@%s"),
		WR_INTERFACE_COLOR,
		string,
		WR_TERMINATE_COLOR);
	wi_terminal_move(wr_terminal, location);
}
