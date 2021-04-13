#include "dhmp_server.h"
#include "dhmp.h"
#include "dhmp_config.h"
#include "dhmp_dev.h"
#include "dhmp_hash.h"
#include "dhmp_log.h"
#include "dhmp_transport.h"
#include "dhmp_watcher.h"
#include <pthread.h>
#include <rdma/rdma_cma.h>
#include <unistd.h>

// 4KB:4096,8192,16384,32768,65536,131072,262144
// 8KB:8192,16384,32768,65536,131072,262144,524288
// 16KB:16384,32768,65536,131072,262144,524288
// 32KB:32768,65536,131072,262144,524288,1048576
// 64KB:65536,131072,262144,524288,1048576,2097152
// 128KB:131072,262144,524288,1048576,2097152,4194304
// 256KB:262144,524288,1048576,2097152,4194304,8388608
// 512KB:524288,1048576,2097152,4194304,8388608,16777216
// 1MB:1048576,2097152,4194304,8388608,16777216,33554432
// 2MB:2097152,4194304,8388608,16777216,33554432,67108864
// 3MB:3145728,6291456,12582912,25165824,50331648,100663296
// 4MB:4194304,8388608,16777216,33554432,67108864,134217728
// 5MB:5242880,10485760,20971520,41943040,83886080,167772160
// 6MB:6291456,12582912,25165824,50331648,100663296,201326592
// 8MB:8388608,16777216,33554432,67108864,134217728,268435456
// 32MB:33554432,67108864,134217728,268435456,536870912
// 64MB:67108864,134217728,268435456,536870912,1073741824,2147483648

/*BUDDY_NUM_SUM = the total num of buddy num array*/
#define BUDDY_NUM_SUM 6
const int buddy_num[MAX_ORDER] = {2, 1, 1, 1, 1};
const size_t buddy_size[MAX_ORDER] = {65536, 131072, 262144, 524288, 1048576};

struct dhmp_server *server = NULL;

/**
 * @brief thread routine for connected relicas. Blocking
 *
 * @param arg
 * @return void*
 */
static void *connection_routine(void *arg) {
    DEBUG_LOG("connection_routine start");
    int target_id = *(int *)arg; // connection request targer server id
    free((int *)arg);
    int retry_cnt = 0;

    server->replica_transports_table[target_id] =
        dhmp_transport_create(&server->ctx, dhmp_get_dev_from_server(), false, false);
    while (1) {
        INFO_LOG("trying to connect retry count :%d", retry_cnt);
        int retval = dhmp_transport_connect(server->replica_transports_table[target_id],
                                            server->config.server_infos_table[target_id].replica_info.addr,
                                            server->config.server_infos_table[target_id].replica_info.port);
        if (retval < 0) {
            ERROR_LOG("server connect failed");
            //? need to clean transport?
            break;
        }
        // XXX poll to waiting for state change.
        while (server->replica_transports_table[target_id]->trans_state == DHMP_TRANSPORT_STATE_CONNECTING)
            ;
        if (server->replica_transports_table[target_id]->trans_state == DHMP_TRANSPORT_STATE_ERROR) {
            INFO_LOG("connect to replica #%d failed, retry times: %d", target_id, ++retry_cnt);
            sleep(1 * retry_cnt);
            continue;
        }
        if (server->replica_transports_table[target_id]->trans_state == DHMP_TRANSPORT_STATE_CONNECTED) {
            INFO_LOG("connect to replica #%d succeed !", target_id);
            break;
        }
    }
    DEBUG_LOG("connection_routine finished");
    return NULL;
}

//
/**
 * @brief append the incoming msg to log
    Blocking due to the applying for the lock
 *
 * @param msg
 */
void dhmp_server_append_log(struct dhmp_msg *msg) {
    struct dhmp_msg *msg_copy;
    msg_copy = malloc(sizeof(struct dhmp_msg));

    pthread_mutex_lock(&server->log_lock);
    list_add_tail(&msg_copy->entry, &server->log);
    pthread_mutex_unlock(&server->log_lock);
}
/**
 * @brief leader used for directly forwarding some client's request, like dhmp_malloc() etc.
 * Blocking
 * @param msg forwarding msg pointer
 */
void dhmp_leader_forward_msg(struct dhmp_msg *msg) {
    // TODO check if the server is leader.

    struct dhmp_msg *msg_copy;
    msg_copy = malloc(sizeof(struct dhmp_msg));

    for (int i = 0; i < server->other_replica_cnts; i++) {
        int replica_id = server->other_replica_info_ptrs[i]->server_id;
        dhmp_post_send(server->replica_transports_table[replica_id], msg_copy);
    }
}

//=============================== public methods ===============================

int dhmp_hash_in_server(void *nvm_addr) {
    uint32_t key;
    int index;

    key = hash(&nvm_addr, sizeof(void *));
    index = ((key % DHMP_DRAM_HT_SIZE) + DHMP_DRAM_HT_SIZE) % DHMP_DRAM_HT_SIZE;

    return index;
}

/**
 *	dhmp_get_dev_from_server:get the dev_ptr from dev_list of server.
 */
struct dhmp_device *dhmp_get_dev_from_server() {
    struct dhmp_device *res_dev_ptr = NULL;
    if (!list_empty(&server->dev_list)) {
        res_dev_ptr = list_first_entry(&server->dev_list, struct dhmp_device, dev_entry);
    }

    return res_dev_ptr;
}

/**
 *	dhmp_buddy_system_build:build the buddy system in area
 */
bool dhmp_buddy_system_build(struct dhmp_area *area) {
    struct dhmp_free_block *free_blk[BUDDY_NUM_SUM];
    int i, j, k, size = buddy_size[0], total = 0;

    for (k = 0; k < BUDDY_NUM_SUM; k++) {
        free_blk[k] = malloc(sizeof(struct dhmp_free_block));
        if (!free_blk[k]) {
            ERROR_LOG("allocate memory error.");
            goto out_free_blk_array;
        }
    }

    for (i = 0, k = 0; i < MAX_ORDER; i++) {
        INIT_LIST_HEAD(&area->block_head[i].free_block_list);
        area->block_head[i].nr_free = buddy_num[i];

        for (j = 0; j < buddy_num[i]; j++) {
            free_blk[k]->addr = area->mr->addr + total;
            free_blk[k]->size = size;
            free_blk[k]->mr = area->mr;
            list_add_tail(&free_blk[k]->free_block_entry, &area->block_head[i].free_block_list);
            total += size;
            INFO_LOG("i %d k %d addr %p", i, k, free_blk[k]->addr);
            k++;
        }

        size *= 2;
    }

    return true;

out_free_blk_array:
    for (k = 0; k < BUDDY_NUM_SUM; k++) {
        if (free_blk[k])
            free(free_blk[k]);
        else
            break;
    }
    return false;
}

struct dhmp_area *dhmp_area_create(bool has_buddy_sys, size_t length) {
    void *addr = NULL;
    struct dhmp_area *area = NULL;
    struct ibv_mr *mr;
    struct dhmp_device *dev;
    bool res;

    /*nvm memory*/
    addr = malloc(length);
    if (!addr) {
        ERROR_LOG("allocate nvm memory error.");
        return NULL;
    }

    dev = dhmp_get_dev_from_server();
    mr = ibv_reg_mr(dev->pd, addr, length, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!mr) {
        ERROR_LOG("ib register mr error.");
        goto out_addr;
    }

    area = malloc(sizeof(struct dhmp_area));
    if (!area) {
        ERROR_LOG("allocate memory error.");
        goto out_mr;
    }

    area->mr = mr;
    if (has_buddy_sys) {
        area->max_index = MAX_ORDER - 1;
        res = dhmp_buddy_system_build(area);
        if (!res)
            goto out_area;
        list_add(&area->area_entry, &server->area_list);
    } else {
        list_add(&area->area_entry, &server->more_area_list);
        area->max_index = -2;
    }

    return area;

out_area:
    free(area);
out_mr:
    ibv_dereg_mr(mr);
out_addr:
    free(addr);

    return NULL;
}

void dhmp_server_init(int id) {
    int i, err = 0;

    server = (struct dhmp_server *)malloc(sizeof(struct dhmp_server));
    if (!server) {
        ERROR_LOG("allocate memory error.");
        return;
    }
    // init for hash
    dhmp_hash_init();

    // init for config and add ref in server
    dhmp_config_init(&server->config, false);
    // server->server_id = server->config.curnet_id;
    server->server_id = id;
    server->my_server_info_ptr = &server->config.server_infos_table[server->server_id];

    server->group_id = server->config.group_id;
    server->other_replica_cnts = server->config.group_infos_table[server->group_id].member_cnt - 1;
    for (int i = 0, j = 0; i < server->config.group_infos_table[server->group_id].member_cnt; i++) {
        int replica_id = server->config.group_infos_table[server->group_id].member_ids[i];
        if (replica_id != server->server_id) {
            server->other_replica_info_ptrs[j++] = &server->config.server_infos_table[replica_id];
        }
    }

    // init for init
    dhmp_context_init(&server->ctx);

    server->watcher_trans = NULL;
    /*init client transport list*/
    server->cur_connections = 0;
    pthread_mutex_init(&server->mutex_client_list, NULL);
    INIT_LIST_HEAD(&server->client_list);

    /*init list about rdma device*/
    INIT_LIST_HEAD(&server->dev_list);
    dhmp_dev_list_init(&server->dev_list);

    /*init the structure about memory count*/
    /*get dram total size, get nvm total size*/
    server->dram_total_size = numa_node_size(0, NULL);
    server->nvm_total_size = numa_node_size(1, NULL);

    server->dram_used_size = server->nvm_used_size = 0;
    INFO_LOG("server dram total size %ld", server->dram_total_size);
    INFO_LOG("server nvm total size %ld", server->nvm_total_size);

    /*init dram cache*/
    for (i = 0; i < DHMP_DRAM_HT_SIZE; i++)
        INIT_HLIST_HEAD(&server->dram_ht[i]);

    server->listen_trans = dhmp_transport_create(&server->ctx, dhmp_get_dev_from_server(), true, false);
    if (!server->listen_trans) {
        ERROR_LOG("create rdma transport error.");
        exit(-1);
    }

    err = dhmp_transport_listen(server->listen_trans, server->my_server_info_ptr->net_info.port);
    if (err)
        exit(-1);

    /*create one area and init area list*/
    INIT_LIST_HEAD(&server->area_list);
    INIT_LIST_HEAD(&server->more_area_list);
    server->cur_area = dhmp_area_create(true, SINGLE_AREA_SIZE);

    /*************************following handle the replicas*****************************/

    memset(server->replica_transports_table, 0, (DHMP_MAX_SERVER_GROUP_NUM - 1) * sizeof(struct dhmp_transport *));

    // passively listen from the higher # replica
    server->replica_listen_transport = dhmp_transport_create(&server->ctx, dhmp_get_dev_from_server(), true, false);
    if (!server->replica_listen_transport) {
        ERROR_LOG("create replica_listen_transport error.");
        exit(-1);
    }

    err = dhmp_transport_listen(server->replica_listen_transport, server->my_server_info_ptr->replica_info.port);
    if (err) {
        exit(-1);
    }
    DEBUG_LOG("start to listening replica");

    // actively connect to the lower # replica
    for (int i = 0; i < server->other_replica_cnts; i++) {
        if (server->server_id > server->other_replica_info_ptrs[i]->server_id) {
            pthread_t thread;
            int *p = malloc(sizeof(int));
            *p = i;
            pthread_create(&thread, NULL, connection_routine, p);
            pthread_detach(thread);
        }
    }
}

void dhmp_server_destroy() {
    INFO_LOG("server destroy start.");
    pthread_join(server->ctx.epoll_thread, NULL);
    INFO_LOG("server destroy end.");
    free(server);
}
