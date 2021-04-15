#ifndef DHMP_H
#define DHMP_H

#include <arpa/inet.h>
#include <assert.h>
#include <infiniband/verbs.h>
#include <json-c/json.h>
#include <libxml/parser.h>
#include <linux/list.h>
#include <linux/sockios.h>
#include <math.h>
#include <net/if.h>
#include <netinet/in.h>
#include <numa.h>
#include <pthread.h>
#include <rdma/rdma_cma.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

//#define DHMP_CACHE_POLICY
//#define DHMP_MR_REUSE_POLICY

#define DHMP_SERVER_DRAM_TH (1024 * 1024 * 1024)

#define DHMP_MAX_SERVER_GROUP_NUM 5
#define DHMP_MAX_SERVER_GROUP_MEMBER_NUM 5
#define DHMP_MAX_SERVER_NUM (DHMP_MAX_SERVER_GROUP_NUM * DHMP_MAX_SERVER_GROUP_MEMBER_NUM)

#define DHMP_DEFAULT_SIZE 256
#define DHMP_DEFAULT_POLL_TIME 200000000

#define DHMP_MAX_OBJ_NUM 20000
#define DHMP_MAX_CLIENT_NUM 100

#define PAGE_SIZE 4096
#define NANOSECOND (1000000000)

#define DHMP_RTT_TIME (6000)
#define DHMP_DRAM_RW_TIME (260)

#define REPLICAS_NUM 2

#define max(a, b) (a > b ? a : b)
#define min(a, b) (a > b ? b : a)

#ifndef bool
#define bool char
#define true 1
#define false 0
#endif

// XXX need a uuid library
typedef int uuid_t;

enum dhmp_msg_type {

    DHMP_MSG_MALLOC_REQUEST,
    DHMP_MSG_MALLOC_RESPONSE,
    DHMP_MSG_MALLOC_ERROR,
    DHMP_MSG_FREE_REQUEST,
    DHMP_MSG_FREE_RESPONSE,
    DHMP_MSG_APPLY_DRAM_REQUEST,
    DHMP_MSG_APPLY_DRAM_RESPONSE,
    DHMP_MSG_CLEAR_DRAM_REQUEST,
    DHMP_MSG_CLEAR_DRAM_RESPONSE,
    DHMP_MSG_MEM_CHANGE,
    DHMP_MSG_SERVER_INFO_REQUEST,
    DHMP_MSG_SERVER_INFO_RESPONSE,
    DHMP_MSG_CLOSE_CONNECTION
};

/*struct dhmp_msg:use for passing control message*/
struct dhmp_msg {
    enum dhmp_msg_type msg_type;
    size_t data_size;
    void *data;
    struct list_head entry;
};

/*struct dhmp_addr_info is the addr struct in cluster*/
struct dhmp_addr_info {
    int read_cnt;
    int write_cnt;
    int node_index;
    bool write_flag;
    struct ibv_mr dram_mr;
    struct ibv_mr nvm_mr;
    struct hlist_node addr_entry;
};

/*dhmp malloc request msg*/
struct dhmp_mc_request {
    uuid_t uuid;
    size_t req_size;
    struct dhmp_addr_info *addr_info;
};

/*dhmp malloc response msg*/
struct dhmp_mc_response {
    struct dhmp_mc_request req_info;
    struct ibv_mr mr;
};

/*dhmp free memory request msg*/
struct dhmp_free_request {
    uuid_t uuid;
    struct dhmp_addr_info *addr_info;
    struct ibv_mr mr;
};

/*dhmp free memory response msg*/
struct dhmp_free_response {
    struct dhmp_addr_info *addr_info;
};

struct dhmp_dram_info {
    void *nvm_addr;
    struct ibv_mr dram_mr;
};

#endif
