#ifndef DHMP_TRANSPORT_H
#define DHMP_TRANSPORT_H
#include "dhmp.h"
#include "dhmp_client.h"
#include <infiniband/verbs.h>
#define ADDR_RESOLVE_TIMEOUT 500
#define ROUTE_RESOLVE_TIMEOUT 500

#define RECV_REGION_SIZE (64 * 1024 * 1024)
#define SEND_REGION_SIZE (64 * 1024 * 1024)

/*recv region include poll recv region,normal recv region*/
#define SINGLE_POLL_RECV_REGION (8 * 1024 * 1024)
#define SINGLE_NORM_RECV_REGION (8 * 1024)

enum dhmp_transport_state {
    DHMP_TRANSPORT_STATE_INIT,
    DHMP_TRANSPORT_STATE_LISTEN,
    DHMP_TRANSPORT_STATE_CONNECTING,
    DHMP_TRANSPORT_STATE_CONNECTED,
    DHMP_TRANSPORT_STATE_DISCONNECTED,
    DHMP_TRANSPORT_STATE_RECONNECT, //? 为啥不用
    DHMP_TRANSPORT_STATE_CLOSED,    //?
    DHMP_TRANSPORT_STATE_DESTROYED, //?
    DHMP_TRANSPORT_STATE_ERROR
};

struct dhmp_cq {
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;
    struct dhmp_device *device;

    /*add the fd of comp_channel into the ctx*/
    struct dhmp_context *ctx;
};

struct dhmp_mr {
    size_t cur_pos;
    void *addr;
    struct ibv_mr *mr;
};

typedef enum dhmp_transport_type {
    DHMP_TRANSPORT_TYPE_MASK_PEER_SERVER = 1 << 1,
    DHMP_TRANSPORT_TYPE_MASK_PEER_CLIENT = 1 << 2,
    DHMP_TRANSPORT_TYPE_MASK_PEER_WATCHER = 1 << 3,

    DHMP_TRANSPORT_TYPE_MASK_CONNECTION_NORMAL = 1 << 4,
    DHMP_TRANSPORT_TYPE_MASK_CONNECTION_POLL = 1 << 5,
    DHMP_TRANSPORT_TYPE_MASK_CONNECTION_LISTEN = 1 << 6,

    // server end
    DHMP_TRANSPORT_TYPE_REPLICA_LISTEN =
        DHMP_TRANSPORT_TYPE_MASK_PEER_SERVER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_LISTEN,
    DHMP_TRANSPORT_TYPE_REPLICA_NORMAL =
        DHMP_TRANSPORT_TYPE_MASK_PEER_SERVER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_POLL,

    DHMP_TRANSPORT_TYPE_CLIENT_LISTEN =
        DHMP_TRANSPORT_TYPE_MASK_PEER_CLIENT | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_LISTEN,
    DHMP_TRANSPORT_TYPE_CLIENT_NORMAL =
        DHMP_TRANSPORT_TYPE_MASK_PEER_CLIENT | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_NORMAL,
    DHMP_TRANSPORT_TYPE_CLIENT_POLL = DHMP_TRANSPORT_TYPE_MASK_PEER_CLIENT | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_POLL,

    DHMP_TRANSPORT_TYPE_WATCHER_LISTEN =
        DHMP_TRANSPORT_TYPE_MASK_PEER_WATCHER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_LISTEN,
    DHMP_TRANSPORT_TYPE_WATCHER_NORMAL =
        DHMP_TRANSPORT_TYPE_MASK_PEER_WATCHER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_NORMAL,

    // client, watcher end
    DHMP_TRANSPORT_TYPE_SERVER_NORMAL =
        DHMP_TRANSPORT_TYPE_MASK_PEER_SERVER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_NORMAL,
    DHMP_TRANSPORT_TYPE_SERVER_POLL = DHMP_TRANSPORT_TYPE_MASK_PEER_SERVER | DHMP_TRANSPORT_TYPE_MASK_CONNECTION_POLL

} dhmp_transport_type;

struct dhmp_transport {
    struct sockaddr_in peer_addr;
    struct sockaddr_in local_addr;

    // TODO unify node_id server_id etc.
    int node_id;

    enum dhmp_transport_state trans_state;
    struct dhmp_context *ctx;
    struct dhmp_device *device;
    struct dhmp_cq *dcq;
    struct ibv_qp *qp;
    struct rdma_event_channel *event_channel;
    struct rdma_cm_id *cm_id;

    /*the var use for two sided RDMA*/
    struct dhmp_mr send_mr;
    struct dhmp_mr recv_mr;

    // bool is_poll_qp;
    struct dhmp_transport *link_trans;

    long dram_used_size;
    long nvm_used_size;

    struct list_head client_entry;

    // XXX maybe not necessary?
    dhmp_transport_type peer_type;
};

struct dhmp_send_mr {
    struct ibv_mr *mr;
    struct list_head send_mr_entry;
};

struct dhmp_transport *dhmp_transport_create(struct dhmp_context *ctx, struct dhmp_device *dev,
                                             enum dhmp_transport_type type);

void dhmp_transport_destroy(struct dhmp_transport *rdma_trans);

int dhmp_transport_connect(struct dhmp_transport *rdma_trans, const char *url, int port);

int dhmp_transport_listen(struct dhmp_transport *rdma_trans, int listen_port);

void dhmp_post_send(struct dhmp_transport *rdma_trans, struct dhmp_msg *msg_ptr);

void dhmp_post_recv(struct dhmp_transport *rdma_trans, void *addr);

int dhmp_rdma_write(struct dhmp_transport *rdma_trans, struct dhmp_addr_info *addr_info, struct ibv_mr *mr,
                    void *local_addr, int length);

int dhmp_rdma_read(struct dhmp_transport *rdma_trans, struct ibv_mr *mr, void *local_addr, int length);
#endif
