/* $Id$ */

/*
 *  Copyright (c) 2004-2011 Axel Andersson
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <wired/wired.h>

#include "chats.h"

#define WR_PUBLIC_CID				1


struct _wr_chat {
	wi_runtime_base_t				base;
	
	wr_cid_t						cid;
	wi_mutable_array_t				*users_array;
	wi_mutable_dictionary_t			*users_dictionary;
};  


static void							wr_chat_dealloc(wi_runtime_instance_t *);
static wi_boolean_t					wr_chat_is_equal(wi_runtime_instance_t *, wi_runtime_instance_t *);
static wi_string_t *				wr_chat_description(wi_runtime_instance_t *);
static wi_hash_code_t				wr_chat_hash(wi_runtime_instance_t *);


wr_chat_t							*wr_public_chat;
wr_chat_t							*wr_private_chat;
wr_uid_t							wr_private_chat_invite_uid;

static wi_mutable_dictionary_t		*wr_chats;

static wi_runtime_id_t				wr_chat_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wr_chat_runtime_class = {
	"wr_chat_t",
	wr_chat_dealloc,
	NULL,
	wr_chat_is_equal,
	wr_chat_description,
	wr_chat_hash
};


void wr_chats_init(void) {
	wr_chat_runtime_id = wi_runtime_register_class(&wr_chat_runtime_class);

	wr_chats = wi_dictionary_init(wi_mutable_dictionary_alloc());
	
	wr_public_chat = wr_chat_init_public_chat(wr_chat_alloc());
	wr_chats_add_chat(wr_public_chat);
}



void wr_chats_clear(void) {
	wi_mutable_dictionary_remove_all_data(wr_chats);
	wr_chat_remove_all_users(wr_public_chat);
	wr_chats_add_chat(wr_public_chat);
	
	wi_release(wr_private_chat);
	wr_private_chat = NULL;
	
	wr_private_chat_invite_uid = 0;
}



#pragma mark -

void wr_chats_add_chat(wr_chat_t *chat) {
	wi_mutable_dictionary_set_data_for_key(wr_chats, chat, wi_number_with_integer(chat->cid));
}



void wr_chats_remove_chat(wr_chat_t *chat) {
	wi_mutable_dictionary_remove_data_for_key(wr_chats, wi_number_with_integer(chat->cid));
}



wr_chat_t * wr_chats_chat_with_cid(wr_cid_t cid) {
	return wi_dictionary_data_for_key(wr_chats, wi_number_with_integer(cid));
}



#pragma mark -

wr_chat_t * wr_chat_alloc(void) {
	return wi_runtime_create_instance(wr_chat_runtime_id, sizeof(wr_chat_t));
}



wr_chat_t * wr_chat_init(wr_chat_t *chat) {
	chat->users_array			= wi_array_init(wi_mutable_array_alloc());
	chat->users_dictionary		= wi_dictionary_init(wi_mutable_dictionary_alloc());
	
	return chat;
}



wr_chat_t * wr_chat_init_public_chat(wr_chat_t *chat) {
	chat = wr_chat_init(chat);
	chat->cid = WR_PUBLIC_CID;
	
	return chat;
}



wr_chat_t * wr_chat_init_private_chat(wr_chat_t *chat, wr_cid_t cid) {
	chat = wr_chat_init(chat);
	chat->cid = cid;
	
	return chat;
}



static void wr_chat_dealloc(wi_runtime_instance_t *instance) {
	wr_chat_t		*chat = instance;
	
	wi_release(chat->users_array);
	wi_release(chat->users_dictionary);
}



static wi_boolean_t wr_chat_is_equal(wi_runtime_instance_t *instance1, wi_runtime_instance_t *instance2) {
	wr_chat_t		*chat1 = instance1;
	wr_chat_t		*chat2 = instance2;
	
	return (chat1->cid == chat2->cid);
}



static wi_string_t * wr_chat_description(wi_runtime_instance_t *instance) {
	wr_chat_t		*chat = instance;
	
	return wi_string_with_format(WI_STR("<%@ %p>{cid = %u, users = %@}"),
		wi_runtime_class_name(chat),
		chat,
		chat->cid,
		chat->users_array);
}



static wi_hash_code_t wr_chat_hash(wi_runtime_instance_t *instance) {
	wr_chat_t		*chat = instance;
	
	return chat->cid;
}



#pragma mark -

wr_cid_t wr_chat_id(wr_chat_t *chat) {
	return chat->cid;
}



wi_array_t * wr_chat_users(wr_chat_t *chat) {
	return chat->users_array;
}



#pragma mark -

void wr_chat_add_user(wr_chat_t *chat, wr_user_t *user) {
	wi_mutable_array_add_data(chat->users_array, user);
	wi_mutable_dictionary_set_data_for_key(chat->users_dictionary, user, wi_number_with_integer(wr_user_id(user)));
}



void wr_chat_remove_user(wr_chat_t *chat, wr_user_t *user) {
	wi_mutable_array_remove_data(chat->users_array, user);
	wi_mutable_dictionary_remove_data_for_key(chat->users_dictionary, wi_number_with_integer(wr_user_id(user)));
}



void wr_chat_remove_all_users(wr_chat_t *chat) {
	wi_mutable_array_remove_all_data(chat->users_array);
	wi_mutable_dictionary_remove_all_data(chat->users_dictionary);
}



wr_user_t * wr_chat_user_with_uid(wr_chat_t *chat, wr_uid_t uid) {
	return wi_dictionary_data_for_key(chat->users_dictionary, wi_number_with_integer(uid));
}



wr_user_t * wr_chat_user_with_nick(wr_chat_t *chat, wi_string_t *nick) {
	wi_enumerator_t	*enumerator;
	wi_string_t		*name;
	wr_user_t       *user, *value = NULL;
	char			*cname;

	cname = ((*rl_filename_dequoting_function) ((char *) wi_string_cstring(nick), 0));
	name = wi_string_with_cstring(cname);
	free(cname);

	enumerator = wi_array_data_enumerator(chat->users_array);
	
	while((user = wi_enumerator_next_data(enumerator))) {
		if(wi_is_equal(wr_user_nick(user), name)) {
			value = user;

			break;
		}
	}

	return value;
}
