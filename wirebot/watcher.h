#ifndef WR_WATCHER_H
#define WR_WATCHER_H 1

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <wired/wired.h>

#define WB_WATCHER_PATH				WI_STR("@WATCHER_PATH")
#define WB_WATCHER_FILE				WI_STR("@WATCHER_FILE")
#define WB_WATCHER_TEXT 			WI_STR("@WATCHER_TEXT")

typedef struct _wb_watcher			wb_watcher_t;

void 								wb_watchers_init(void);

wb_watcher_t * 						wb_watcher_alloc(void);
wb_watcher_t * 						wb_watcher_init(wb_watcher_t *, xmlNodePtr);

wi_boolean_t						wb_watcher_activated(wb_watcher_t *);
wi_string_t *						wb_watcher_path(wb_watcher_t *);
wi_string_t *				 		wb_watcher_type(wb_watcher_t *);
wi_mutable_array_t *				wb_watcher_files(wb_watcher_t *);
wi_mutable_array_t * 				wb_watcher_new_files(wb_watcher_t *);
wi_mutable_array_t * 				wb_watcher_outputs(wb_watcher_t *);

wi_boolean_t						wb_watcher_add_file(wb_watcher_t *, wi_string_t *);
wi_boolean_t						wb_watcher_add_new_file(wb_watcher_t *, wi_string_t *);

void								wb_watcher_files_diff(wb_watcher_t *);

#endif /* WR_WATCHER_H */