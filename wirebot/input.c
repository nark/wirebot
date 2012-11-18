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


#include "input.h"



#define WB_COMPARISON_EQUIVALENT(s) ({ \
    wb_bot_comparison_method_t 						result = (WB_NOT_EQUALS); \
    if(wi_is_equal(s, WI_STR("equals"))) 			result = (WB_EQUALS); \
  	else if(wi_is_equal(s, WI_STR("contains"))) 	result = (WB_CONTAINS); \
  	else if(wi_is_equal(s, WI_STR("starts"))) 		result = (WB_STARTS_WITH); \
  	else if(wi_is_equal(s, WI_STR("ends"))) 		result = (WB_ENDS_WITH); \
  	else 											result = (WB_NOT_EQUALS) ; \
    result; \
})




struct _wb_input {
	wi_runtime_base_t				base;

	wi_string_t						*message_name;
	wi_string_t						*input;
	wb_bot_comparison_method_t		comparison;
	wi_boolean_t					case_sensitive;
};  

static void							wb_input_dealloc(wi_runtime_instance_t *);
static wi_string_t *				wb_input_description(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_input_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_input_runtime_class = {
	"wb_input_t",
	wb_input_dealloc,
	NULL,
	NULL,
	wb_input_description,
	NULL
};


static wb_input_t *				 	_wb_bot_load_input_with_node(wb_input_t *, xmlNodePtr);





#pragma mark -

wb_input_t * wb_input_alloc(void) {
	return wi_runtime_create_instance(wb_input_runtime_id, sizeof(wb_input_t));
}


wb_input_t * wb_input_init(wb_input_t *input, xmlNodePtr node) {
	
	input->case_sensitive 	= false;
	input->comparison 		= WB_EQUALS;

	return _wb_bot_load_input_with_node(input, node);
}




#pragma mark -

wi_string_t * wb_input_message_name(wb_input_t *input) {
	return input->message_name;
}

wi_string_t * wb_input_input(wb_input_t *input) {
	return input->input;
}

wb_bot_comparison_method_t wb_input_comparison(wb_input_t *input) {
	return input->comparison;
}

wi_boolean_t wb_input_is_case_sensitive(wb_input_t *input) {
	return input->case_sensitive;
}






#pragma mark -

static wb_input_t * _wb_bot_load_input_with_node(wb_input_t *input, xmlNodePtr node) {
	
	wi_string_t		*message_name, *input_string, *comparison, *sensitive;

	message_name = wi_xml_node_attribute_with_name(node, WI_STR("message"));
	if(message_name)
		input->message_name = wi_retain(message_name);

	comparison = wi_xml_node_attribute_with_name(node, WI_STR("comparison"));
	if(comparison)
		input->comparison = WB_COMPARISON_EQUIVALENT(comparison);

	sensitive = wi_xml_node_attribute_with_name(node, WI_STR("sensitive"));
	if(sensitive)
		input->case_sensitive = wi_is_equal(sensitive, WI_STR("true"));

	input_string =	wi_xml_node_content(node);
	if(input_string)
		input->input = wi_retain(input_string);

	return input;
}





#pragma mark -

static void wb_input_dealloc(wi_runtime_instance_t *instance) {
	wb_input_t			*input = instance;

	wi_release(input->message_name);
	wi_release(input->input);
}

static wi_string_t * wb_input_description(wi_runtime_instance_t *instance) {
	wb_input_t			* input = instance;
	wi_string_t 		* string;

	string = wi_string_with_format(WI_STR("Input: [%@] -> %@"), input->message_name, input->input);

	return string;
}
