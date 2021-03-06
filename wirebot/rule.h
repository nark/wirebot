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


#ifndef WR_RULE_H
#define WR_RULE_H 1

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>

#include <wired/wired.h>


typedef struct _wb_rule				wb_rule_t;

void 								wb_rules_init(void);

wb_rule_t * 						wb_rule_alloc(void);
wb_rule_t *							wb_rule_init(wb_rule_t *, xmlNodePtr);

wi_boolean_t						wb_rule_is_activated(wb_rule_t *);
wi_string_t *						wb_rule_permissions(wb_rule_t *);
wi_mutable_array_t * 				wb_rule_inputs(wb_rule_t *);
wi_mutable_array_t * 				wb_rule_outputs(wb_rule_t *);

#endif /* WR_RULE_H */

