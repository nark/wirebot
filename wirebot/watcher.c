/* $Id$ */

/*
 *  Copyright (c) 2012 Rafael Warnault
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

#include <wired/wired.h>
#include "spec.h"
#include "client.h"
#include "bot.h"
#include "watcher.h"
#include "settings.h"
#include "service.h"
#include "commands.h"
#include <wired/wired.h>
#include <string.h>

#pragma mark -

struct _wb_watcher {
	wi_runtime_base_t				base;

	wi_boolean_t					activated;
	wi_string_t						*path;
	wi_string_t						*type;
	wi_mutable_array_t				*services;
	wi_mutable_array_t				*outputs;

	wi_mutable_array_t 				*files;
	wi_mutable_array_t 				*new_files;
};  

static void							wb_watcher_dealloc(wi_runtime_instance_t *);
static wi_string_t *				wb_watcher_description(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_watcher_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_watcher_runtime_class = {
	"wb_watcher_t",
	wb_watcher_dealloc,
	NULL,
	NULL,
	wb_watcher_description,
	NULL
};

static wb_watcher_t * 				_wb_watcher_load_with_node(wb_watcher_t *, xmlNodePtr);

void 								wb_watcher_execute_output(wb_watcher_t *, wb_output_t *, wi_string_t *);
wi_string_t * 						wb_watcher_compute_output(wb_watcher_t *, wb_output_t *, wi_string_t *);





#pragma mark -

void wb_watchers_init(void) {
	wb_watcher_runtime_id = wi_runtime_register_class(&wb_watcher_runtime_class);
}



wb_watcher_t * wb_watcher_alloc(void) {
	return wi_runtime_create_instance(wb_watcher_runtime_id, sizeof(wb_watcher_t));
}


wb_watcher_t * wb_watcher_init(wb_watcher_t *watcher, xmlNodePtr node) {
	
	watcher->path 			= NULL;
	watcher->type 			= NULL;
	watcher->services 		= wi_array_init(wi_mutable_array_alloc());
	watcher->outputs		= wi_array_init(wi_mutable_array_alloc());

	watcher->files 			= wi_array_init(wi_mutable_array_alloc());
	watcher->new_files		= wi_array_init(wi_mutable_array_alloc());

	return _wb_watcher_load_with_node(watcher, node);
}




#pragma mark -

static wb_watcher_t * _wb_watcher_load_with_node(wb_watcher_t *watcher, xmlNodePtr node) {
	wi_string_t 			*path, *type, *activated;
	wb_output_t 			*output;
	wb_service_t 			*service;
	xmlNodePtr				sub_node, next_node;

	// get watcher path
	path = wi_xml_node_attribute_with_name(node, WI_STR("path"));
	if(path)
		watcher->path = wi_retain(path);

	type = wi_xml_node_attribute_with_name(node, WI_STR("type"));
	if(type)
		watcher->type = wi_retain(type);

	// is an activated watcher ?
	activated = wi_xml_node_attribute_with_name(node, WI_STR("activated"));
	if(activated)
		watcher->activated = wi_is_equal(activated, WI_STR("true")) ? true : false;

	// get children: services and outputs 
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {
			if(strcmp((const char *) sub_node->name, "output") == 0) {
				output = wb_output_init(wb_output_alloc(), sub_node);
				if(output)
					wi_mutable_array_add_data(watcher->outputs, output);

				wi_release(output);
			}
			else if(strcmp((const char *) sub_node->name, "service") == 0) {
				service = wb_service_init(wb_service_alloc(), sub_node);
				if(service)
					wi_mutable_array_add_data(watcher->services, service);

				wi_release(service);
			}
		}
	}

	return watcher;
}





#pragma mark -

wi_boolean_t wb_watcher_activated(wb_watcher_t *watcher) {
	return watcher->activated;
}

wi_string_t * wb_watcher_path(wb_watcher_t *watcher) {
	return watcher->path;
}

wi_string_t * wb_watcher_type(wb_watcher_t *watcher) {
	return watcher->type;
}

wi_mutable_array_t * wb_watcher_files(wb_watcher_t *watcher) {
	return watcher->files;
}

wi_mutable_array_t * wb_watcher_new_files(wb_watcher_t *watcher) {
	return watcher->new_files;
}

wi_mutable_array_t * wb_watcher_outputs(wb_watcher_t *watcher) {
	return watcher->outputs;
}





#pragma mark -

wi_boolean_t wb_watcher_add_file(wb_watcher_t *watcher, wi_string_t *path) {
	if(!wi_array_contains_data(watcher->files, path)) {
		wi_mutable_array_add_data(watcher->files, path);

		return true;
	}
	return false;
}

wi_boolean_t wb_watcher_add_new_file(wb_watcher_t *watcher, wi_string_t *path) {
	if(!wi_array_contains_data(watcher->new_files, path)) {
		wi_mutable_array_add_data(watcher->new_files, path);

		return true;
	}
	return false;
}

void wb_watcher_files_diff(wb_watcher_t *watcher) {
	wi_enumerator_t			*enumerator;
	wi_string_t 			*path;
	wb_service_t 			*service;
	int i;

	enumerator = wi_array_data_enumerator(watcher->files);

	while((path = wi_enumerator_next_data(enumerator))) {
		if(!wi_array_contains_data(watcher->new_files, path)) {
			if(!wi_is_equal(wi_string_path_extension(path), WI_STR("WiredTransfer"))) {
				wi_log_info(WI_STR("Watcher File Removed: %@"), path);
			}
		}
	}

	enumerator = wi_array_data_enumerator(watcher->new_files);

	while((path = wi_enumerator_next_data(enumerator))) {
		if(!wi_array_contains_data(watcher->files, path)) {
			if(!wi_is_equal(wi_string_path_extension(path), WI_STR("WiredTransfer"))) {
				if(wi_array_count(watcher->outputs) > 0) {
					wi_log_info(WI_STR("Watcher File Added: %@"), path);
					
					for(i = 0; i < wi_array_count(watcher->outputs); i++) {
						wb_watcher_execute_output(watcher, wi_array_data_at_index(watcher->outputs, i), path);
					}
				}
			}
		}
	}

	wi_release(watcher->files);

	watcher->files = wi_copy(watcher->new_files);
	wi_mutable_array_remove_all_data(watcher->new_files);
}



void wb_watcher_execute_output(wb_watcher_t *watcher, wb_output_t *output, wi_string_t *path) {
	wi_p7_message_t 	*message;
	wb_service_t 		*service;
	wi_string_t 		*board, *subject, *text, *output_string;

	service = NULL;

	if(wi_array_count(watcher->services) > 0) {
		service = wi_array_data_at_index(watcher->services, 0);

		wb_service_set_file_path(service, path);
		wb_service_execute(service, watcher);
	}

	if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.board.add_thread"))) {
		board = wb_output_board(output);

		if(board) {
			output_string = wb_watcher_compute_output(watcher, output, path);

			if(service && wb_service_text(service)) {
				text = wb_service_text(service);
			} else {
				text = wi_string_with_format(
					WI_STR("[b]Name:[/b] %@\n[b]Path:[/b] %@\n"), 
					wi_string_last_path_component(path), 
					watcher->path);
			}

			message = wi_p7_message_with_name(WI_STR("wired.board.add_thread"), wr_p7_spec);
			wi_p7_message_set_string_for_name(message, board, WI_STR("wired.board.board"));
			wi_p7_message_set_string_for_name(message, output_string, WI_STR("wired.board.subject"));
			wi_p7_message_set_string_for_name(message, text, WI_STR("wired.board.text"));

			if(message) {
				if(wr_connected) {
					wr_client_send_message(message);
				}
			}
		}	
	} 
	else if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.chat.say"))) {
		output_string = wb_watcher_compute_output(watcher, output, path);
		wr_commands_parse_command(output_string, true);
	} 

	if(service) {
		wb_service_cleanup(service);
	}
}


wi_string_t * wb_watcher_compute_output(wb_watcher_t *watcher, wb_output_t *output, wi_string_t *path) {
	wi_string_t 		*result;

	result = wb_output_output(output);

	if(wi_string_contains_string(wb_output_output(output), WB_WATCHER_PATH, 0)) {
		result = wi_string_by_replacing_string_with_string(result, WB_WATCHER_PATH, watcher->path, 0);
	} 
	if(wi_string_contains_string(wb_output_output(output), WB_WATCHER_FILE, 0)) {
		result = wi_string_by_replacing_string_with_string(result, WB_WATCHER_FILE, wi_string_last_path_component(path), 0);
	} 

	if(!result)
		result = wb_output_output(output);

	return result;
}



#pragma mark -

static void wb_watcher_dealloc(wi_runtime_instance_t *instance) {
	wb_watcher_t		* watcher = instance;

	wi_release(watcher->path);
	wi_release(watcher->type);
	wi_release(watcher->services);
	wi_release(watcher->outputs);
	wi_release(watcher->files);
	wi_release(watcher->new_files);
}

static wi_string_t * wb_watcher_description(wi_runtime_instance_t *instance) {
	wb_watcher_t		* watcher = instance;
	wi_string_t 		* string;

	string = wi_string_with_format(WI_STR("Watcher: [%@] (%@) (%@)"), watcher->path, watcher->type, watcher->files);

	return string;
}


