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

#include <wired/wired.h>
#include "spec.h"
#include "client.h"
#include "service.h"
#include "output.h"
#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>


struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}



struct _wb_service {
	wi_runtime_base_t				base;

	wi_string_t 					*name;
	wi_string_t 					*type;
	wi_string_t 					*file_path;
	wi_string_t 					*readable_filename;
	wi_string_t 					*text;
};  

static void							wb_service_dealloc(wi_runtime_instance_t *);
static wi_string_t * 				wb_service_description(wi_runtime_instance_t *);

static wi_runtime_id_t				wb_service_runtime_id = WI_RUNTIME_ID_NULL;
static wi_runtime_class_t			wb_service_runtime_class = {
	"wb_service_t",
	wb_service_dealloc,
	NULL,
	NULL,
	wb_service_description,
	NULL
};



static wb_service_t*				_wb_service_load_with_node(wb_service_t *, xmlNodePtr);
static wi_string_t * 				_wb_service_parse_for_human_readable_name(wb_service_t *);
static wi_string_t * 				_wb_service_curl_request(wb_service_t *, wi_string_t *) ;
static wi_boolean_t 				_wb_service_format_xml_output(wb_service_t *, wb_watcher_t *, wi_string_t *);




#pragma mark -

void wb_services_init(void) {
	wb_service_runtime_id = wi_runtime_register_class(&wb_service_runtime_class);
}






#pragma mark -

wb_service_t * wb_service_alloc(void) {
	return wi_runtime_create_instance(wb_service_runtime_id, sizeof(wb_service_t));
}



wb_service_t * wb_service_init(wb_service_t *service, xmlNodePtr node) {
	service->name 					= NULL;
	service->type 					= NULL;
	service->file_path 				= NULL;
	service->readable_filename 		= NULL;
	service->text 					= NULL;

	return _wb_service_load_with_node(service, node);
}





#pragma mark -

wi_string_t * wb_service_type(wb_service_t *service) {
	return service->type;
}

wi_string_t * wb_service_file_path(wb_service_t *service) {
	return service->file_path;
}

wi_string_t * wb_service_readable_filename(wb_service_t *service) {
	return service->readable_filename;
}

wi_string_t * wb_service_text(wb_service_t *service) {
	return service->text;
}

void wb_service_set_file_path(wb_service_t *service, wi_string_t *file_path) {
	service->file_path = wi_retain(file_path);
}




#pragma mark -

wi_boolean_t wb_service_execute(wb_service_t *service, wb_watcher_t *watcher) {
	wi_string_t 			*readable_name, *xml_string;
	wi_string_t 			*board, *subject, *text;
	wb_output_t				*output;
	wi_p7_message_t 		*message;

	if(!service->file_path)
		return false;

	readable_name = _wb_service_parse_for_human_readable_name(service);

	wi_log_info(WI_STR("Service: %@ searching « %@ »"), service->name, readable_name);

	if(readable_name && wi_string_length(readable_name) > 0) {
		service->readable_filename = wi_retain(readable_name);
		xml_string = _wb_service_curl_request(service, service->readable_filename);

		wi_log_debug(WI_STR("Service output: %@"), xml_string);

		if(_wb_service_format_xml_output(service, watcher, xml_string)) {
			return true;
		}
	}
	return false;
}

void wb_service_cleanup(wb_service_t *service) {
	service->file_path 				= NULL;
	service->readable_filename 		= NULL;
	service->text 					= NULL;
}



#pragma mark - 

static wb_service_t * _wb_service_load_with_node(wb_service_t *service, xmlNodePtr node) {
	wi_string_t 		*name, *type;

	name = wi_xml_node_attribute_with_name(node, WI_STR("name"));
	if(name)
		service->name = wi_retain(name);
	
	type = wi_xml_node_attribute_with_name(node, WI_STR("type"));
	if(type)
		service->type = wi_retain(type);

	return service;
}


static wi_string_t * _wb_service_parse_for_human_readable_name(wb_service_t *service) {
	wi_boolean_t			is_file;
	wi_string_t 			*file_name, *clean_name, *readable_name, *drop_string;
	wi_regexp_t 			*ext_regex, *tail_regex, *nose_regex;

	if(!service->file_path)
			return NULL;

	file_name 		= wi_string_last_path_component(service->file_path);
	clean_name		= wi_string_by_deleting_path_extension(file_name);
	clean_name		= wi_string_by_replacing_string_with_string(clean_name, WI_STR("."), WI_STR(" "), 0);
	clean_name		= wi_string_lowercase_string(clean_name);

	ext_regex 		= wi_regexp_with_string(WI_STR("/([^\\\\\\]+)\\.(m4v|3gp|nsv|ts|ty|strm|rm|rmvb|m3u|ifo|mov|qt|divx|xvid|bivx|vob|nrg|img|iso|pva|wmv|asf|asx|ogm|m2v|avi|bin|dat|dvr-ms|mpg|mpeg|mp4|mkv|avc|vp3|svq3|nuv|viv|dv|fli|flv|rar|001|wpl|zip)$/"));
	tail_regex		= wi_regexp_with_string(WI_STR("/(.\\*?)( cd[0-9]|dvdrip|xvid|ac3|dts|custom|dc|divx|divx5|dsr|dsrip|dutch|dvd|dvdscr|dvdscreener|screener|dvdivx|cam|fragment|fs|hdtv|hdrip|hdtvrip|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|r3|r5|bd5|svcd|s0|swedish|german|read.nfo|nfofix|unrated|ws|telesync|ts|telecine|tc|brrip|bdrip|480p|480i|576p|576i|720p|720i|1080p|1080i|hrhd|hrhdtv|hddvd|bluray|x264|h264|xvidvd|xxx|www.www|-|[\\{\\(\\[]?[0-9]{4}).*/i"));
	nose_regex		= wi_regexp_with_string(WI_STR("/(\\[.*\\])/i"));
	
	drop_string 	= wi_regexp_string_by_matching_string(tail_regex, clean_name, 0);

	if(drop_string) {
		wi_range_t range = wi_string_range_of_string(clean_name, drop_string, 0);

		if(range.location != WI_NOT_FOUND) {
			readable_name = wi_string_by_deleting_characters_in_range(clean_name, range);

			drop_string = wi_regexp_string_by_matching_string(nose_regex, readable_name, 0);
			
			if(drop_string) {
				range 	= wi_string_range_of_string(readable_name, drop_string, 0);
				if(range.location != WI_NOT_FOUND) {
					readable_name = wi_string_by_deleting_characters_in_range(readable_name, range);
				}
			}
		} 
		else {
			readable_name = clean_name;
		}
	} 
	else {
		readable_name = clean_name;
	}
	return wi_string_by_deleting_surrounding_whitespace(readable_name);
}


static wi_string_t * _wb_service_curl_request(wb_service_t *service, wi_string_t *name) {
	wi_string_t 			*api_key, *url, *result;
	CURL 					*curl;  
	CURLcode 				res;
	struct MemoryStruct 	chunk;

	curl_global_init(CURL_GLOBAL_ALL);

	result 			= NULL;    			
	curl 			= curl_easy_init();
	chunk.memory 	= malloc(1);
	chunk.size 		= 0;

	if(curl) {
		api_key = wi_config_string_for_name(wd_config, WI_STR("omdb api key"));

		url		= wi_string_with_format(WI_STR("http://www.omdbapi.com/?apiKey=%@&t=%@&r=xml"), api_key, name);
		url 	= wi_string_by_replacing_string_with_string(url, WI_STR(" "), WI_STR("+"), 0);

		wi_log_debug(WI_STR("Service URL: %@"), url);

		curl_easy_setopt(curl, CURLOPT_URL, wi_string_cstring(url));
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		/* Perform the request, res will get the return code */
		res 	= curl_easy_perform(curl);

		/* Check for errors */
		if(res != CURLE_OK)
		  fprintf(stderr, "ERROR: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		if(chunk.memory) {
			result = wi_string_with_format(WI_STR("%s"), chunk.memory);
    		free(chunk.memory);
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return result;
}


static wi_boolean_t _wb_service_format_xml_output(wb_service_t *service, wb_watcher_t *watcher, wi_string_t *xml_string) {
	wi_string_t 	*string, *response;
	xmlDocPtr		doc;
	xmlNodePtr		root, node, next;
	int				length;

	string 			= wi_mutable_string();
	doc 			= xmlParseDoc(wi_string_cstring(xml_string));
	root 			= xmlDocGetRootElement(doc);
	response 		= wi_xml_node_attribute_with_name(root, WI_STR("response"));

	if(!wi_is_equal(response, WI_STR("True"))) {
		xmlCleanupParser();
		return false;
	}

	for(node = root->children; node != NULL; node = next) {
		next = node->next;

		if(node->type == XML_ELEMENT_NODE) {
			if(strcmp((const char *) node->name, "movie") == 0) {
				wi_mutable_string_append_format(string, WI_STR("[b]Title:[/b] %@\n"), 
					wi_xml_node_attribute_with_name(node, WI_STR("title")));

				wi_mutable_string_append_format(string, WI_STR("[b]Year:[/b] %@\n"), 
					wi_xml_node_attribute_with_name(node, WI_STR("year")));

				wi_mutable_string_append_format(string, WI_STR("[b]Genre:[/b] %@\n"), 
					wi_xml_node_attribute_with_name(node, WI_STR("genre")));

				wi_mutable_string_append_format(string, WI_STR("[b]IMDB:[/b] [url]http://www.imdb.com/title/%@[/url]\n"), 
					wi_xml_node_attribute_with_name(node, WI_STR("imdbID")));

				wi_mutable_string_append_format(string, WI_STR("[b]Path:[/b] %@\n\n"), 
					service->file_path);

				wi_mutable_string_append_format(string, WI_STR("[img]%@[/img]"), 
					wi_xml_node_attribute_with_name(node, WI_STR("poster")));

				service->text 	= wi_retain(string);
			}
		}
	}

	xmlCleanupParser();

	return true;
}






#pragma mark -

static void wb_service_dealloc(wi_runtime_instance_t *instance) {
	wb_service_t			*service = instance;

	if(service->name)
		wi_release(service->name);

	if(service->type)
		wi_release(service->type);

	if(service->file_path)
		wi_release(service->file_path);

	if(service->text)
		wi_release(service->text);

	if(service->readable_filename)
		wi_release(service->readable_filename);
}

static wi_string_t * wb_service_description(wi_runtime_instance_t *instance) {
	wb_service_t			* service = instance;
	wi_string_t 			* string;

	string = wi_string_with_format(WI_STR("Service: [%@] -> %@"), service->name, service->type);

	return string;
}


