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

#ifndef WR_SERVICE_H
#define WR_SERVICE_H 1

#include "watcher.h"
 
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <wired/wired.h>


typedef struct _wb_service		wb_service_t;

void 							wb_services_init(void);

wb_service_t * 					wb_service_alloc(void);
wb_service_t *					wb_service_init(wb_service_t *, xmlNodePtr);

wi_string_t *					wb_service_type(wb_service_t *);
wi_string_t *					wb_service_file_path(wb_service_t *);
wi_string_t *					wb_service_readable_filename(wb_service_t *);
wi_string_t *					wb_service_text(wb_service_t *);
void 							wb_service_set_file_path(wb_service_t *, wi_string_t *);

wi_boolean_t 					wb_service_execute(wb_service_t *, wb_watcher_t *);
void 							wb_service_cleanup(wb_service_t *);

#endif /* WR_SERVICE_H */
