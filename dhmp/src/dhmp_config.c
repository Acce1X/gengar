#include "dhmp_config.h"
#include "dhmp.h"
#include "dhmp_log.h"
#include "libxml/tree.h"
#include "libxml/xmlstring.h"
#include <stdlib.h>

/*these strings in config xml*/
#define DHMP_DHMP_CONFIG_STR "dhmp_config"
#define DHMP_CLIENT_STR "client"
#define DHMP_LOG_LEVEL_STR "log_level"
#define DHMP_SERVER_STR "server"
#define DHMP_NIC_NAME_STR "nic_name"
#define DHMP_ADDR_STR "addr"
#define DHMP_PORT_STR "port"
#define DHMP_RDELAY_STR "rdelay"
#define DHMP_WDELAY_STR "wdelay"
#define DHMP_KNUM_STR "knum"
#define DHMP_XML_ATTR_ID_STR "id"

#define DHMP_REPLICA_ID_STR "replica_id"
#define DHMP_REPLICA_ADDR_STR "replica_addr"
#define DHMP_REPLICA_NIC_STR "replica_nic"
#define DHMP_REPLICA_PORT_STR "replica_port"

#define DHMP_WATCHER_STR "watcher"
#define DHMP_REPLICA_GROUP_STR "replica_group"
#define DHMP_GROUP_STR "group"

static void dhmp_print_config(struct dhmp_config *total_config_ptr) {
    int i;
    INFO_LOG("server cnt = %d", total_config_ptr->nets_cnt);
    for (i = 0; i < total_config_ptr->nets_cnt; i++) {

        INFO_LOG("-------------------------------------------------");
        INFO_LOG("server id %d", total_config_ptr->server_infos_table[i].server_id);
        INFO_LOG("nic name %s", total_config_ptr->server_infos_table[i].net_info.nic_name);
        INFO_LOG("addr %s", total_config_ptr->server_infos_table[i].net_info.addr);
        INFO_LOG("port %d", total_config_ptr->server_infos_table[i].net_info.port);
        INFO_LOG("replica_nic %s", total_config_ptr->server_infos_table[i].replica_info.nic_name);
        INFO_LOG("replica_addr %s", total_config_ptr->server_infos_table[i].replica_info.addr);
        INFO_LOG("replica_port %d", total_config_ptr->server_infos_table[i].replica_info.port);
    }
    INFO_LOG("-------------------------------------------------");
    INFO_LOG("group cnt = %d", total_config_ptr->groups_cnt);
    for (i = 0; i < total_config_ptr->groups_cnt; i++) {
        INFO_LOG("group id %d", total_config_ptr->group_infos_table[i].group_id);
        INFO_LOG("  group member cnt %d", total_config_ptr->group_infos_table[i].member_cnt);
        for (int j = 0; j < total_config_ptr->group_infos_table[i].member_cnt; j++) {
            INFO_LOG("  group member id %d", total_config_ptr->group_infos_table[i].member_ids[j]);
        }
        INFO_LOG("-------------------------------------------------");
    }
}

static int dhmp_parse_watcher_node(struct dhmp_config *config_ptr, int index, xmlDocPtr doc, xmlNodePtr curnode) {
    xmlChar *val;
    int log_level = 0;

    curnode = curnode->xmlChildrenNode;
    while (curnode != NULL) {
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_ADDR_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            memcpy(config_ptr->watcher_addr, (const void *)val, strlen((const char *)val) + 1);
            xmlFree(val);
        }

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_PORT_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            config_ptr->watcher_port = atoi((const char *)val);
            xmlFree(val);
        }

        curnode = curnode->next;
    }

    return 0;
}

static int dhmp_parse_client_node(struct dhmp_config *config_ptr, int index, xmlDocPtr doc, xmlNodePtr curnode) {
    xmlChar *val;
    int log_level = 0;

    curnode = curnode->xmlChildrenNode;
    while (curnode != NULL) {
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_LOG_LEVEL_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            log_level = atoi((const char *)val);
            global_log_level = (enum dhmp_log_level)log_level;
            xmlFree(val);
        }

        curnode = curnode->next;
    }

    return 0;
}

static int dhmp_parse_server_node(struct dhmp_config *config_ptr, int index, xmlDocPtr doc, xmlNodePtr curnode) {
    config_ptr->nets_cnt++;

    xmlChar *val;
    val = xmlGetProp(curnode, (const xmlChar *)DHMP_XML_ATTR_ID_STR);
    int server_id = atoi((const char *)val);
    config_ptr->server_infos_table[server_id].server_id = server_id;
    xmlFree(val);

    curnode = curnode->xmlChildrenNode;
    while (curnode != NULL) {
        // net_info
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_NIC_NAME_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            memcpy(config_ptr->server_infos_table[server_id].net_info.nic_name, (const void *)val,
                   strlen((const char *)val) + 1);
            xmlFree(val);
        }

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_ADDR_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            memcpy(config_ptr->server_infos_table[server_id].net_info.addr, (const void *)val,
                   strlen((const char *)val) + 1);
            xmlFree(val);
        }

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_PORT_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            config_ptr->server_infos_table[server_id].net_info.port = atoi((const char *)val);
            xmlFree(val);
        }

        // replica_info
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_REPLICA_NIC_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            memcpy(config_ptr->server_infos_table[server_id].replica_info.nic_name, (const void *)val,
                   strlen((const char *)val) + 1);
            xmlFree(val);
        }
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_REPLICA_ADDR_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            memcpy(config_ptr->server_infos_table[server_id].replica_info.addr, (const void *)val,
                   strlen((const char *)val) + 1);
            xmlFree(val);
        }
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_REPLICA_PORT_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            config_ptr->server_infos_table[server_id].replica_info.port = atoi((const char *)val);
            xmlFree(val);
        }

        curnode = curnode->next;
    }
    return 0;
}

static int dhmp_parse_group_node(struct dhmp_config *config_ptr, int index, xmlDocPtr doc, xmlNodePtr curnode) {
    config_ptr->groups_cnt++;

    xmlChar *val;
    val = xmlGetProp(curnode, (const xmlChar *)DHMP_XML_ATTR_ID_STR);
    int group_id = atoi((const char *)val);
    config_ptr->group_infos_table[group_id].group_id = group_id;
    xmlFree(val);

    curnode = curnode->xmlChildrenNode;
    while (curnode != NULL) {
        // level : group / server
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_SERVER_STR)) {
            val = xmlNodeListGetString(doc, curnode->xmlChildrenNode, 1);
            int server_id = atoi((const char *)val);
            config_ptr->group_infos_table[group_id].member_ids[config_ptr->group_infos_table[group_id].member_cnt] =
                server_id;
            config_ptr->group_infos_table[group_id].member_cnt++;
            xmlFree(val);
        }
        curnode = curnode->next;
    }

    return 0;
}

static void dhmp_set_curnode_id(struct dhmp_config *config_ptr) {
    int socketfd, i, k, dev_num;
    char buf[BUFSIZ];
    const char *addr = NULL, *replica_addr = NULL;
    struct ifconf conf;
    struct ifreq *ifr;
    struct sockaddr_in *sin;

    socketfd = socket(PF_INET, SOCK_DGRAM, 0);
    conf.ifc_len = BUFSIZ;
    conf.ifc_buf = buf;

    ioctl(socketfd, SIOCGIFCONF, &conf);
    dev_num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    bool valid = false;
    for (k = 0; k < config_ptr->nets_cnt && !valid; k++) {
        for (i = 0; i < dev_num && !valid; i++, ifr++) {
            sin = (struct sockaddr_in *)(&ifr->ifr_addr);
            // find the addr of local nic that corresponding to config xml
            ioctl(socketfd, SIOCGIFFLAGS, ifr);
            if (((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)) {
                if (addr == NULL && !strcmp(ifr->ifr_name, config_ptr->server_infos_table[k].net_info.nic_name)) {
                    INFO_LOG("%s %s", ifr->ifr_name, inet_ntoa(sin->sin_addr));
                    addr = inet_ntoa(sin->sin_addr);
                }
                if (!strcmp(ifr->ifr_name, config_ptr->server_infos_table[k].replica_info.nic_name)) {
                    INFO_LOG("%s %s", ifr->ifr_name, inet_ntoa(sin->sin_addr));
                    replica_addr = inet_ntoa(sin->sin_addr);
                }
            }
            if (addr != NULL && replica_addr != NULL) {
                for (i = 0; i < config_ptr->nets_cnt; i++) {
                    if (!strcmp(config_ptr->server_infos_table[i].net_info.addr, addr)) {
                        config_ptr->curnet_id = i;
                    }
                }
                valid = true;
            }
        }
    }

    if (k >= config_ptr->nets_cnt) {
        if (addr == NULL) {
            ERROR_LOG("can't find ip corresponding to NIC:%s", config_ptr->server_infos_table[k].net_info.nic_name);
        }
        if (replica_addr == NULL) {
            ERROR_LOG("can't find ip corresponding to NIC for replica:%s",
                      config_ptr->server_infos_table[k].replica_info.nic_name);
        }
        exit(-1);
    }
    // check whether the addr is corresponding to the config xml
}

//=============================== public methods ===============================
// TODO declaration in header
int dhmp_config_init(struct dhmp_config *config_ptr, bool is_client) {
    const char *config_file = DHMP_CONFIG_FILE_NAME;
    int index = 0; // will be assigned by server atrributes in xml
    xmlDocPtr config_doc;
    xmlNodePtr curnode;

    config_doc = xmlParseFile(config_file);
    if (!config_doc) {
        ERROR_LOG("xml parse file error.");
        return -1;
    }

    curnode = xmlDocGetRootElement(config_doc);
    if (!curnode) {
        ERROR_LOG("xml doc get root element error.");
        goto cleandoc;
    }

    if (xmlStrcmp(curnode->name, (const xmlChar *)DHMP_DHMP_CONFIG_STR)) {
        ERROR_LOG("xml root node is not dhmp_config string.");
        goto cleandoc;
    }

    config_ptr->nets_cnt = 0;
    config_ptr->curnet_id = -1;
    curnode = curnode->xmlChildrenNode;
    while (curnode) {
        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_WATCHER_STR)) {
            dhmp_parse_watcher_node(config_ptr, index, config_doc, curnode);
        }

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_CLIENT_STR))
            dhmp_parse_client_node(config_ptr, index, config_doc, curnode);

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_SERVER_STR)) {
            dhmp_parse_server_node(config_ptr, index, config_doc, curnode);
        }

        if (!xmlStrcmp(curnode->name, (const xmlChar *)DHMP_GROUP_STR)) {
            dhmp_parse_group_node(config_ptr, index, config_doc, curnode);
        }
        curnode = curnode->next;
    }

    dhmp_print_config(config_ptr);
    xmlFreeDoc(config_doc);
    if (!is_client)
        dhmp_set_curnode_id(config_ptr);
    return 0;

cleandoc:
    xmlFreeDoc(config_doc);
    return -1;
}
