/* $Id$ */

/*
 *  Copyright (c) 2003-2009 Axel Andersson
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

#include <wired/wired.h>

#include "files.h"
#include "main.h"
#include "server.h"
#include "settings.h"
#include "transfers.h"
#include "spec.h"
#include "client.h"
#include "bot.h"


wi_config_t						*wd_config;
wi_dictionary_t					*types, *defaults;


void wd_settings_initialize(void) {
	wi_string_t 	*dict_path;
	
	types = wi_dictionary_with_data_and_keys(
		WI_INT32(WI_CONFIG_STRING),				WI_STR("login"),
		WI_INT32(WI_CONFIG_STRING),				WI_STR("password"),
		WI_INT32(WI_CONFIG_STRING),				WI_STR("hostname"),
		WI_INT32(WI_CONFIG_PORT),				WI_STR("port"),
		WI_INT32(WI_CONFIG_STRING),				WI_STR("nick"),
		WI_INT32(WI_CONFIG_STRING),				WI_STR("status"),
		WI_INT32(WI_CONFIG_PATH),				WI_STR("icon path"),
		WI_INT32(WI_CONFIG_PATH),				WI_STR("dictionary path"),
		WI_INT32(WI_CONFIG_BOOL),				WI_STR("auto reconnect"),
		WI_INT32(WI_CONFIG_BOOL),				WI_STR("reconnect on kick"),
		WI_INT32(WI_CONFIG_STRING),				WI_STR("omdb api key"),
		NULL);
	
	defaults = wi_dictionary_with_data_and_keys(
		WI_STR("admin"),						WI_STR("login"),
		WI_STR(""),								WI_STR("password"),
		WI_STR("localhost"),					WI_STR("hostname"),
		WI_INT32(4871),							WI_STR("port"),
		WI_STR("WireBot"),						WI_STR("nick"),
		WI_STR("Jedi in the Matrix"),			WI_STR("status"),
		WI_STR("icon.png"),						WI_STR("icon path"),
		WI_STR("wirebot.xml"),					WI_STR("dictionary path"),
		wi_number_with_bool(true),				WI_STR("auto reconnect"),
		wi_number_with_bool(false),				WI_STR("reconnect on kick"),
		WI_STR(""),										WI_STR("omdb api key"),
		NULL);
	
	wd_config = wi_config_init_with_path(wi_config_alloc(), wr_config_path, types, defaults);
}



wi_boolean_t wd_settings_read_config(void) {
	wi_boolean_t		result;
	wi_file_t 			*file;

	if(!wi_fs_path_exists(wr_config_path, false)) {
		file = wi_file_for_writing(wr_config_path);

		wi_config_read_file(wd_config);

		wi_config_note_change(wd_config, WI_STR("login"));
		wi_config_note_change(wd_config, WI_STR("password"));
		wi_config_note_change(wd_config, WI_STR("hostname"));
		wi_config_note_change(wd_config, WI_STR("port"));
		wi_config_note_change(wd_config, WI_STR("nick"));
		wi_config_note_change(wd_config, WI_STR("status"));
		wi_config_note_change(wd_config, WI_STR("dictionary path"));
		wi_config_note_change(wd_config, WI_STR("icon path"));
		wi_config_note_change(wd_config, WI_STR("auto reconnect"));
		wi_config_note_change(wd_config, WI_STR("reconnect on kick"));
		wi_config_note_change(wd_config, WI_STR("omdb api key"));
		
		result = wi_config_write_file(wd_config);
	} else {
		result = wi_config_read_file(wd_config);
	}
	
	return result;
}



void wd_settings_apply_settings(wi_set_t *changes) {
	wr_client_apply_settings(changes);
	wb_bot_apply_settings(changes);
}

