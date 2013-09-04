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

#ifndef WR_BOT_H
#define WR_BOT_H 1

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <wired/wired.h>

#include "users.h"
#include "rule.h"
#include "input.h"
#include "output.h"
#include "command.h"
#include "watcher.h"

#define WB_BOT_NICK 				WI_STR("@BOT_NICK")
#define WB_INPUT_NICK 				WI_STR("@INPUT_NICK")
#define WB_INPUT_TEXT 				WI_STR("@INPUT_TEXT")



/**
 * Wirebot object types
 */
typedef struct _wb_bot				wb_bot_t;

/* Initializer */
void 								wb_bot_initialize(void);
void								wb_bot_init(void);


/* Allocators */
wb_bot_t *							wb_bot_alloc(void);


/* Constructors */
wb_bot_t *							wb_bot_init_with_file(wb_bot_t *, wi_string_t *);


/* Accessors */
wi_string_t *						wb_bot_path(wb_bot_t *);
wi_array_t *						wb_bot_commands(wb_bot_t *);
wi_array_t *						wb_bot_rules(wb_bot_t *);

wi_boolean_t						wb_bot_is_subscribing(wb_bot_t *);
wi_boolean_t						wb_bot_set_subscribing(wb_bot_t *, wi_boolean_t);

/* Bot Engine */
wi_boolean_t						wb_bot_dispatch_message(wb_bot_t *, wi_p7_message_t *);
void								wb_bot_apply_settings(wi_set_t *);


/* Bot API
/* The following methods could become private 
 **/
wi_boolean_t						wb_bot_reload_configuration(wb_bot_t *);

void								wb_bot_subscribe_watchers(wb_bot_t *);
void								wb_bot_unsubscribe_watchers(wb_bot_t *);
wb_watcher_t *						wb_bot_watcher_for_path(wb_bot_t *, wi_string_t *);

wi_boolean_t						wb_bot_execute_output(wb_output_t *, wr_user_t *);
wi_boolean_t						wb_bot_execute_command(wb_bot_t *, wb_command_t *, wi_string_t *, wr_user_t *, wi_p7_message_t *);

wi_array_t *						wb_bot_outputs_for_message(wb_bot_t *, wr_user_t *, wi_p7_message_t *);
wb_command_t * 						wb_bot_command_for_message(wb_bot_t *, wr_user_t *, wi_p7_message_t *);

wi_boolean_t 						wb_bot_check_input_match(wb_input_t *, wi_string_t *);
wi_boolean_t 						wb_bot_check_rule_permissions(wr_user_t *, wb_rule_t *);
wi_boolean_t 						wb_bot_check_command_permissions(wr_user_t *, wb_command_t *);
wi_string_t * 						wb_bot_input_for_message(wi_p7_message_t *);

wb_output_t *						wb_bot_select_random_output(wi_array_t *);

void								wb_bot_log_rule_input(wb_rule_t *, wb_input_t *);
void								wb_bot_log_rule_output(wb_rule_t *, wb_output_t *);
void  								wb_bot_log_command(wb_command_t *);

void								wb_bot_start_command(wb_bot_t *);
void								wb_bot_stop_command(wb_bot_t *);
void								wb_bot_sleep_command(wb_bot_t *);
void								wb_bot_nick_command(wb_bot_t *, wi_string_t *);
void								wb_bot_status_command(wb_bot_t *, wi_string_t *);
void								wb_bot_help_command(wb_bot_t *, wi_p7_message_t *, wr_user_t *);

#endif /* WR_BOT_H */

