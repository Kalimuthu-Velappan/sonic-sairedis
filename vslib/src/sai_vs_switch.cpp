#include "sai_vs.h"
#include "sai_vs_internal.h"

#include "swss/selectableevent.h"
#include "swss/netmsg.h"
#include "swss/select.h"

#include <thread>

using namespace saivs;

void update_port_oper_status(
        _In_ sai_object_id_t port_id,
        _In_ sai_port_oper_status_t port_oper_status)
{
    MUTEX();

    SWSS_LOG_ENTER();

    sai_attribute_t attr;

    attr.id = SAI_PORT_ATTR_OPER_STATUS;
    attr.value.s32 = port_oper_status;

    sai_status_t status = vs_generic_set(SAI_OBJECT_TYPE_PORT, port_id, &attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("failed to update port status %s: %s",
                sai_serialize_object_id(port_id).c_str(),
                sai_serialize_port_oper_status(port_oper_status).c_str());
    }
}

sai_status_t vs_create_switch(
        _Out_ sai_object_id_t *switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    sai_status_t status = meta_sai_create_oid(
            SAI_OBJECT_TYPE_SWITCH,
            switch_id,
            SAI_NULL_OBJECT_ID, // no switch id since we create switch
            attr_count,
            attr_list,
            &vs_generic_create);

    if (status == SAI_STATUS_SUCCESS)
    {
        auto sw = std::make_shared<Switch>(*switch_id, attr_count, attr_list);

        g_switchContainer->insert(sw);
    }

    return status;
}

sai_status_t vs_remove_switch(
            _In_ sai_object_id_t switch_id)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    sai_status_t status = meta_sai_remove_oid(
            SAI_OBJECT_TYPE_SWITCH,
            switch_id,
            &vs_generic_remove);

    if (status == SAI_STATUS_SUCCESS)
    {
        // remove switch from container

        g_switchContainer->removeSwitch(switch_id);
    }

    return status;
}

sai_status_t vs_set_switch_attribute(
        _In_ sai_object_id_t switch_id,
        _In_ const sai_attribute_t *attr)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    sai_status_t status = meta_sai_set_oid(
            SAI_OBJECT_TYPE_SWITCH,
            switch_id,
            attr,
            &vs_generic_set);

    if (status == SAI_STATUS_SUCCESS)
    {
        auto sw = g_switchContainer->getSwitch(switch_id);

        if (!sw)
        {
            SWSS_LOG_THROW("failed to find switch %s in container",
                    sai_serialize_object_id(switch_id).c_str());
        }

        /*
         * When doing SET operation user may want to update notification
         * pointers.
         */
        sw->updateNotifications(1, attr);
    }

    return status;
}

VS_GET(SWITCH,switch);
VS_GENERIC_STATS(SWITCH, switch);

const sai_switch_api_t vs_switch_api = {

    vs_create_switch,
    vs_remove_switch,
    vs_set_switch_attribute,
    vs_get_switch_attribute,
    VS_GENERIC_STATS_API(switch)
};
