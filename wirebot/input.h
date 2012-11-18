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

#ifndef WR_INPUT_H
#define WR_INPUT_H 1

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <wired/wired.h>


/**
 * Comparison method wirebot use to match
 * an input value 
 */
enum _wb_comparison_method {
	WB_NOT_EQUALS				= 0,
	WB_EQUALS					= 1,
	WB_CONTAINS					= 2,
	WB_STARTS_WITH				= 3,
	WB_ENDS_WITH				= 4
};
typedef enum _wb_comparison_method			wb_bot_comparison_method_t;




typedef struct _wb_input			wb_input_t;

wb_input_t * 						wb_input_alloc(void);
wb_input_t *						wb_input_init(wb_input_t *, xmlNodePtr);

wi_string_t * 						wb_input_message_name(wb_input_t *);
wi_string_t *						wb_input_input(wb_input_t *);
wb_bot_comparison_method_t			wb_input_comparison(wb_input_t *);
wi_boolean_t						wb_input_is_case_sensitive(wb_input_t *);

#endif /* WR_INPUT_H */

