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

#include "bot.h"
#include "spec.h"
#include "chats.h"
#include "client.h"
#include "commands.h"
#include "messages.h"
#include "settings.h"
#include <wired/wired.h>






struct _wb_bot {
	wi_runtime_base_t				base;

	wi_boolean_t					started;
	wi_boolean_t					subscribing;
	wi_string_t 					*path;
	wi_string_t						*xml;
	wi_mutable_array_t				*commands;
	wi_mutable_array_t				*rules;
	wi_mutable_array_t				*watchers;
};  

static void							wb_bot_dealloc(wi_runtime_instance_t *);
static wi_string_t *				wb_bot_description(wi_runtime_instance_t *);
static wi_hash_code_t				wb_bot_hash(wi_runtime_instance_t *);


static wi_runtime_id_t				wb_bot_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_bot_runtime_class = {
	"wb_bot_t",
	wb_bot_dealloc,
	NULL,
	NULL,
	wb_bot_description,
	wb_bot_hash
};








static wi_array_t * 				_wb_bot_decompose_bot_command_arguments(wi_p7_message_t *);
static wi_string_t * 				_wb_bot_recompose_bot_command_arguments(wi_array_t *);
static void 						_wb_bot_split_command(wi_string_t *, wi_string_t **, wi_string_t **);

static wi_boolean_t 				_wb_bot_load_file(wb_bot_t *, wi_string_t *);
static wi_boolean_t 				_wb_bot_load_wirebot(wb_bot_t *, xmlDocPtr);
static wi_boolean_t 				_wb_bot_load_rules(wb_bot_t *, xmlNodePtr);
static wi_boolean_t 				_wb_bot_load_commands(wb_bot_t *, xmlNodePtr);
static wi_boolean_t 				_wb_bot_load_watchers(wb_bot_t *, xmlNodePtr);

static wb_input_t*					_wb_bot_load_input_with_node(wb_input_t *, xmlNodePtr);
static wb_output_t*					_wb_bot_load_output_with_node(wb_output_t *, xmlNodePtr);

void 								_wb_bot_subscribe_to_remote_directory_for_watcher(wb_bot_t *, wb_watcher_t *);
void								_wb_bot_unsubscribe_to_remote_directory_for_watcher(wb_bot_t *, wb_watcher_t *);





#pragma mark -
#pragma mark initializer methods

void wb_bot_initialize(void) {
	wb_bot_runtime_id = wi_runtime_register_class(&wb_bot_runtime_class);
}




void wb_bot_init(void) {
	wi_string_t 			*dict_path;
	wi_string_t 			*dict_string;

	// init XML parser
	xmlInitParser();

	// get dictionary path from the config
	dict_path = wi_config_path_for_name(wd_config, WI_STR("dictionary path"));

	// check if dictionary path is relatiove or absolute path. If relative, append it to the user wirebot folder (.wirebot)
	if(!wi_string_has_prefix(dict_path, WI_STR("/")))
		dict_path = wi_string_by_appending_path_component(wi_string_by_appending_path_component(wi_user_home(), WI_STR(".wirebot")), dict_path);

	// if the dictionary doesn't exist at path, setup the default dictionary
	if(!wi_fs_path_exists(dict_path, false)) {
		if(wi_fs_path_exists(WI_STR("/usr/local/share/doc/wirebot/wirebot.xml"), false))
			dict_string = wi_string_init_with_contents_of_file(wi_string_alloc(), WI_STR("/usr/local/share/doc/wirebot/wirebot.xml"));
			wi_string_write_to_file(dict_string, dict_path);
			wi_release(dict_string);
	}

	// init the bot here
	if(wi_fs_path_exists(dict_path, false))
   	 	wb_bot = wb_bot_init_with_file(wb_bot_alloc(), dict_path);

   	 wi_log_info(WI_STR("wb_bot path: %@"), wb_bot->path);
}




#pragma mark -
#pragma mark allocator methods

wb_bot_t * wb_bot_alloc(void) {
	return wi_runtime_create_instance(wb_bot_runtime_id, sizeof(wb_bot_t));
}




#pragma mark -
#pragma mark constructor methods

wb_bot_t * wb_bot_init_with_file(wb_bot_t *bot, wi_string_t *path) {

	bot->started 				= true;
	bot->subscribing			= false;
	bot->path					= wi_retain(path);
	bot->commands				= wi_array_init(wi_mutable_array_alloc());
	bot->rules					= wi_array_init(wi_mutable_array_alloc());
	bot->watchers 				= wi_array_init(wi_mutable_array_alloc());

	if(!_wb_bot_load_file(bot, path)) {
		wi_log_error(WI_STR("Wirebot cannot be initialized properly, shutdown."), path);
		exit(-1);
	}
	
	return bot;
}




#pragma mark bot engine methods

wi_boolean_t wb_bot_dispatch_message(wb_bot_t *bot, wi_p7_message_t *message) {
	wi_array_t 				*outputs;
	wr_user_t       		*user;
	wb_command_t 			*command;
	wb_output_t 			*output;
	wi_string_t				*buffer, *cmd_string = NULL, *args_string = NULL;
	wi_p7_uint32_t			uid;

	wi_p7_message_get_uint32_for_name(message, &uid, WI_STR("wired.user.id"));

	// get the user
	user 	= wr_chat_user_with_uid(wr_public_chat, uid);
	buffer	= wb_bot_input_for_message(message);

	// do not reply to myself
	if(wr_user_id(user) == wb_user_id)
		return false;

	// split command and arguments
	_wb_bot_split_command(buffer, &cmd_string, &args_string);

	// check the bot is properly loaded
	if(!wb_bot)
		return false;

	// get command for input user and message
	command 	= wb_bot_command_for_message(wb_bot, user, message);

	// execute command
	if(command)
		return wb_bot_execute_command(bot, command, args_string, user, message);

	if(bot->started) {
		// get outputs for input user and message
		outputs 		= wb_bot_outputs_for_message(wb_bot, user, message);

		if(outputs) {
			if(wi_array_count(outputs) > 1) {
				// if multiple outputs found, get a random output
				output = wb_bot_select_random_output(outputs);

			} 
			// elese get the first output
			else if(wi_array_count(outputs) == 1)
				output = wi_array_first_data(outputs);

			if(output) {
				// execute the output: reply a message
				return wb_bot_execute_output(output, user);
			}
		}
	}
	return false;
}


void wb_bot_apply_settings(wi_set_t *changes) {
	if(wb_bot) {
		wb_bot_reload_configuration(wb_bot);
	}
}






// void wb_bot_add_file_to_watchers(wr_file_t *file, wb_bot_t *bot) {
// 	wi_enumerator_t			*enumerator;
// 	wb_watcher_t 			*watcher;
// 	wi_string_t 			*path;

// 	if(!bot->watchers)
// 		return;

// 	enumerator = wi_array_data_enumerator(bot->watchers);

// 	while((watcher = wi_enumerator_next_data(enumerator))) {

// 	}
// }

void wb_bot_subscribe_watchers(wb_bot_t *bot) {
	wi_enumerator_t			*enumerator;
	wb_watcher_t 			*watcher;
	wi_string_t 			*path;

	if(!bot->watchers)
		return;

	enumerator = wi_array_data_enumerator(bot->watchers);

	while((watcher = wi_enumerator_next_data(enumerator))) {
		_wb_bot_subscribe_to_remote_directory_for_watcher(bot, watcher);
	}
}

void wb_bot_unsubscribe_watchers(wb_bot_t *bot) {
	wi_enumerator_t			*enumerator;
	wb_watcher_t 			*watcher;
	wi_string_t 			*path;

	if(!bot->watchers)
		return;

	enumerator = wi_array_data_enumerator(bot->watchers);

	while((watcher = wi_enumerator_next_data(enumerator))) {
		_wb_bot_unsubscribe_to_remote_directory_for_watcher(bot, watcher);
	}
}

wb_watcher_t * wb_bot_watcher_for_path(wb_bot_t *bot, wi_string_t *path) {
	wi_enumerator_t			*enumerator;
	wb_watcher_t 			*watcher;

	if(!bot->watchers)
		return NULL;

	enumerator = wi_array_data_enumerator(bot->watchers);

	while((watcher = wi_enumerator_next_data(enumerator))) {
		if(wi_is_equal(wb_watcher_path(watcher), path)) {
			return watcher;
		}
	}
	return NULL;
}




wi_array_t * wb_bot_outputs_for_message(wb_bot_t *bot, wr_user_t *user, wi_p7_message_t *message) {
	
	wi_enumerator_t			*rules_enumerator, *inputs_enumerator, *outputs_enumerator;
	wi_mutable_array_t 		*results;
	wi_string_t 			*message_name, *message_input;
	wb_rule_t 				*rule;
	wb_input_t 				*input;
	wb_output_t 			*output;

	rules_enumerator 	= wi_array_data_enumerator(bot->rules);
	results				= wi_mutable_array();

	while((rule = wi_enumerator_next_data(rules_enumerator))) {

		if(wb_rule_is_activated(rule) && wb_bot_check_rule_permissions(user, rule)) {
			inputs_enumerator = wi_array_data_enumerator(wb_rule_inputs(rule));

			input = NULL;

			while((input = wi_enumerator_next_data(inputs_enumerator))) {
				message_name = wi_p7_message_name(message);

				if(wi_is_equal(message_name, wb_input_message_name(input))) {

					message_input = wb_bot_input_for_message(message);

					if(wb_bot_check_input_match(input, message_input)) {

						outputs_enumerator = wi_array_data_enumerator(wb_rule_outputs(rule));

						//wi_log_info(wi_description(input));

						while(output = wi_enumerator_next_data(outputs_enumerator)) {
							wb_output_set_input_text(output, wb_input_input(input));
							wi_mutable_array_add_data(results, output);
						}
						
						return results;
					}
				}
			}	
		}
	}
	return NULL;
}
 
wb_command_t * wb_bot_command_for_message(wb_bot_t *bot, wr_user_t *user, wi_p7_message_t *message) {
	wi_enumerator_t			*enumerator;
	wi_mutable_array_t 		*results;
	wi_string_t 			*message_name, *message_input, *command_name;
	wb_command_t 			*command;
	wi_array_t 				*args;

	enumerator 			= wi_array_data_enumerator(bot->commands);
	results				= wi_mutable_array();

	args 			= _wb_bot_decompose_bot_command_arguments(message);

	if(!args)
		return NULL;

	message_input	= wi_array_data_at_index(args, 0);

	while((command = wi_enumerator_next_data(enumerator)))
		if(wb_command_is_activated(command)) {

			command_name = wi_string_with_format(WI_STR("!%@"), wb_command_name(command));

			if(wi_is_equal(message_input, command_name))
				if(wb_bot_check_command_permissions(user, command))
					return command;	
		}
	
	return NULL;
}




#pragma mark -

wi_boolean_t wb_bot_check_rule_permissions(wr_user_t *user, wb_rule_t *rule) {
	wi_enumerator_t			*enumerator;
	wi_string_t 			*permissions, *permission, *nick, *login;
	wi_array_t 				*components;

	permissions 	= wb_rule_permissions(rule);
	components 		= wi_string_components_separated_by_string(permissions, WI_STR(","));

	if(!components)
		return true;

	enumerator 		= wi_array_data_enumerator(components);

	while((permission = wi_enumerator_next_data(enumerator))) {
		if(wi_is_equal(permission, WI_STR("any")))
			return true;

		if (wi_string_contains_string(wr_user_nick(user), permission, WI_STRING_CASE_INSENSITIVE) ||
			wi_is_equal(wr_user_login(user), permission))
			return true;
	}	
	return false;
}

wi_boolean_t wb_bot_check_command_permissions(wr_user_t *user, wb_command_t *command) {
	wi_enumerator_t			*enumerator;
	wi_string_t 			*permissions, *permission, *nick, *login;
	wi_array_t 				*components;

	permissions 	= wb_command_permissions(command);
	components 		= wi_string_components_separated_by_string(permissions, WI_STR(","));

	if(!components)
		return true;

	enumerator 		= wi_array_data_enumerator(components);

	while((permission = wi_enumerator_next_data(enumerator))) {
		if(wi_is_equal(permission, WI_STR("any")))
			return true;

		if (wi_string_contains_string(wr_user_nick(user), permission, WI_STRING_CASE_INSENSITIVE) ||
			wi_is_equal(wr_user_login(user), permission))
			return true;
	}	
	return false;
}


#pragma mark -

wi_boolean_t wb_bot_check_input_match(wb_input_t *input, wi_string_t *message_input) {

	// specific and critical case for empty string on chat events: join and leave
	if(!message_input)
		return true;

	if(wi_string_length(message_input) == 0)
		return true;

	if(wb_input_input(input) && (message_input || wi_string_length(message_input) == 0)) {
		wi_string_options_t option = wb_input_is_case_sensitive(input) ? 0 : WI_STRING_SMART_CASE_INSENSITIVE;

		switch(wb_input_comparison(input)) {
			case WB_EQUALS: {
				if(wb_input_is_case_sensitive(input))
		 			return (wi_string_compare(wb_input_input(input), message_input) == 0); 
		 		else
		 			return (wi_string_case_insensitive_compare(wb_input_input(input), message_input) == 0);

		 	} break;

			case WB_CONTAINS: {
		 		return wi_string_contains_string(message_input, wb_input_input(input), option);
			} break;

			case WB_STARTS_WITH: {
			 	return wi_string_range_of_string(wb_input_input(input), message_input, 0).location != WI_NOT_FOUND;
			} break;
			
			case WB_ENDS_WITH: {
			 	return wi_string_range_of_string(wb_input_input(input), message_input, WI_STRING_BACKWARDS).location != WI_NOT_FOUND; 
			} break;

			default: return true; break;
		}
	} else {
		return true;
	}

	return false;
}

wi_string_t * wb_bot_input_for_message(wi_p7_message_t *message) {
	wi_string_t 	* result;

	if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.chat.say"))) {
		result = wi_p7_message_string_for_name(message, WI_STR("wired.chat.say"));

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.chat.me"))) {
		result = wi_p7_message_string_for_name(message, WI_STR("wired.chat.me"));

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.message.message"))) {
		result = wi_p7_message_string_for_name(message, WI_STR("wired.message.message"));

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.message.message"))) {
		result = wi_p7_message_string_for_name(message, WI_STR("wired.message.message"));

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.chat.user_join"))) {
		result = WI_STR("");

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.chat.user_leave"))) {
		result = WI_STR("");

	} else {
		result = NULL;
	}

	return result;
}

#pragma mark -

wb_output_t * wb_bot_select_random_output(wi_array_t *outputs) {

	int random_index;

	srand(time(NULL));

	if(!outputs || wi_array_count(outputs) == 0)
		return NULL;

	random_index = rand() % wi_array_count(outputs);

	if(random_index < 0)
		return NULL;

	return wi_array_data_at_index(outputs, random_index);
}




#pragma mark -

wi_boolean_t wb_bot_execute_output(wb_output_t *output, wr_user_t *user) {
	
	int 				i, repeat, delay;
	wi_string_t *		output_string;

	if(!output)
		return false;

	if(wb_output_output(output) == NULL)
		return false;

	wi_log_info(WI_STR("Output: %@"), wb_output_output(output));

	i 					= 0;
	repeat 				= wb_output_repeat(output);
	delay				= wb_output_delay(output);

	if(repeat < 1)
		repeat = 1;

	for(i = 0; i < repeat; i++) {
		if(delay > 0)
			sleep(delay);

		output_string 	= wb_output_wire_command_string(output, user);

		wr_commands_parse_command(output_string, wb_output_is_chat(output));
	}
}


wi_boolean_t wb_bot_execute_command(wb_bot_t *bot, wb_command_t *command, wi_string_t *arguments, wr_user_t *user, wi_p7_message_t *message) {
	wi_string_t * 		command_name;
	wi_array_t *		outputs;
	wb_output_t *		output;
	
	command_name 	= wb_command_name(command);
	outputs 		= wb_command_outputs(command);
	output 			= wi_null();

	if(outputs && wi_array_count(outputs) > 0) {
		output		= wb_bot_select_random_output(outputs);
		wb_output_set_message_name(output, wi_p7_message_name(message));
	}	

	if(wi_is_equal(command_name, WI_STR("reload"))) {
		if(wb_bot_reload_configuration(bot)) {
			
			if(output) 
				wb_bot_execute_output(output, user);

			return true;
		} else {
			wr_commands_parse_command(WI_STR("/me failed to reload the dictionary..."), true);
			return false;
		}

	} else if(wi_is_equal(command_name, WI_STR("start"))) {
		wb_bot_start_command(bot);

		if(output)
		 wb_bot_execute_output(output, user);

		return true;

	} else if(wi_is_equal(command_name, WI_STR("stop"))) {

		if(output) 
			wb_bot_execute_output(output, user);

		wb_bot_sleep_command(bot);
		wb_bot_stop_command(bot);
		return true;

	} else if(wi_is_equal(command_name, WI_STR("nick"))) {
		wb_bot_nick_command(bot, arguments);

		if(output != NULL) 
			wb_bot_execute_output(output, user);

		return true;

	} else if(wi_is_equal(command_name, WI_STR("status"))) {
		wb_bot_status_command(bot, arguments);

		if(output != NULL) 
			wb_bot_execute_output(output, user);

		return true;

	} else if(wi_is_equal(command_name, WI_STR("sleep"))) {

		if(output) 
			wb_bot_execute_output(output, user);

		wb_bot_sleep_command(bot);
		return true;

	} else if(wi_is_equal(command_name, WI_STR("help"))) {
		wb_bot_help_command(bot, message, user);

		return true;

	} else {
		if(output) 
			wb_bot_execute_output(output, user);
	}

	return false;
}





#pragma mark -

void wb_bot_log_rule_input(wb_rule_t *rule, wb_input_t *input) {

}


void wb_bot_log_rule_output(wb_rule_t *rule, wb_output_t *output) {
	
}


void wb_bot_log_command(wb_command_t *command) {
	
}




#pragma mark -

wi_boolean_t wb_bot_reload_configuration(wb_bot_t *bot) {
	wi_string_t 	* old_path, * command;
	wi_boolean_t	loaded;
	
	loaded 	= false;

	// keep the original XML dictionary path
	old_path 		= wi_copy(bot->path);

	// release old attributes of the bot
	wi_release(bot->path);
	wi_release(bot->xml);
	wi_release(bot->commands);
	wi_release(bot->rules);

	wb_bot_unsubscribe_watchers(bot);

	// init again
	bot->path					= wi_retain(old_path);
	bot->commands				= wi_array_init(wi_mutable_array_alloc());
	bot->rules					= wi_array_init(wi_mutable_array_alloc());

	wi_release(old_path);

	// reload dictionnary
	loaded = _wb_bot_load_file(bot, old_path);

	wr_client_reload_icon();

	//reload icon
	if(wi_fs_path_exists(wr_icon_path, false)) {
		command = wi_string_with_format(WI_STR("/icon %@"), wr_icon_path);
		wr_commands_parse_command(command, false);
	}

	if(loaded) {
		wb_bot_subscribe_watchers(bot);
	}

	return loaded;
}


void wb_bot_start_command(wb_bot_t *bot) {
	bot->started = true;
}


void wb_bot_stop_command(wb_bot_t *bot) {
	bot->started = false;
}


void wb_bot_sleep_command(wb_bot_t *bot) {
	wi_p7_message_t * 		message;
	wr_user_t *				user;

	user = wr_chat_user_with_uid(wr_public_chat, wb_user_id);

	if(user) {
		message = wi_p7_message_with_name(WI_STR("wired.user.set_idle"), wr_p7_spec);
		wi_p7_message_set_bool_for_name(message, true, WI_STR("wired.user.idle"));

		wr_client_send_message(message);
	}
}


void wb_bot_nick_command(wb_bot_t *bot, wi_string_t *arguments) {
	wi_string_t 		 	*command;

	if(arguments != NULL && wi_string_length(arguments) > 0) {
		command = wi_string_with_format(WI_STR("/nick %@"), arguments);
		wr_commands_parse_command(command, false);
	}
}


void wb_bot_status_command(wb_bot_t *bot, wi_string_t *arguments) {
	wi_string_t 		 	*command;

	if(arguments != NULL) {
		command = wi_string_with_format(WI_STR("/status %@"), arguments);
		wr_commands_parse_command(command, false);
	}
}


void wb_bot_help_command(wb_bot_t *bot, wi_p7_message_t *message, wr_user_t *user) {
	wi_string_t * 		help_string;
	wb_output_t * 		output = NULL;

	output = wb_output_init_with_message_name(wb_output_alloc(), wi_p7_message_name(message));

	help_string = WI_STR("Wirebot Help:\n\n"
		" \n"
		"Wirebot responds to chat commands to execute specific\n"
		"features. A chat command starts with a '!' characters\n"
		"followed by a keyword. See the list of available chat\n" 
		"commands below:\n" 
		" \n"
		"!start           : Start the bot\n"
		"!stop            : Stop the bot\n"
		"!sleep           : Make the bot idle\n"
		"!reload          : Reload the dictionary\n"
		"!nick [nickname] : Change the nick of the bot\n"
		"!status [status] : Change the status of the bot\n"
		"!help            : Show this help message\n\n"
		);

	wb_output_set_output(output, help_string);
	wb_bot_execute_output(output, user);

	// need a fix, it's weird
	if(output)
		wi_release(output);
}



void wb_bot_command_reply_error(wi_string_t *command, wi_string_t *error) {

}




#pragma mark -

static wi_array_t * _wb_bot_decompose_bot_command_arguments(wi_p7_message_t *message) {
	wi_array_t 				* arguments;
	wi_string_t 			* message_input;

	if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.chat.say"))) {
		message_input 	= wi_p7_message_string_for_name(message, WI_STR("wired.chat.say"));

	} else if(wi_is_equal(wi_p7_message_name(message), WI_STR("wired.message.message"))) {
		message_input = wi_p7_message_string_for_name(message, WI_STR("wired.message.message"));

	} else {
		return NULL;
	}

	return wi_string_components_separated_by_string(message_input, WI_STR(" "));
}


static wi_string_t * _wb_bot_recompose_bot_command_arguments(wi_array_t *arguments) {
	wi_mutable_string_t		*string;
	int 					i;

	string = wi_mutable_string();

	if(wi_array_count(arguments) <= 1)
		return string;

	for(i = 1; i < wi_array_count(arguments); i++) {
		wi_mutable_string_append_format(string, WI_STR("%@"), wi_array_data_at_index(arguments, i));

		if(i < wi_array_count(arguments)-1)
			wi_mutable_string_append_string(string, WI_STR(" "));
	}

	return string;
}


static void _wb_bot_split_command(wi_string_t *buffer, wi_string_t **out_command, wi_string_t **out_arguments) {
	wi_uinteger_t	index;
	
	index = wi_string_index_of_string(buffer, WI_STR(" "), 0);
	
	if(index != WI_NOT_FOUND) {
		*out_command	= wi_string_substring_to_index(buffer, index);
		*out_arguments	= wi_string_substring_from_index(buffer, index + 1);
	} else {
		*out_command	= wi_autorelease(wi_copy(buffer));
		*out_arguments	= wi_string();
	}
	
	if(wi_string_has_prefix(*out_command, WI_STR("!")))
		*out_command = wi_string_by_deleting_characters_to_index(*out_command, 1);
}



static wi_boolean_t _wb_bot_load_file(wb_bot_t *bot, wi_string_t *path) {
	xmlDocPtr	doc;
	xmlChar		*buffer;
	int			length;
	
	doc = xmlReadFile(wi_string_cstring(path), NULL, 0);
	
	if(!doc) {
		wi_log_error(WI_STR("Can't read robot file: %@"), path);
		return false;
	}

	if(!_wb_bot_load_wirebot(bot, doc)) {
		xmlFreeDoc(doc);
		return false;
	}
	
	xmlDocDumpMemory(doc, &buffer, &length);
	
	bot->xml = wi_string_init_with_bytes(wi_string_alloc(), (const char *) buffer, length);
	
	xmlFreeDoc(doc);
	xmlFree(buffer);

	return true;
}

static wi_boolean_t _wb_bot_load_wirebot(wb_bot_t *bot, xmlDocPtr doc) {
	xmlNodePtr		root_node, node, next_node;

	root_node = xmlDocGetRootElement(doc);
	
	if(strcmp((const char *) root_node->name, "wirebot") != 0) {
		wi_error_set_libwired_error_with_format(WI_ERROR_P7_INVALIDSPEC,
			WI_STR("Expected \"wirebot\" node but got \"%s\""),
			root_node->name);
		
		return false;
	}

	wi_log_info(WI_STR("Reading %@ robot file..."), bot->path);

	for(node = root_node->children; node != NULL; node = next_node) {
		next_node = node->next;
		
		if(node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) node->name, "rules") == 0) {
				if(!_wb_bot_load_rules(bot, node))
					return false;
			}
			else if(strcmp((const char *) node->name, "commands") == 0) {
				if(!_wb_bot_load_commands(bot, node))
					return false;
			}
			else if(strcmp((const char *) node->name, "watchers") == 0) {
				if(!_wb_bot_load_watchers(bot, node))
					return false;
			}
		}
	}
	
	return true;
}

static wi_boolean_t _wb_bot_load_rules(wb_bot_t *bot, xmlNodePtr node) {
	wi_string_t 			*string;
	wb_rule_t 				*rule;
	xmlNodePtr				sub_node, next_node;
	
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) sub_node->name, "rule") != 0) {
				return false;
			}

			rule = wb_rule_init(wb_rule_alloc(), sub_node);

			if(!rule)
				return false;

			wi_mutable_array_add_data(bot->rules, rule);
		}
	}
	return true;
}

static wi_boolean_t _wb_bot_load_commands(wb_bot_t *bot, xmlNodePtr node) {
	wi_string_t 			*string;
	wb_command_t 			*command;
	xmlNodePtr				sub_node, next_node;
	
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) sub_node->name, "command") != 0)
				return false;

			command = wb_command_init(wb_command_alloc(), sub_node);
			if(!command)
				return false;

			wi_mutable_array_add_data(bot->commands, command);
		}
	}
	
	return true;
}

static wi_boolean_t _wb_bot_load_watchers(wb_bot_t *bot, xmlNodePtr node) {
	wi_string_t 			*string;
	wb_watcher_t 			*watcher;
	xmlNodePtr				sub_node, next_node;
	
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) sub_node->name, "watcher") != 0)
				return false;

			watcher = wb_watcher_init(wb_watcher_alloc(), sub_node);
			if(!watcher)
				return false;

			wi_mutable_array_add_data(bot->watchers, watcher);
		}
	}
	
	return true;
}






#pragma mark -

void _wb_bot_subscribe_to_remote_directory_for_watcher(wb_bot_t *bot, wb_watcher_t *watcher) {
	wi_p7_message_t     *message;
	wi_string_t 		*path;

	bot->subscribing = true;

	path = wb_watcher_path(watcher);

	wi_log_info(WI_STR("Watcher subscribe to directory: %@"), path);

	message = wi_p7_message_with_name(WI_STR("wired.file.list_directory"), wr_p7_spec);
	wi_p7_message_set_string_for_name(message, path, WI_STR("wired.file.path"));

	if(wr_connected)
    	wr_client_send_message(message);

    message = wi_p7_message_with_name(WI_STR("wired.file.subscribe_directory"), wr_p7_spec);
    wi_p7_message_set_string_for_name(message, path, WI_STR("wired.file.path"));

	if(wr_connected)
    	wr_client_send_message(message);
}

void _wb_bot_unsubscribe_to_remote_directory_for_watcher(wb_bot_t *bot, wb_watcher_t *watcher) {
	wi_p7_message_t     *message;
	wi_string_t 		*path;

	path = wb_watcher_path(watcher);

	wi_log_info(WI_STR("Watcher unsubscribe to directory: %@"), path);

    message = wi_p7_message_with_name(WI_STR("wired.file.unsubscribe_directory"), wr_p7_spec);
    wi_p7_message_set_string_for_name(message, path, WI_STR("wired.file.path"));

    if(wr_connected)
    	wr_client_send_message(message);
}




#pragma mark -
#pragma mark bot accessor methods

wi_string_t * wb_bot_path(wb_bot_t *bot) {
	return bot->path;
}

wi_array_t * wb_bot_commands(wb_bot_t * bot) {
	return bot->commands;
}

wi_array_t * wb_bot_rules(wb_bot_t * bot) {
	return bot->rules;
}

wi_boolean_t wb_bot_is_subscribing(wb_bot_t * bot) {
	return bot->subscribing;
}

wi_boolean_t wb_bot_set_subscribing(wb_bot_t *bot, wi_boolean_t sub) {
	bot->subscribing = sub;
}



#pragma mark -
#pragma mark bot runtime methods

static void wb_bot_dealloc(wi_runtime_instance_t *instance) {
	wb_bot_t		*bot = instance;
	
	wi_release(bot->path);
	wi_release(bot->commands);
	wi_release(bot->rules);
	wi_release(bot->watchers);
	wi_release(bot->xml);
}

static wi_string_t * wb_bot_description(wi_runtime_instance_t *instance) {
	wb_bot_t		*bot = instance;

	return bot->xml;
}

static wi_hash_code_t wb_bot_hash(wi_runtime_instance_t *instance) {	
	return wi_string_length(wb_bot_description(instance));
}



