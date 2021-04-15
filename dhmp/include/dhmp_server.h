#ifndef DHMP_SERVER_H
#define DHMP_SERVER_H
#include "dhmp.h"
#include "dhmp_config.h"
#include "dhmp_context.h"
#include "linux/list.h"
#include <pthread.h>

/*decide the buddy system's order*/
#define MAX_ORDER 5
#define SINGLE_AREA_SIZE 2097152

#define DHMP_DRAM_HT_SIZE 251

extern const size_t buddy_size[MAX_ORDER];
/*XXX to some extent, it is kind of a ugly subtitution of thread stack in multithread model,
since the event handler loop is single-thread. If you want to keep the singe thread model and not
add extra blocking mechanism to achieve the purpose of synchronization, you have to do so. It is supposed
to be replaced in future.
    anyway, it do works. :)
*/
struct client_req_meta_info {
    uuid_t req_id;
    union {
        struct dhmp_mc_response mc_res;
        struct dhmp_free_response free_res;
        // TODO more response
    };
    struct dhmp_transport *trans_ptr;
    int res_cnt; // used for leader for counting the follower responses. when
    struct hlist_node h_entry;
};

enum dhmp_log_entry_op_type { DHMP_MALLOC, DHMP_FREE, DHMP_WRITE };

struct dhmp_log_entry {
    struct list_head log_entry;
    struct hlist_node ht_entry;
    uuid_t uuid;
    enum dhmp_log_entry_op_type op_type;
    // how to deal the addr
    union {
        struct dhmp_mc_request mc_req;
        struct dhmp_free_request free_req;
    };
};

struct dhmp_log {
    struct list_head log_head; /*type: dhmp_log_entry */
};

struct dhmp_free_block {
    void *addr;
    size_t size;
    struct ibv_mr *mr;
    struct list_head free_block_entry;
};

struct dhmp_free_block_head {
    struct list_head free_block_list;
    int nr_free;
};

struct dhmp_area {
    /*be able to alloc max size=buddy_size[max_index]*/
    int max_index;
    // pthread_spinlock_t mutex_area;
    struct ibv_mr *mr;
    struct list_head area_entry;
    struct dhmp_free_block_head block_head[MAX_ORDER];
};

struct dhmp_dram_entry {
    void *nvm_addr;
    size_t length;
    struct ibv_mr *dram_mr;
    struct hlist_node dram_node;
};

struct dhmp_server {
    struct dhmp_context ctx;
    struct dhmp_config config;

    // store config metadata to simplify access.
    int server_id; // my id
    int group_id;  // my replica group id
    struct dhmp_server_info
        *other_replica_info_ptrs[DHMP_MAX_SERVER_GROUP_MEMBER_NUM - 1]; // my replica group members(not include myself)
    int other_replica_cnts;
    struct dhmp_server_info *my_server_info_ptr;

    struct dhmp_transport *listen_trans;
    struct dhmp_transport *watcher_trans;

    struct list_head dev_list;
    pthread_mutex_t mutex_client_list;
    struct list_head client_list;

    /*below structure about area*/
    struct dhmp_area *cur_area;
    struct list_head area_list;
    struct list_head more_area_list;

    int cur_connections;
    long dram_used_size;
    long nvm_used_size;
    long dram_total_size;
    long nvm_total_size;

    struct hlist_head dram_ht[DHMP_DRAM_HT_SIZE];

    /*replicas infos*/ // TODO init
    struct dhmp_transport *replica_listen_transport;
    struct dhmp_transport *replica_transports_table[DHMP_MAX_SERVER_GROUP_MEMBER_NUM - 1]; // server_id -> trans map

    // TODO init
    struct list_head log; // type : struct dhmp_msg
    pthread_t log_thread;
    pthread_mutex_t log_lock;
    struct hlist_head client_request_table[251]; /* type :struct client_req_meta_info
                                                    record the incoming request uuid & transport*/
};

extern struct dhmp_server *server;



struct dhmp_area *dhmp_area_create(bool is_init_buddy, size_t length);

bool dhmp_buddy_system_build(struct dhmp_area *area);

int dhmp_hash_in_server(void *nvm_addr);

struct dhmp_device *dhmp_get_dev_from_server();

void dhmp_server_append_log(struct dhmp_msg *msg);

void dhmp_leader_forward_msg(struct dhmp_msg *msg);


/**
 *	dhmp_server_init:init server
 *	include config.xml read, rdma listen,
 *	register memory, context run.
 */
void dhmp_server_init(int server_id);

/**
 *	dhmp_server_destroy: close the context,
 *	clean the rdma resource
 */
void dhmp_server_destroy();

#endif
