#ifndef DHMP_CONFIG_H
#define DHMP_CONFIG_H

#include "dhmp.h"

#define DHMP_CONFIG_FILE_NAME "config.xml"
#define DHMP_ADDR_LEN 18
#define DHMA_NIC_NAME_LEN 10

struct dhmp_net_info {
    char nic_name[DHMA_NIC_NAME_LEN];
    char addr[DHMP_ADDR_LEN];
    int port;
};

/*nvm simulate infomation*/
struct dhmp_simu_info {
    int rdelay;
    int wdelay;
    int knum;
};

// TODO complete definition
struct dhmp_client_info {};

struct dhmp_watcher_info {};

struct dhmp_server_info {
    int server_id;
    struct dhmp_net_info net_info;
    struct dhmp_net_info replica_info;
};

struct dhmp_replica_group_info {
    int group_id;
    int member_cnt;
    int member_ids[DHMP_MAX_SERVER_GROUP_MEMBER_NUM];
};

struct dhmp_config {

    /* server only:*/
    // TODO unify server_id naming
    int curnet_id; // store the net_infos index of curnet
    int group_id;  // store the replica group index

    int nets_cnt;   // current include total server nodes
    int groups_cnt; // store the exsiting group number - client only

    char watcher_addr[DHMP_ADDR_LEN];
    int watcher_port;

    // new design

    struct dhmp_server_info server_infos[DHMP_MAX_SERVER_GROUP_NUM * DHMP_MAX_SERVER_GROUP_MEMBER_NUM];
    struct dhmp_replica_group_info group_infos[DHMP_MAX_SERVER_GROUP_MEMBER_NUM];
};

/**
 *	app read the config.xml infomation,
 *	analyse the client configuration include log level
 *	and the servers configuration
 */
int dhmp_config_init(struct dhmp_config *config_ptr, bool is_client);
#endif
