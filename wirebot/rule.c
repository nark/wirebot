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


#include "rule.h"
#include "input.h"
#include "output.h"
#include <string.h>

struct _wb_rule {
	wi_runtime_base_t				base;

	wi_boolean_t					activated;
	wi_string_t 					*permissions;
	wi_mutable_array_t				*inputs;
	wi_mutable_array_t				*outputs;
};  


static void							wb_rule_dealloc(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_rule_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_rule_runtime_class = {
	"wb_rule_t",
	wb_rule_dealloc,
	NULL,
	NULL,
	NULL,
	NULL
};


static wb_rule_t*					_wb_rule_load_with_node(wb_rule_t *, xmlNodePtr);



#pragma mark -

void wb_rules_init(void) {
	wb_rule_runtime_id = wi_runtime_register_class(&wb_rule_runtime_class);
}





#pragma mark -

wb_rule_t * wb_rule_alloc(void) {
	return wi_runtime_create_instance(wb_rule_runtime_id, sizeof(wb_rule_t));
}




#pragma mark -

	wb_rule_t * wb_rule_init(wb_rule_t *rule, xmlNodePtr node) {
	
	rule->permissions 	= wi_retain(WI_STR("any"));
	rule->activated 	= true;
	rule->inputs 		= wi_array_init(wi_mutable_array_alloc());
	rule->outputs 		= wi_array_init(wi_mutable_array_alloc());

	return _wb_rule_load_with_node(rule, node);
}



#pragma mark - 

wi_boolean_t wb_rule_is_activated(wb_rule_t *rule) {
	return rule->activated;
}

wi_string_t * wb_rule_permissions(wb_rule_t *rule) {
	return rule->permissions;
}

wi_mutable_array_t * wb_rule_inputs(wb_rule_t *rule) {
	return rule->inputs;
}

wi_mutable_array_t * wb_rule_outputs(wb_rule_t *rule) {
	return rule->outputs;
}





#pragma mark -

static wb_rule_t * _wb_rule_load_with_node(wb_rule_t *rule, xmlNodePtr node) {
	wi_string_t 			*permissions, *activated;
	wb_input_t 				*input;
	wb_output_t 			*output;
	xmlNodePtr				sub_node, next_node;
	
	// get rule permissions
	permissions = wi_xml_node_attribute_with_name(node, WI_STR("permissions"));
	if(permissions)
		rule->permissions = wi_retain(permissions);
		
	// is an activated rule ?
	activated = wi_xml_node_attribute_with_name(node, WI_STR("activated"));
	if(activated)
		rule->activated = wi_is_equal(activated, WI_STR("true")) ? true : false;

	// get rule children: inputs and outputs 
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) sub_node->name, "input") == 0) {
				
				input = wb_input_init(wb_input_alloc(), sub_node);
				if(input)
					wi_mutable_array_add_data(rule->inputs, input);

				wi_release(input);

			} else if(strcmp((const char *) sub_node->name, "output") == 0) {
				
				output = wb_output_init(wb_output_alloc(), sub_node);
				if(output)
					wi_mutable_array_add_data(rule->outputs, output);

				wi_release(output);
			}
		}
	}
	return rule;
}

static void wb_rule_dealloc(wi_runtime_instance_t *instance) {
	wb_rule_t		*rule = instance;

	wi_release(rule->permissions);
	wi_release(rule->inputs);
	wi_release(rule->outputs);
}






