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

#ifndef WR_OUTPUT_H
#define WR_OUTPUT_H 1

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <wired/wired.h>

#include "users.h"



/**
 * Time value used to trigger an output
 * only at a desired time
 */
enum _wb_time_range {
	WB_EVERY_TIME				= 0,
	WB_MORNING_TIME				= 1,
	WB_AFTERNOON_TIME			= 2,
	WB_EVENING_TIME				= 3,
	WB_NIGHT_TIME				= 4
};
typedef enum _wb_time_range			wb_bot_time_range_t;





typedef struct _wb_output		wb_output_t;

wb_output_t * 					wb_output_alloc(void);
wb_output_t *					wb_output_init(wb_output_t *, xmlNodePtr);
wb_output_t *					wb_output_init_with_message(wb_output_t *, wi_p7_message_t *);

wi_string_t *					wb_output_wire_command_string(wb_output_t *, wr_user_t *);
wi_boolean_t					wb_output_is_chat(wb_output_t *);

wi_string_t *					wb_output_message_name(wb_output_t *);
void							wb_output_set_message_name(wb_output_t *, wi_string_t *);

wi_string_t * 					wb_output_input_text(wb_output_t *);
void							wb_output_set_input_text(wb_output_t *, wi_string_t *);

wi_string_t * 					wb_output_output(wb_output_t *);
void							wb_output_set_output(wb_output_t *, wi_string_t *);

wb_bot_time_range_t				wb_output_time(wb_output_t *);
wi_integer_t					wb_output_delay(wb_output_t *);
wi_integer_t					wb_output_repeat(wb_output_t *);

#endif /* WR_OUTPUT_H */
