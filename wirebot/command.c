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


#include "command.h"
#include "output.h"
#include <string.h>


struct _wb_command {
	wi_runtime_base_t				base;

	wi_boolean_t					activated;
	wi_string_t						*name;
	wi_string_t						*permissions;

	wi_mutable_array_t				*outputs;
};  

static void							wb_command_dealloc(wi_runtime_instance_t *);
static wi_string_t *				wb_command_description(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_command_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_command_runtime_class = {
	"wb_command_t",
	wb_command_dealloc,
	NULL,
	NULL,
	wb_command_description,
	NULL
};



static wb_command_t * 				_wb_command_load_with_node(wb_command_t *, xmlNodePtr);




#pragma mark -

void wb_commands_init(void) {
	wb_command_runtime_id = wi_runtime_register_class(&wb_command_runtime_class);
}






#pragma mark -

wb_command_t * wb_command_alloc(void) {
	return wi_runtime_create_instance(wb_command_runtime_id, sizeof(wb_command_t));
}


wb_command_t * wb_command_init(wb_command_t *command, xmlNodePtr node) {
	
	command->activated 		= true;
	command->name 			= wi_retain(WI_STR("help"));
	command->permissions 	= wi_retain(WI_STR("any"));
	command->outputs 		= wi_array_init(wi_mutable_array_alloc());

	return _wb_command_load_with_node(command, node);
}





#pragma mark -

wi_string_t * wb_command_name(wb_command_t * command) {
	return command->name;
}


wi_boolean_t wb_command_is_activated(wb_command_t * command) {
	return command->activated;
}


wi_string_t * wb_command_permissions(wb_command_t * command) {
	return command->permissions;
}


wi_mutable_array_t * wb_command_outputs(wb_command_t * command) {
	return command->outputs;
}





#pragma mark -

static wb_command_t * _wb_command_load_with_node(wb_command_t *command, xmlNodePtr node) {
	wi_string_t 			*permissions, *activated, *name;
	wb_output_t 			*output;
	xmlNodePtr				sub_node, next_node;
	
	// get command name
	name = wi_xml_node_attribute_with_name(node, WI_STR("name"));
	if(name)
		command->name = wi_retain(name);

	// get command permissions
	permissions = wi_xml_node_attribute_with_name(node, WI_STR("permissions"));
	if(permissions)
		command->permissions = wi_retain(permissions);
		
	// is an activated command ?
	activated = wi_xml_node_attribute_with_name(node, WI_STR("activated"));
	if(activated)
		command->activated = wi_is_equal(activated, WI_STR("true")) ? true : false;

	// get rule children: outputs 
	for(sub_node = node->children; sub_node != NULL; sub_node = next_node) {
		next_node = sub_node->next;
		
		if(sub_node->type == XML_ELEMENT_NODE) {

			if(strcmp((const char *) sub_node->name, "output") == 0) {
				
				output = wb_output_init(wb_output_alloc(), sub_node);
				if(output)
					wi_mutable_array_add_data(command->outputs, output);

				wi_release(output);
			}
		}
	}

	return command;
}




#pragma mark -

static void wb_command_dealloc(wi_runtime_instance_t *instance) {
	wb_command_t		*command = instance;

	wi_release(command->permissions);
	wi_release(command->name);
	wi_release(command->outputs);
}


static wi_string_t * wb_command_description(wi_runtime_instance_t *instance) {
	wb_command_t		* command = instance;
	wi_string_t 		* string;

	string = wi_string_with_format(WI_STR("Command: [!%@] -> %@"), command->name, command->permissions);

	return string;
}

