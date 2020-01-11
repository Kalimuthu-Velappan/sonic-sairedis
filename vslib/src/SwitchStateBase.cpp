#include "SwitchStateBase.h"

#include "swss/logger.h"

#include "sai_vs.h" // TODO to be removed

#include <algorithm>

using namespace saivs;

SwitchStateBase::SwitchStateBase(
                    _In_ sai_object_id_t switch_id):
    SwitchState(switch_id)
{
    SWSS_LOG_ENTER();

    // empty
}

sai_status_t SwitchStateBase::create(
        _In_ sai_object_type_t object_type,
        _Out_ sai_object_id_t *object_id,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    *object_id = g_realObjectIdManager->allocateNewObjectId(object_type, switch_id);

    auto sid = sai_serialize_object_id(*object_id);

    return create(object_type, sid, switch_id, attr_count, attr_list);
}

sai_status_t SwitchStateBase::create(
        _In_ sai_object_type_t object_type,
        _In_ const std::string &serializedObjectId,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto &objectHash = g_switch_state_map.at(switch_id)->m_objectHash.at(object_type);

    auto it = objectHash.find(serializedObjectId);

    if (object_type != SAI_OBJECT_TYPE_SWITCH)
    {
        /*
         * Switch is special, and object is already created by init.
         *
         * XXX revisit this.
         */

        if (it != objectHash.end())
        {
            SWSS_LOG_ERROR("create failed, object already exists, object type: %s: id: %s",
                    sai_serialize_object_type(object_type).c_str(),
                    serializedObjectId.c_str());

            return SAI_STATUS_ITEM_ALREADY_EXISTS;
        }
    }

    if (objectHash.find(serializedObjectId) == objectHash.end())
    {
        /*
         * Number of attributes may be zero, so see if actual entry was created
         * with empty hash.
         */

        objectHash[serializedObjectId] = {};
    }

    for (uint32_t i = 0; i < attr_count; ++i)
    {
        auto a = std::make_shared<SaiAttrWrap>(object_type, &attr_list[i]);

        objectHash[serializedObjectId][a->getAttrMetadata()->attridname] = a;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::remove(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto sid = sai_serialize_object_id(objectId);

    return remove(object_type, sid);
}

sai_status_t SwitchStateBase::remove(
        _In_ sai_object_type_t object_type,
        _In_ const std::string &serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto &objectHash = m_objectHash.at(object_type);

    auto it = objectHash.find(serializedObjectId);

    if (it == objectHash.end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                sai_serialize_object_type(object_type).c_str(),
                serializedObjectId.c_str());

        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    objectHash.erase(it);

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::set(
        _In_ sai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const sai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    auto it = m_objectHash.at(objectType).find(serializedObjectId);

    if (it == m_objectHash.at(objectType).end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                sai_serialize_object_type(objectType).c_str(),
                serializedObjectId.c_str());

        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    auto &attrHash = it->second;

    auto a = std::make_shared<SaiAttrWrap>(objectType, attr);

    // set have only one attribute
    attrHash[a->getAttrMetadata()->attridname] = a;

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::set(
        _In_ sai_object_type_t objectType,
        _In_ sai_object_id_t objectId,
        _In_ const sai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    auto sid = sai_serialize_object_id(objectId);

    return set(objectType, sid, attr);
}

sai_status_t SwitchStateBase::get(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t attr_count,
        _Out_ sai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto sid = sai_serialize_object_id(object_id);

    return get(object_type, sid, attr_count, attr_list);
}

sai_status_t SwitchStateBase::get(
        _In_ sai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ uint32_t attr_count,
        _Out_ sai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto &objectHash = m_objectHash.at(objectType);

    auto it = objectHash.find(serializedObjectId);

    if (it == objectHash.end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                sai_serialize_object_type(objectType).c_str(),
                serializedObjectId.c_str());

        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    /*
     * We need reference here since we can potentially update attr hash for RO
     * object.
     */

    auto& attrHash = it->second;

    /*
     * Some of the list query maybe for length, so we can't do
     * normal serialize, maybe with count only.
     */

    sai_status_t final_status = SAI_STATUS_SUCCESS;

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        sai_attr_id_t id = attr_list[idx].id;

        auto meta = sai_metadata_get_attr_metadata(objectType, id);

        if (meta == NULL)
        {
            SWSS_LOG_ERROR("failed to find attribute %d for %s:%s", id,
                    sai_serialize_object_type(objectType).c_str(),
                    serializedObjectId.c_str());

            return SAI_STATUS_FAILURE;
        }

        sai_status_t status;

        if (SAI_HAS_FLAG_READ_ONLY(meta->flags))
        {
            /*
             * Read only attributes may require recalculation.
             * Metadata makes sure that non object id's can't have
             * read only attributes. So here is definitely OID.
             */

            sai_object_id_t oid;
            sai_deserialize_object_id(serializedObjectId, oid);

            status = refresh_read_only(meta, oid);

            if (status != SAI_STATUS_SUCCESS)
            {
                SWSS_LOG_ERROR("%s read only not implemented on %s",
                        meta->attridname,
                        serializedObjectId.c_str());

                return status;
            }
        }

        auto ait = attrHash.find(meta->attridname);

        if (ait == attrHash.end())
        {
            SWSS_LOG_ERROR("%s not implemented on %s",
                    meta->attridname,
                    serializedObjectId.c_str());

            return SAI_STATUS_NOT_IMPLEMENTED;
        }

        auto attr = ait->second->getAttr();

        status = transfer_attributes(objectType, 1, attr, &attr_list[idx], false);

        if (status == SAI_STATUS_BUFFER_OVERFLOW)
        {
            /*
             * This is considered partial success, since we get correct list
             * length.  Note that other items ARE processes on the list.
             */

            SWSS_LOG_NOTICE("BUFFER_OVERFLOW %s: %s",
                    serializedObjectId.c_str(),
                    meta->attridname);

            /*
             * We still continue processing other attributes for get as long as
             * we only will be getting buffer overflow error.
             */

            final_status = status;
            continue;
        }

        if (status != SAI_STATUS_SUCCESS)
        {
            // all other errors

            SWSS_LOG_ERROR("get failed %s: %s: %s",
                    serializedObjectId.c_str(),
                    meta->attridname,
                    sai_serialize_status(status).c_str());

            return status;
        }
    }

    return final_status;
}

sai_status_t SwitchStateBase::set_switch_mac_address()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create switch src mac address");

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;

    attr.value.mac[0] = 0x11;
    attr.value.mac[1] = 0x22;
    attr.value.mac[2] = 0x33;
    attr.value.mac[3] = 0x44;
    attr.value.mac[4] = 0x55;
    attr.value.mac[5] = 0x66;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::set_switch_default_attributes()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create switch default attributes");

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    attr.value.ptr = NULL;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_FDB_AGING_TIME;
    attr.value.u32 = 0;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_RESTART_WARM;
    attr.value.booldata = false;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_WARM_RECOVER;
    attr.value.booldata = false;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_default_vlan()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create default vlan");

    sai_attribute_t attr;

    attr.id = SAI_VLAN_ATTR_VLAN_ID;
    attr.value.u16 = DEFAULT_VLAN_NUMBER;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_VLAN, &m_default_vlan_id, m_switch_id, 1, &attr));

    /* set default vlan on switch */

    attr.id = SAI_SWITCH_ATTR_DEFAULT_VLAN_ID;
    attr.value.oid = m_default_vlan_id;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_cpu_port()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create cpu port");

    sai_attribute_t attr;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_PORT, &m_cpu_port_id, m_switch_id, 0, &attr));

    // populate cpu port object on switch
    attr.id = SAI_SWITCH_ATTR_CPU_PORT;
    attr.value.oid = m_cpu_port_id;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    // set type on cpu
    attr.id = SAI_PORT_ATTR_TYPE;
    attr.value.s32 = SAI_PORT_TYPE_CPU;

    return set(SAI_OBJECT_TYPE_PORT, m_cpu_port_id, &attr);
}

sai_status_t SwitchStateBase::create_default_1q_bridge()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create default 1q bridge");

    sai_attribute_t attr;

    attr.id = SAI_BRIDGE_ATTR_TYPE;
    attr.value.s32 = SAI_BRIDGE_TYPE_1Q;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_BRIDGE, &m_default_1q_bridge, m_switch_id, 1, &attr));

    attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;
    attr.value.oid = m_default_1q_bridge;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_ports()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create ports");

    std::vector<std::vector<uint32_t>> laneMap;

    getPortLaneMap(laneMap); // TODO per switch !! on constructor !

    uint32_t port_count = (uint32_t)laneMap.size();

    m_port_list.clear();

    for (uint32_t i = 0; i < port_count; i++)
    {
        SWSS_LOG_DEBUG("create port index %u", i);

        sai_object_id_t port_id;

        CHECK_STATUS(create(SAI_OBJECT_TYPE_PORT, &port_id, m_switch_id, 0, NULL));

        m_port_list.push_back(port_id);

        sai_attribute_t attr;

        attr.id = SAI_PORT_ATTR_ADMIN_STATE;
        attr.value.booldata = false;     /* default admin state is down as defined in SAI */

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        attr.id = SAI_PORT_ATTR_MTU;
        attr.value.u32 = 1514;     /* default MTU is 1514 as defined in SAI */

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        attr.id = SAI_PORT_ATTR_SPEED;
        attr.value.u32 = 40 * 1000;     /* TODO from config */

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        std::vector<uint32_t> lanes = laneMap.at(i);

        attr.id = SAI_PORT_ATTR_HW_LANE_LIST;
        attr.value.u32list.count = (uint32_t)lanes.size();
        attr.value.u32list.list = lanes.data();

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        attr.id = SAI_PORT_ATTR_TYPE;
        attr.value.s32 = SAI_PORT_TYPE_LOGICAL;

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        attr.id = SAI_PORT_ATTR_OPER_STATUS;
        attr.value.s32 = SAI_PORT_OPER_STATUS_DOWN;

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

        attr.id = SAI_PORT_ATTR_PORT_VLAN_ID;
        attr.value.u32 = DEFAULT_VLAN_NUMBER;

        CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::set_port_list()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("set port list");

    /*
     * TODO this is static, when we start to "create/remove" ports we need to
     * update this list since it can change depends on profile.ini
     */

    sai_attribute_t attr;

    uint32_t port_count = (uint32_t)m_port_list.size();

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist.count = port_count;
    attr.value.objlist.list = m_port_list.data();

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;
    attr.value.u32 = port_count;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_default_virtual_router()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create default virtual router");

    sai_object_id_t virtual_router_id;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_VIRTUAL_ROUTER, &virtual_router_id, m_switch_id, 0, NULL));

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID;
    attr.value.oid = virtual_router_id;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_default_stp_instance()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create default stp instance");

    sai_object_id_t stp_instance_id;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_STP, &stp_instance_id, m_switch_id, 0, NULL));

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_DEFAULT_STP_INST_ID;
    attr.value.oid = stp_instance_id;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_default_trap_group()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create default trap group");

    sai_object_id_t trap_group_id;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_HOSTIF_TRAP_GROUP, &trap_group_id, m_switch_id, 0, NULL));

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_DEFAULT_TRAP_GROUP;
    attr.value.oid = trap_group_id;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_ingress_priority_groups_per_port(
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    const uint32_t port_pgs_count = 8; // TODO must be per switch (mlnx and brcm is 8)

    std::vector<sai_object_id_t> pgs;

    for (uint32_t i = 0; i < port_pgs_count; ++i)
    {
        sai_object_id_t pg_id;

        sai_attribute_t attr[3];

        // TODO on brcm this attribute is not added

        attr[0].id = SAI_INGRESS_PRIORITY_GROUP_ATTR_BUFFER_PROFILE;
        attr[0].value.oid = SAI_NULL_OBJECT_ID;

        /*
         * not in headers yet
         *
         * attr[1].id = SAI_INGRESS_PRIORITY_GROUP_ATTR_PORT;
         * attr[1].value.oid = port_id;

         * attr[2].id = SAI_INGRESS_PRIORITY_GROUP_ATTR_INDEX;
         * attr[2].value.oid = i;

         * TODO fix number of attributes
         */

        CHECK_STATUS(create(SAI_OBJECT_TYPE_INGRESS_PRIORITY_GROUP, &pg_id, m_switch_id, 1, attr));

        pgs.push_back(pg_id);
    }

    sai_attribute_t attr;

    attr.id = SAI_PORT_ATTR_NUMBER_OF_INGRESS_PRIORITY_GROUPS;
    attr.value.u32 = port_pgs_count;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

    attr.id = SAI_PORT_ATTR_INGRESS_PRIORITY_GROUP_LIST;
    attr.value.objlist.count = port_pgs_count;
    attr.value.objlist.list = pgs.data();

    CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::create_ingress_priority_groups()
{
    SWSS_LOG_ENTER();

    // TODO priority groups size may change when we will modify pg or ports

    SWSS_LOG_INFO("create ingress priority groups");

    for (auto &port_id : m_port_list)
    {
        create_ingress_priority_groups_per_port(port_id);
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::create_vlan_members()
{
    SWSS_LOG_ENTER();

    // Crete vlan members for bridge ports.

    for (auto bridge_port_id: m_bridge_port_list_port_based)
    {
        SWSS_LOG_DEBUG("create vlan member for bridge port %s",
                sai_serialize_object_id(bridge_port_id).c_str());

        sai_attribute_t attrs[3];

        attrs[0].id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
        attrs[0].value.oid = bridge_port_id;

        attrs[1].id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
        attrs[1].value.oid = m_default_vlan_id;

        attrs[2].id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;
        attrs[2].value.s32 = SAI_VLAN_TAGGING_MODE_UNTAGGED;

        sai_object_id_t vlan_member_id;

        CHECK_STATUS(create(SAI_OBJECT_TYPE_VLAN_MEMBER, &vlan_member_id, m_switch_id, 3, attrs));
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::create_bridge_ports()
{
    SWSS_LOG_ENTER();

    // Create bridge port for 1q router.

    sai_attribute_t attr;

    attr.id = SAI_BRIDGE_PORT_ATTR_TYPE;
    attr.value.s32 = SAI_BRIDGE_PORT_TYPE_1Q_ROUTER;

    CHECK_STATUS(create(SAI_OBJECT_TYPE_BRIDGE_PORT, &m_default_bridge_port_1q_router, m_switch_id, 1, &attr));

    attr.id = SAI_BRIDGE_PORT_ATTR_PORT_ID;
    attr.value.oid = SAI_NULL_OBJECT_ID;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_BRIDGE_PORT, m_default_bridge_port_1q_router, &attr));

    // Create bridge ports for regular ports.

    m_bridge_port_list_port_based.clear();

    for (const auto &port_id: m_port_list)
    {
        SWSS_LOG_DEBUG("create bridge port for port %s", sai_serialize_object_id(port_id).c_str());

        sai_attribute_t attrs[4];

        attrs[0].id = SAI_BRIDGE_PORT_ATTR_BRIDGE_ID;
        attrs[0].value.oid = m_default_1q_bridge;

        attrs[1].id = SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE;
        attrs[1].value.s32 = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;

        attrs[2].id = SAI_BRIDGE_PORT_ATTR_PORT_ID;
        attrs[2].value.oid = port_id;

        attrs[3].id = SAI_BRIDGE_PORT_ATTR_TYPE;
        attrs[3].value.s32 = SAI_BRIDGE_PORT_TYPE_PORT;

        sai_object_id_t bridge_port_id;

        CHECK_STATUS(create(SAI_OBJECT_TYPE_BRIDGE_PORT, &bridge_port_id, m_switch_id, 4, attrs));

        m_bridge_port_list_port_based.push_back(bridge_port_id);
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::set_acl_entry_min_prio()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("set acl entry min prio");

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY;
    attr.value.u32 = 1;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY;
    attr.value.u32 = 16000;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::set_acl_capabilities()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("set acl capabilities");

    sai_attribute_t attr;

    m_ingress_acl_action_list.clear();
    m_egress_acl_action_list.clear();

    for (int action_type = SAI_ACL_ENTRY_ATTR_ACTION_START; action_type <= SAI_ACL_ENTRY_ATTR_ACTION_END; action_type++)
    {
        m_ingress_acl_action_list.push_back(static_cast<sai_acl_action_type_t>(action_type - SAI_ACL_ENTRY_ATTR_ACTION_START));
        m_egress_acl_action_list.push_back(static_cast<sai_acl_action_type_t>(action_type - SAI_ACL_ENTRY_ATTR_ACTION_START));
    }

    attr.id = SAI_SWITCH_ATTR_MAX_ACL_ACTION_COUNT;
    attr.value.u32 = static_cast<uint32_t>(std::max(m_ingress_acl_action_list.size(), m_egress_acl_action_list.size()));

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_ACL_STAGE_INGRESS;
    attr.value.aclcapability.action_list.list = reinterpret_cast<int32_t*>(m_ingress_acl_action_list.data());
    attr.value.aclcapability.action_list.count = static_cast<uint32_t>(m_ingress_acl_action_list.size());

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_ACL_STAGE_EGRESS;
    attr.value.aclcapability.action_list.list = reinterpret_cast<int32_t*>(m_egress_acl_action_list.data());
    attr.value.aclcapability.action_list.count = static_cast<uint32_t>(m_egress_acl_action_list.size());

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::create_qos_queues_per_port(
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::create_qos_queues()
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::create_scheduler_group_tree(
        _In_ const std::vector<sai_object_id_t>& sgs,
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::create_scheduler_groups_per_port(
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::create_scheduler_groups()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create scheduler groups");

    // TODO scheduler groups size may change when we will modify sg or ports

    for (auto &port_id : m_port_list)
    {
        CHECK_STATUS(create_scheduler_groups_per_port(port_id));
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::set_maximum_number_of_childs_per_scheduler_group()
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::set_number_of_ecmp_groups()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("set number of ecmp groups");

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_NUMBER_OF_ECMP_GROUPS;
    attr.value.u32 = 512;

    return set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr);
}

sai_status_t SwitchStateBase::initialize_default_objects()
{
    SWSS_LOG_ENTER();

    CHECK_STATUS(set_switch_mac_address());
    CHECK_STATUS(create_cpu_port());
    CHECK_STATUS(create_default_vlan());
    CHECK_STATUS(create_default_virtual_router());
    CHECK_STATUS(create_default_stp_instance());
    CHECK_STATUS(create_default_1q_bridge());
    CHECK_STATUS(create_default_trap_group());
    CHECK_STATUS(create_ports());
    CHECK_STATUS(set_port_list());
    CHECK_STATUS(create_bridge_ports());
    CHECK_STATUS(create_vlan_members());
    CHECK_STATUS(set_acl_entry_min_prio());
    CHECK_STATUS(set_acl_capabilities());
    CHECK_STATUS(create_ingress_priority_groups());
    CHECK_STATUS(create_qos_queues());
    CHECK_STATUS(set_maximum_number_of_childs_per_scheduler_group());
    CHECK_STATUS(set_number_of_ecmp_groups());
    CHECK_STATUS(set_switch_default_attributes());
    CHECK_STATUS(create_scheduler_groups());

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::create_port(
        _In_ sai_object_id_t port_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_WARN("check attributes and set, FIXME");

    // this method is post create action on generic create object

    sai_attribute_t attr;

    // default admin state is down as defined in SAI

    attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    attr.value.booldata = false;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_PORT, port_id, &attr));

    // attributes are not required since they will be set outside this function

    CHECK_STATUS(create_ingress_priority_groups_per_port(port_id));
    CHECK_STATUS(create_qos_queues_per_port(port_id));
    CHECK_STATUS(create_scheduler_groups_per_port(port_id));

    // TODO should bridge ports should also be created when new port is created?
    // this needs to be checked on real ASIC and updated here

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::refresh_ingress_priority_group(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    /*
     * TODO Currently we don't have index in groups, so we don't know how to
     * sort.  Returning success, since assuming that we will not create more
     * ingress priority groups.
     */

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::refresh_qos_queues(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    /*
     * TODO Currently we don't have index in groups, so we don't know how to
     * sort.  Returning success, since assuming that we will not create more
     * ingress priority groups.
     */

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::refresh_scheduler_groups(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t port_id)
{
    SWSS_LOG_ENTER();

    /*
     * TODO Currently we don't have index in groups, so we don't know how to
     * sort.  Returning success, since assuming that we will not create more
     * ingress priority groups.
     */

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::warm_boot_initialize_objects()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("warm boot init objects");

    /*
     * We need to bring back previous state in case user will get some read
     * only attributes and recalculation will need to be done.
     *
     * We need to refresh:
     * - ports
     * - default bridge port 1q router
     */

    m_port_list.resize(SAI_VS_MAX_PORTS);

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;

    attr.value.objlist.count = SAI_VS_MAX_PORTS;
    attr.value.objlist.list = m_port_list.data();

    CHECK_STATUS(get(SAI_OBJECT_TYPE_SWITCH, m_switch_id, 1, &attr));

    m_port_list.resize(attr.value.objlist.count);

    SWSS_LOG_NOTICE("port list size: %zu", m_port_list.size());

    attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;

    CHECK_STATUS(get(SAI_OBJECT_TYPE_SWITCH, m_switch_id, 1, &attr));

    m_default_bridge_port_1q_router = attr.value.oid;

    SWSS_LOG_NOTICE("default bridge port 1q router: %s",
            sai_serialize_object_id(m_default_bridge_port_1q_router).c_str());

    // TODO refresh
    //
    // m_bridge_port_list_port_based;
    // m_cpu_port_id;
    // m_default_1q_bridge;
    // m_default_vlan_id;
    //
    // other objects/numbers if added (per switch also)

    return SAI_STATUS_SUCCESS;
}

sai_status_t SwitchStateBase::refresh_bridge_port_list(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t bridge_id)
{
    SWSS_LOG_ENTER();

    // TODO this method can be abstract, FIXME

    SWSS_LOG_ERROR("implement in child class");

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t SwitchStateBase::refresh_vlan_member_list(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t vlan_id)
{
    SWSS_LOG_ENTER();

    auto &all_vlan_members = m_objectHash.at(SAI_OBJECT_TYPE_VLAN_MEMBER);

    auto m_member_list = sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_VLAN, SAI_VLAN_ATTR_MEMBER_LIST);
    auto md_vlan_id = sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_VLAN_MEMBER, SAI_VLAN_MEMBER_ATTR_VLAN_ID);
    //auto md_brportid = sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_VLAN_MEMBER, SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID);

    std::vector<sai_object_id_t> vlan_member_list;

    /*
     * We want order as bridge port order (so port order)
     */

    sai_attribute_t attr;

    auto me = m_objectHash.at(SAI_OBJECT_TYPE_VLAN).at(sai_serialize_object_id(vlan_id));

    for (auto vm: all_vlan_members)
    {
        if (vm.second.at(md_vlan_id->attridname)->getAttr()->value.oid != vlan_id)
        {
            /*
             * Only interested in our vlan
             */

            continue;
        }

        // TODO we need order as bridge ports, but we need bridge id!

        {
            sai_object_id_t vlan_member_id;

            sai_deserialize_object_id(vm.first, vlan_member_id);

            vlan_member_list.push_back(vlan_member_id);
        }
    }

    uint32_t vlan_member_list_count = (uint32_t)vlan_member_list.size();

    SWSS_LOG_NOTICE("recalculated %s: %u", m_member_list->attridname, vlan_member_list_count);

    attr.id = SAI_VLAN_ATTR_MEMBER_LIST;
    attr.value.objlist.count = vlan_member_list_count;
    attr.value.objlist.list = vlan_member_list.data();

    return set(SAI_OBJECT_TYPE_VLAN, vlan_id, &attr);
}

sai_status_t SwitchStateBase::refresh_port_list(
        _In_ const sai_attr_metadata_t *meta)
{
    SWSS_LOG_ENTER();

    // since now port can be added or removed, we need to update port list
    // dynamically

    sai_attribute_t attr;

    attr.id = SAI_SWITCH_ATTR_CPU_PORT;

    CHECK_STATUS(get(SAI_OBJECT_TYPE_SWITCH, m_switch_id, 1, &attr));

    const sai_object_id_t cpu_port_id = attr.value.oid;

    m_port_list.clear();

    // iterate via ASIC state to find all the ports

    for (const auto& it: m_objectHash.at(SAI_OBJECT_TYPE_PORT))
    {
        sai_object_id_t port_id;
        sai_deserialize_object_id(it.first, port_id);

        // don't put CPU port id on the list

        if (port_id == cpu_port_id)
            continue;

        m_port_list.push_back(port_id);
    }

    /*
     * TODO:
     *
     * Currently we don't know what's happen on brcm SAI implementation when
     * port is removed and then added, will new port could get the same vendor
     * OID or always different, and what is order of those new oids on the
     * PORT_LIST attribute.
     *
     * This needs to be investigated, and to reflect exact behaviour here.
     * Currently we just sort all the port oids.
     */

    std::sort(m_port_list.begin(), m_port_list.end());

    uint32_t port_count = (uint32_t)m_port_list.size();

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist.count = port_count;
    attr.value.objlist.list = m_port_list.data();

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;
    attr.value.u32 = port_count;

    CHECK_STATUS(set(SAI_OBJECT_TYPE_SWITCH, m_switch_id, &attr));

    SWSS_LOG_NOTICE("refreshed port list, current port number: %zu, not counting cpu port", m_port_list.size());

    return SAI_STATUS_SUCCESS;
}

// TODO extra work may be needed on GET api if N on list will be > then actual

/*
 * We can use local variable here for initialization (init should be in class
 * constructor anyway, we can move it there later) because each switch init is
 * done under global lock.
 */

/*
 * NOTE For recalculation we can add flag on create/remove specific object type
 * so we can deduce whether actually need to perform recalculation, as
 * optimization.
 */

sai_status_t SwitchStateBase::refresh_read_only(
        _In_ const sai_attr_metadata_t *meta,
        _In_ sai_object_id_t object_id)
{
    SWSS_LOG_ENTER();

    if (meta->objecttype == SAI_OBJECT_TYPE_SWITCH)
    {
        switch (meta->attrid)
        {
            case SAI_SWITCH_ATTR_CPU_PORT:
            case SAI_SWITCH_ATTR_DEFAULT_VIRTUAL_ROUTER_ID:
            case SAI_SWITCH_ATTR_DEFAULT_TRAP_GROUP:
            case SAI_SWITCH_ATTR_DEFAULT_VLAN_ID:
            case SAI_SWITCH_ATTR_DEFAULT_STP_INST_ID:
            case SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID:
                return SAI_STATUS_SUCCESS;

            case SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY:
            case SAI_SWITCH_ATTR_ACL_ENTRY_MAXIMUM_PRIORITY:
                return SAI_STATUS_SUCCESS;

            case SAI_SWITCH_ATTR_MAX_ACL_ACTION_COUNT:
            case SAI_SWITCH_ATTR_ACL_STAGE_INGRESS:
            case SAI_SWITCH_ATTR_ACL_STAGE_EGRESS:
                return SAI_STATUS_SUCCESS;

            case SAI_SWITCH_ATTR_NUMBER_OF_ECMP_GROUPS:
                return SAI_STATUS_SUCCESS;

            case SAI_SWITCH_ATTR_PORT_NUMBER:
            case SAI_SWITCH_ATTR_PORT_LIST:
                return refresh_port_list(meta);

            case SAI_SWITCH_ATTR_QOS_MAX_NUMBER_OF_CHILDS_PER_SCHEDULER_GROUP:
                return SAI_STATUS_SUCCESS;
        }
    }

    if (meta->objecttype == SAI_OBJECT_TYPE_PORT)
    {
        switch (meta->attrid)
        {
            case SAI_PORT_ATTR_QOS_NUMBER_OF_QUEUES:
            case SAI_PORT_ATTR_QOS_QUEUE_LIST:
                return refresh_qos_queues(meta, object_id);

            case SAI_PORT_ATTR_NUMBER_OF_INGRESS_PRIORITY_GROUPS:
            case SAI_PORT_ATTR_INGRESS_PRIORITY_GROUP_LIST:
                return refresh_ingress_priority_group(meta, object_id);

            case SAI_PORT_ATTR_QOS_NUMBER_OF_SCHEDULER_GROUPS:
            case SAI_PORT_ATTR_QOS_SCHEDULER_GROUP_LIST:
                return refresh_scheduler_groups(meta, object_id);

                /*
                 * This status is based on hostif vEthernetX status.
                 */

            case SAI_PORT_ATTR_OPER_STATUS:
                return SAI_STATUS_SUCCESS;
        }
    }

    if (meta->objecttype == SAI_OBJECT_TYPE_SCHEDULER_GROUP)
    {
        switch (meta->attrid)
        {
            case SAI_SCHEDULER_GROUP_ATTR_CHILD_COUNT:
            case SAI_SCHEDULER_GROUP_ATTR_CHILD_LIST:
                return refresh_scheduler_groups(meta, object_id);
        }
    }

    if (meta->objecttype == SAI_OBJECT_TYPE_BRIDGE && meta->attrid == SAI_BRIDGE_ATTR_PORT_LIST)
    {
        return refresh_bridge_port_list(meta, object_id);
    }

    if (meta->objecttype == SAI_OBJECT_TYPE_VLAN && meta->attrid == SAI_VLAN_ATTR_MEMBER_LIST)
    {
        return refresh_vlan_member_list(meta, object_id);
    }

    if (meta->objecttype == SAI_OBJECT_TYPE_DEBUG_COUNTER && meta->attrid == SAI_DEBUG_COUNTER_ATTR_INDEX)
    {
        return SAI_STATUS_SUCCESS;
    }

    if (meta_unittests_enabled())
    {
        SWSS_LOG_NOTICE("unittests enabled, SET could be performed on %s, not recalculating", meta->attridname);

        return SAI_STATUS_SUCCESS;
    }

    SWSS_LOG_WARN("need to recalculate RO: %s", meta->attridname);

    return SAI_STATUS_NOT_IMPLEMENTED;
}


