#include "dhmp.h"
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <stdlib.h>
typedef struct event_handler_manager {
    void *owner_ptr;
    struct list_head event_handler_list; /*event_handler*/
} event_handler_manager;

typedef struct event_handler {
    event_handler_manager *manager;
    union {
        enum ibv_event_type ibv_event_type;
        enum rdma_cm_event_type cm_event_type;
        enum dhmp_msg_type msg_type;
    };
    void *(*callback)(void *arg);
    struct list_head entry;
} event_handler;

static void *default_callback(void *arg) {
    // do sth
}

void test() {
    event_handler *malloc_request_handler_ptr = malloc(sizeof(event_handler));
    malloc_request_handler_ptr->callback = default_callback;
    malloc_request_handler_ptr->msg_type = DHMP_MSG_MALLOC_REQUEST;
}