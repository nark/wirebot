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


#include "output.h"
#include "bot.h"
#include "client.h"



#define WB_TIME_RANGE(s) ({ \
    wb_bot_time_range_t 							result = (WB_EVERY_TIME); \
    if(wi_is_equal(s, WI_STR("every"))) 			result = (WB_EVERY_TIME); \
  	else if(wi_is_equal(s, WI_STR("morning"))) 		result = (WB_MORNING_TIME); \
  	else if(wi_is_equal(s, WI_STR("afternoon"))) 	result = (WB_AFTERNOON_TIME); \
  	else if(wi_is_equal(s, WI_STR("evening"))) 		result = (WB_EVENING_TIME); \
  	else if(wi_is_equal(s, WI_STR("night"))) 		result = (WB_NIGHT_TIME); \
  	else 											result = (WB_EVERY_TIME) ; \
    result; \
})





 struct _wb_output {
	wi_runtime_base_t				base;

	wi_string_t						*message_name;
	wi_string_t						*input_text;
	wi_string_t						*output;
	wb_bot_time_range_t				time;
	wi_integer_t					delay;
	wi_integer_t					repeat;
};  

static void							wb_output_dealloc(wi_runtime_instance_t *);
static wi_string_t * 				wb_output_description(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_output_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_output_runtime_class = {
	"wb_output_t",
	wb_output_dealloc,
	NULL,
	NULL,
	wb_output_description,
	NULL
};



static wb_output_t*					_wb_bot_load_output_with_node(wb_output_t *, xmlNodePtr);





#pragma mark -

wb_output_t * wb_output_alloc(void) {
	return wi_runtime_create_instance(wb_output_runtime_id, sizeof(wb_output_t));
}



wb_output_t * wb_output_init(wb_output_t *output, xmlNodePtr node) {
	return _wb_bot_load_output_with_node(output, node);
}


wb_output_t * wb_output_init_with_message(wb_output_t *output, wi_p7_message_t *message) {
	output->message_name = wi_retain(wi_p7_message_name(message));

	return output;
}




#pragma mark -

wi_string_t * wb_output_wire_command_string(wb_output_t *output, wr_user_t *user) {
	wi_string_t 	* result, *nick, *string;

	nick 			= wr_user_nick(user);

	if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.chat.say"))) {
		string = wi_string_with_format(WI_STR("%@"), wb_output_output(output));

	} else if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.chat.me"))) {
		string = wi_string_with_format(WI_STR("/me %@"), wb_output_output(output));

	} else if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.message.message"))) {
		string = wi_string_with_format(WI_STR("/msg %@ %@"), nick, wb_output_output(output));

	} else if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.message.broadcast"))) {
		string = wi_string_with_format(WI_STR("/broadcast %@"), wb_output_output(output));

	} else {
		string = wi_string_with_format(WI_STR("%@"), wb_output_output(output));

	}

	string = wi_string_by_replacing_string_with_string(string, WB_BOT_NICK, wr_nick, WI_STRING_SMART_CASE_INSENSITIVE);
	string = wi_string_by_replacing_string_with_string(string, WB_INPUT_NICK, nick, WI_STRING_SMART_CASE_INSENSITIVE);

	if(wb_output_input_text(output))
		string = wi_string_by_replacing_string_with_string(string, WB_INPUT_TEXT, wb_output_input_text(output), WI_STRING_SMART_CASE_INSENSITIVE);

	return string;
}


wi_boolean_t wb_output_is_chat(wb_output_t *output) {
	if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.chat.say")))
		return true;

	if(wi_is_equal(wb_output_message_name(output), WI_STR("wired.chat.me")))
		return true;

	return false;
}



#pragma mark -

wi_string_t * wb_output_message_name(wb_output_t *output) {
	return output->message_name;
}

void wb_output_set_message_name(wb_output_t *output, wi_string_t *string) {
	if(output->message_name)
		wi_release(output->message_name);

	output->message_name = wi_retain(string);
}



wi_string_t * wb_output_input_text(wb_output_t *output) {
	return output->input_text;
}

void wb_output_set_input_text(wb_output_t *output, wi_string_t *string) {
	if(output->input_text)
		wi_release(output->input_text);

	output->input_text = wi_retain(string);
}



wi_string_t *  wb_output_output(wb_output_t *output) {
	return output->output;
}

void wb_output_set_output(wb_output_t *output, wi_string_t *string) {
	if(output->output)
		wi_release(output->output);

	output->output = wi_retain(string);
}




wb_bot_time_range_t wb_output_time(wb_output_t *output) {
	return output->time;
}


wi_integer_t wb_output_delay(wb_output_t *output) {
	return output->delay;
}


wi_integer_t wb_output_repeat(wb_output_t *output) {
	return output->repeat;
}





#pragma mark - 

static wb_output_t * _wb_bot_load_output_with_node(wb_output_t *output, xmlNodePtr node) {

	wi_string_t * message_name, *output_string, *time, *delay, *repeat;

	message_name = wi_xml_node_attribute_with_name(node, WI_STR("message"));
	if(message_name)
		wb_output_set_message_name(output, message_name);
	
	output_string =	wi_xml_node_content(node);
	if(output_string)
		wb_output_set_output(output, output_string);

	time = wi_xml_node_attribute_with_name(node, WI_STR("time"));
	if(time)
		output->time = WB_TIME_RANGE(time);

	delay = wi_xml_node_attribute_with_name(node, WI_STR("delay"));
	if(delay)
		output->delay = wi_string_integer(delay);

	repeat = wi_xml_node_attribute_with_name(node, WI_STR("repeat"));
	if(repeat)
		output->repeat = wi_string_integer(repeat);

	return output;
}





#pragma mark -

static void wb_output_dealloc(wi_runtime_instance_t *instance) {
	wb_output_t			*output = instance;

	wi_release(output->message_name);
	wi_release(output->input_text);
	wi_release(output->output);
}

static wi_string_t * wb_output_description(wi_runtime_instance_t *instance) {
	wb_output_t			* output = instance;
	wi_string_t 		* string;

	string = wi_string_with_format(WI_STR("Output: [%@] -> %@"), output->message_name, output->output);

	return string;
}


