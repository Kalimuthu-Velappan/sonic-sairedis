#include "Meta.h"
#include "sai_meta.h"

#include "swss/logger.h"
#include "sai_serialize.h"
#include "OidRefCounter.h"
#include "Globals.h"
#include "SaiObjectCollection.h"

#include <inttypes.h>

#include <set>

#define CHECK_STATUS_SUCCESS(s) { if ((s) != SAI_STATUS_SUCCESS) return (s); }

using namespace saimeta;

extern OidRefCounter g_oids;
extern SaiObjectCollection g_saiObjectCollection;


// TODO to be moved to private member methods
sai_status_t meta_generic_validation_remove(
        _In_ const sai_object_meta_key_t& meta_key);

sai_status_t meta_sai_validate_oid(
        _In_ sai_object_type_t object_type,
        _In_ const sai_object_id_t* object_id,
        _In_ sai_object_id_t switch_id,
        _In_ bool create);

void meta_generic_validation_post_remove(
        _In_ const sai_object_meta_key_t& meta_key);

sai_status_t meta_sai_validate_fdb_entry(
        _In_ const sai_fdb_entry_t* fdb_entry,
        _In_ bool create,
        _In_ bool get = false);

sai_status_t meta_sai_validate_mcast_fdb_entry(
        _In_ const sai_mcast_fdb_entry_t* mcast_fdb_entry,
        _In_ bool create,
        _In_ bool get = false);

sai_status_t meta_sai_validate_neighbor_entry(
        _In_ const sai_neighbor_entry_t* neighbor_entry,
        _In_ bool create);

sai_status_t meta_sai_validate_route_entry(
        _In_ const sai_route_entry_t* route_entry,
        _In_ bool create);

sai_status_t meta_sai_validate_l2mc_entry(
        _In_ const sai_l2mc_entry_t* l2mc_entry,
        _In_ bool create);

sai_status_t meta_sai_validate_ipmc_entry(
        _In_ const sai_ipmc_entry_t* ipmc_entry,
        _In_ bool create);

sai_status_t meta_sai_validate_nat_entry(
        _In_ const sai_nat_entry_t* nat_entry,
        _In_ bool create);

sai_status_t meta_sai_validate_inseg_entry(
        _In_ const sai_inseg_entry_t* inseg_entry,
        _In_ bool create);

sai_status_t meta_generic_validation_create(
        _In_ const sai_object_meta_key_t& meta_key,
        _In_ sai_object_id_t switch_id,
        _In_ const uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list);

sai_status_t meta_generic_validation_set(
        _In_ const sai_object_meta_key_t& meta_key,
        _In_ const sai_attribute_t *attr);

sai_status_t meta_generic_validation_get(
        _In_ const sai_object_meta_key_t& meta_key,
        _In_ const uint32_t attr_count,
        _In_ sai_attribute_t *attr_list);

void meta_generic_validation_post_get(
        _In_ const sai_object_meta_key_t& meta_key,
        _In_ sai_object_id_t switch_id,
        _In_ const uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list);

sai_status_t meta_generic_validation_objlist(
        _In_ const sai_attr_metadata_t& md,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t count,
        _In_ const sai_object_id_t* list);

sai_status_t meta_genetic_validation_list(
        _In_ const sai_attr_metadata_t& md,
        _In_ uint32_t count,
        _In_ const void* list);

sai_status_t Meta::remove(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_oid(object_type, &object_id, SAI_NULL_OBJECT_ID, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(object_type, object_id);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_fdb_entry_t* fdb_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_fdb_entry(fdb_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_FDB_ENTRY, .objectkey = { .key = { .fdb_entry = *fdb_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(fdb_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_mcast_fdb_entry_t* mcast_fdb_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_mcast_fdb_entry(mcast_fdb_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_MCAST_FDB_ENTRY, .objectkey = { .key = { .mcast_fdb_entry = *mcast_fdb_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(mcast_fdb_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_neighbor_entry_t* neighbor_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_neighbor_entry(neighbor_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, .objectkey = { .key = { .neighbor_entry = *neighbor_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(neighbor_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_route_entry_t* route_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_route_entry(route_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_ROUTE_ENTRY, .objectkey = { .key = { .route_entry = *route_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(route_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_l2mc_entry_t* l2mc_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_l2mc_entry(l2mc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_L2MC_ENTRY, .objectkey = { .key = { .l2mc_entry = *l2mc_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(l2mc_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_ipmc_entry_t* ipmc_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_ipmc_entry(ipmc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_IPMC_ENTRY, .objectkey = { .key = { .ipmc_entry = *ipmc_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(ipmc_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_nat_entry_t* nat_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_nat_entry(nat_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NAT_ENTRY, .objectkey = { .key = { .nat_entry = *nat_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(nat_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
      meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::remove(
        _In_ const sai_inseg_entry_t* inseg_entry,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_inseg_entry(inseg_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_INSEG_ENTRY, .objectkey = { .key = { .inseg_entry = *inseg_entry  } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.remove(inseg_entry);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_fdb_entry_t* fdb_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_fdb_entry(fdb_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_FDB_ENTRY, .objectkey = { .key = { .fdb_entry = *fdb_entry  } } };

    status = meta_generic_validation_create(meta_key, fdb_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(fdb_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, fdb_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_mcast_fdb_entry_t* mcast_fdb_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_mcast_fdb_entry(mcast_fdb_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_MCAST_FDB_ENTRY, .objectkey = { .key = { .mcast_fdb_entry = *mcast_fdb_entry  } } };

    status = meta_generic_validation_create(meta_key, mcast_fdb_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(mcast_fdb_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, mcast_fdb_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_neighbor_entry_t* neighbor_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_neighbor_entry(neighbor_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, .objectkey = { .key = { .neighbor_entry = *neighbor_entry  } } };

    status = meta_generic_validation_create(meta_key, neighbor_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(neighbor_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, neighbor_entry->switch_id, attr_count, attr_list);
    }

    return status;
}
sai_status_t Meta::create(
        _In_ const sai_route_entry_t* route_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_route_entry(route_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_ROUTE_ENTRY, .objectkey = { .key = { .route_entry = *route_entry  } } };

    status = meta_generic_validation_create(meta_key, route_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(route_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, route_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_l2mc_entry_t* l2mc_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_l2mc_entry(l2mc_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_L2MC_ENTRY, .objectkey = { .key = { .l2mc_entry = *l2mc_entry  } } };

    status = meta_generic_validation_create(meta_key, l2mc_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(l2mc_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, l2mc_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_ipmc_entry_t* ipmc_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_ipmc_entry(ipmc_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_IPMC_ENTRY, .objectkey = { .key = { .ipmc_entry = *ipmc_entry  } } };

    status = meta_generic_validation_create(meta_key, ipmc_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(ipmc_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, ipmc_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_inseg_entry_t* inseg_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_inseg_entry(inseg_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_INSEG_ENTRY, .objectkey = { .key = { .inseg_entry = *inseg_entry  } } };

    status = meta_generic_validation_create(meta_key, inseg_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(inseg_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, inseg_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ const sai_nat_entry_t* nat_entry,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_nat_entry(nat_entry, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NAT_ENTRY, .objectkey = { .key = { .nat_entry = *nat_entry  } } };

    status = meta_generic_validation_create(meta_key, nat_entry->switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(nat_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_create(meta_key, nat_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_fdb_entry_t* fdb_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_fdb_entry(fdb_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_FDB_ENTRY, .objectkey = { .key = { .fdb_entry = *fdb_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(fdb_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_mcast_fdb_entry_t* mcast_fdb_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_mcast_fdb_entry(mcast_fdb_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_MCAST_FDB_ENTRY, .objectkey = { .key = { .mcast_fdb_entry = *mcast_fdb_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(mcast_fdb_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_neighbor_entry_t* neighbor_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_neighbor_entry(neighbor_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, .objectkey = { .key = { .neighbor_entry = *neighbor_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(neighbor_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_route_entry_t* route_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_route_entry(route_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_ROUTE_ENTRY, .objectkey = { .key = { .route_entry = *route_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(route_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_l2mc_entry_t* l2mc_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_l2mc_entry(l2mc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_L2MC_ENTRY, .objectkey = { .key = { .l2mc_entry = *l2mc_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(l2mc_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_ipmc_entry_t* ipmc_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_ipmc_entry(ipmc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_IPMC_ENTRY, .objectkey = { .key = { .ipmc_entry = *ipmc_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(ipmc_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_inseg_entry_t* inseg_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_inseg_entry(inseg_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_INSEG_ENTRY, .objectkey = { .key = { .inseg_entry = *inseg_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(inseg_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ const sai_nat_entry_t* nat_entry,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();
    sai_status_t status = meta_sai_validate_nat_entry(nat_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NAT_ENTRY, .objectkey = { .key = { .nat_entry = *nat_entry  } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(nat_entry, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_fdb_entry_t* fdb_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // NOTE: when doing get, entry may not exist on metadata db

    sai_status_t status = meta_sai_validate_fdb_entry(fdb_entry, false, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_FDB_ENTRY, .objectkey = { .key = { .fdb_entry = *fdb_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(fdb_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, fdb_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_mcast_fdb_entry_t* mcast_fdb_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // NOTE: when doing get, entry may not exist on metadata db

    sai_status_t status = meta_sai_validate_mcast_fdb_entry(mcast_fdb_entry, false, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_MCAST_FDB_ENTRY, .objectkey = { .key = { .mcast_fdb_entry = *mcast_fdb_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(mcast_fdb_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, mcast_fdb_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_neighbor_entry_t* neighbor_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_neighbor_entry(neighbor_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NEIGHBOR_ENTRY, .objectkey = { .key = { .neighbor_entry = *neighbor_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(neighbor_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, neighbor_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_route_entry_t* route_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_route_entry(route_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_ROUTE_ENTRY, .objectkey = { .key = { .route_entry = *route_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(route_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, route_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_l2mc_entry_t* l2mc_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_l2mc_entry(l2mc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_L2MC_ENTRY, .objectkey = { .key = { .l2mc_entry = *l2mc_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(l2mc_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, l2mc_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_ipmc_entry_t* ipmc_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_ipmc_entry(ipmc_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_IPMC_ENTRY, .objectkey = { .key = { .ipmc_entry = *ipmc_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(ipmc_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, ipmc_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_inseg_entry_t* inseg_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_inseg_entry(inseg_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_INSEG_ENTRY, .objectkey = { .key = { .inseg_entry = *inseg_entry } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(inseg_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, inseg_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ const sai_nat_entry_t* nat_entry,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_nat_entry(nat_entry, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NAT_ENTRY, .objectkey = { .key = { .nat_entry = *nat_entry  } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(nat_entry, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_get(meta_key, nat_entry->switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::create(
        _In_ sai_object_type_t object_type,
        _Out_ sai_object_id_t* object_id,
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_oid(object_type, object_id, switch_id, true);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = SAI_NULL_OBJECT_ID } } };

    status = meta_generic_validation_create(meta_key, switch_id, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.create(object_type, object_id, switch_id, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_key.objectkey.key.object_id = *object_id;

        if (meta_key.objecttype == SAI_OBJECT_TYPE_SWITCH)
        {
            /*
             * We are creating switch object, so switch id must be the same as
             * just created object. We could use SAI_NULL_OBJECT_ID in that
             * case and do special switch inside post_create method.
             */

            switch_id = *object_id;
        }

        meta_generic_validation_post_create(meta_key, switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::set(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ const sai_attribute_t *attr,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_oid(object_type, &object_id, SAI_NULL_OBJECT_ID, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.set(object_type, object_id, attr);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", sai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", sai_serialize_status(status).c_str());
    }

    if (status == SAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

sai_status_t Meta::get(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t attr_count,
        _Inout_ sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    sai_status_t status = meta_sai_validate_oid(object_type, &object_id, SAI_NULL_OBJECT_ID, false);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    sai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != SAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = saiInterface.get(object_type, object_id, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        sai_object_id_t switch_id = sai_switch_id_query(object_id);

        if (!g_oids.objectReferenceExists(switch_id))
        {
            SWSS_LOG_ERROR("switch id 0x%" PRIx64 " doesn't exist", switch_id);
        }

        meta_generic_validation_post_get(meta_key, switch_id, attr_count, attr_list);
    }

    return status;
}

sai_status_t Meta::flushFdbEntries(
        _In_ sai_object_id_t switch_id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    if (attr_count > MAX_LIST_COUNT)
    {
        SWSS_LOG_ERROR("create attribute count %u > max list count %u", attr_count, MAX_LIST_COUNT);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_count != 0 && attr_list == NULL)
    {
        SWSS_LOG_ERROR("attribute list is NULL");

        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_object_type_t swot = sai_object_type_query(switch_id);

    if (swot != SAI_OBJECT_TYPE_SWITCH)
    {
        SWSS_LOG_ERROR("object type for switch_id %s is %s",
                sai_serialize_object_id(switch_id).c_str(),
                sai_serialize_object_type(swot).c_str());

        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (!g_oids.objectReferenceExists(switch_id))
    {
        SWSS_LOG_ERROR("switch id %s doesn't exist",
                sai_serialize_object_id(switch_id).c_str());

        return SAI_STATUS_INVALID_PARAMETER;
    }

    // validate attributes
    // - attribute list can be empty
    // - validation is similar to "create" action but there is no
    //   post create step and no references are updated
    // - fdb entries are updated in fdb notification

    std::unordered_map<sai_attr_id_t, const sai_attribute_t*> attrs;

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const sai_attribute_t* attr = &attr_list[idx];

        auto mdp = sai_metadata_get_attr_metadata(SAI_OBJECT_TYPE_FDB_FLUSH, attr->id);

        if (mdp == NULL)
        {
            SWSS_LOG_ERROR("unable to find attribute metadata SAI_OBJECT_TYPE_FDB_FLUSH:%d", attr->id);

            return SAI_STATUS_INVALID_PARAMETER;
        }

        const sai_attribute_value_t& value = attr->value;

        const sai_attr_metadata_t& md = *mdp;

        META_LOG_DEBUG(md, "(fdbflush)");

        if (attrs.find(attr->id) != attrs.end())
        {
            META_LOG_ERROR(md, "attribute id (%u) is defined on attr list multiple times", attr->id);

            return SAI_STATUS_INVALID_PARAMETER;
        }

        attrs[attr->id] = attr;

        if (md.flags != SAI_ATTR_FLAGS_CREATE_ONLY)
        {
            META_LOG_ERROR(md, "attr is expected to be marked as CREATE_ONLY");

            return SAI_STATUS_INVALID_PARAMETER;
        }

        if (md.isconditional || md.validonlylength > 0)
        {
            META_LOG_ERROR(md, "attr should not be conditional or validonly");

            return SAI_STATUS_INVALID_PARAMETER;
        }

        switch (md.attrvaluetype)
        {
            case SAI_ATTR_VALUE_TYPE_UINT16:

                if (md.isvlan && (value.u16 >= 0xFFF || value.u16 == 0))
                {
                    META_LOG_ERROR(md, "is vlan id but has invalid id %u", value.u16);

                    return SAI_STATUS_INVALID_PARAMETER;
                }

                break;

            case SAI_ATTR_VALUE_TYPE_INT32:

                if (md.isenum && !sai_metadata_is_allowed_enum_value(&md, value.s32))
                {
                    META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", value.s32);

                    return SAI_STATUS_INVALID_PARAMETER;
                }

                break;

            case SAI_ATTR_VALUE_TYPE_OBJECT_ID:

                {
                    sai_status_t status = meta_generic_validation_objlist(md, switch_id, 1, &value.oid);

                    if (status != SAI_STATUS_SUCCESS)
                    {
                        return status;
                    }

                    break;
                }

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }
    }

    // there are no mandatory attributes
    // there are no conditional attributes

    auto status = saiInterface.flushFdbEntries(switch_id, attr_count, attr_list);

    if (status == SAI_STATUS_SUCCESS)
    {
        SWSS_LOG_WARN("TODO, remove all matching fdb entries here, currently removed in FDB notification");
    }

    return status;
}

#define PARAMETER_CHECK_IF_NOT_NULL(param) {                                                \
    if ((param) == nullptr) {                                                               \
        SWSS_LOG_ERROR("parameter " # param " is NULL");                                    \
        return SAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_OBJECT_TYPE(param, OT) {                                        \
    sai_object_type_t _ot = sai_object_type_query(param);                                   \
    if (_ot != (OT)) {                                                                      \
        SWSS_LOG_ERROR("parameter " # param " %s object type is %s, but expected %s",       \
                sai_serialize_object_id(param).c_str(),                                     \
                sai_serialize_object_type(_ot).c_str(),                                     \
                sai_serialize_object_type(OT).c_str());                                     \
        return SAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OBJECT_TYPE_VALID(ot) {                                             \
    if (!sai_metadata_is_object_type_valid(ot)) {                                           \
        SWSS_LOG_ERROR("parameter " # ot " object type %d is invalid", (ot));               \
        return SAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_POSITIVE(param) {                                                   \
    if ((param) <= 0) {                                                                     \
        SWSS_LOG_ERROR("parameter " #param " must be positive");                            \
        return SAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_EXISTS(oid, OT) {                                               \
    sai_object_meta_key_t _key = {                                                          \
        .objecttype = (OT), .objectkey = { .key = { .object_id = (oid) } } };               \
    if (!g_saiObjectCollection.objectExists(_key)) {                                        \
        SWSS_LOG_ERROR("object %s don't exists", sai_serialize_object_id(oid).c_str()); } }

sai_status_t Meta::objectTypeGetAvailability(
        _In_ sai_object_id_t switchId,
        _In_ sai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const sai_attribute_t *attrList,
        _Out_ uint64_t *count,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OID_OBJECT_TYPE(switchId, SAI_OBJECT_TYPE_SWITCH);
    PARAMETER_CHECK_OID_EXISTS(switchId, SAI_OBJECT_TYPE_SWITCH);
    PARAMETER_CHECK_OBJECT_TYPE_VALID(objectType);
    PARAMETER_CHECK_POSITIVE(attrCount);
    PARAMETER_CHECK_IF_NOT_NULL(attrList);
    PARAMETER_CHECK_IF_NOT_NULL(count);

    auto info = sai_metadata_get_object_type_info(objectType);

    PARAMETER_CHECK_IF_NOT_NULL(info);

    std::set<sai_attr_id_t> attrs;

    for (uint32_t idx = 0; idx < attrCount; idx++)
    {
        auto id = attrList[idx].id;

        auto mdp = sai_metadata_get_attr_metadata(objectType, id);

        if (mdp == nullptr)
        {
            SWSS_LOG_ERROR("can't find attribute %s:%d",
                    info->objecttypename,
                    attrList[idx].id);

            return SAI_STATUS_INVALID_PARAMETER;
        }

        if (attrs.find(id) != attrs.end())
        {
            SWSS_LOG_ERROR("attr %s already defined on list", mdp->attridname);

            return SAI_STATUS_INVALID_PARAMETER;
        }

        attrs.insert(id);

        if (!mdp->isresourcetype)
        {
            SWSS_LOG_ERROR("attr %s is not resource type", mdp->attridname);

            return SAI_STATUS_INVALID_PARAMETER;
        }

        switch (mdp->attrvaluetype)
        {
            case SAI_ATTR_VALUE_TYPE_INT32:

                if (mdp->isenum && !sai_metadata_is_allowed_enum_value(mdp, attrList[idx].value.s32))
                {
                    SWSS_LOG_ERROR("%s is enum, but value %d not found on allowed values list",
                            mdp->attridname,
                            attrList[idx].value.s32);

                    return SAI_STATUS_INVALID_PARAMETER;
                }

                break;

            default:

                SWSS_LOG_THROW("value type %s not supported yet, FIXME!",
                        sai_serialize_attr_value_type(mdp->attrvaluetype).c_str());
        }
    }

    auto status = saiInterface.objectTypeGetAvailability(switchId, objectType, attrCount, attrList, count);

    // no post validation required

    return status;
}

sai_status_t Meta::queryAattributeEnumValuesCapability(
        _In_ sai_object_id_t switchId,
        _In_ sai_object_type_t objectType,
        _In_ sai_attr_id_t attrId,
        _Inout_ sai_s32_list_t *enumValuesCapability,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OID_OBJECT_TYPE(switchId, SAI_OBJECT_TYPE_SWITCH);
    PARAMETER_CHECK_OID_EXISTS(switchId, objectType);
    PARAMETER_CHECK_OBJECT_TYPE_VALID(objectType);

    auto mdp = sai_metadata_get_attr_metadata(objectType, attrId);

    if (!mdp)
    {
        SWSS_LOG_ERROR("unable to find attribute: %s:%d",
                sai_serialize_object_type(objectType).c_str(),
                attrId);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    if (!mdp->isenum && !mdp->isenumlist)
    {
        SWSS_LOG_ERROR("%s is not enum/enum list", mdp->attridname);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    PARAMETER_CHECK_IF_NOT_NULL(enumValuesCapability);

    if (meta_genetic_validation_list(*mdp, enumValuesCapability->count, enumValuesCapability->list)
            != SAI_STATUS_SUCCESS)
    {
        return SAI_STATUS_INVALID_PARAMETER;
    }

    auto status = saiInterface.queryAattributeEnumValuesCapability(switchId, objectType, attrId, enumValuesCapability);

    if (status == SAI_STATUS_SUCCESS)
    {
        if (enumValuesCapability->list)
        {
            // check if all returned values are members of defined enum
            for (uint32_t idx = 0; idx < enumValuesCapability->count; idx++)
            {
                int val = enumValuesCapability->list[idx];

                if (!sai_metadata_is_allowed_enum_value(mdp, val))
                {
                    SWSS_LOG_ERROR("returned value %d is not allowed on %s", val, mdp->attridname);
                }
            }
        }
    }

    return status;
}

#define META_COUNTERS_COUNT_MSB (0x80000000)

static sai_status_t meta_validate_stats(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const sai_stat_id_t *counter_ids,
        _Out_ uint64_t *counters,
        _In_ sai_stats_mode_t mode)
{
    SWSS_LOG_ENTER();

    /*
     * If last bit of counters count is set to high, and unittests are enabled,
     * then this api can be used to SET counter values by user for debugging purposes.
     */
    number_of_counters &= ~(META_COUNTERS_COUNT_MSB);

    PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_OID_OBJECT_TYPE(object_id, object_type);
    PARAMETER_CHECK_OID_EXISTS(object_id, object_type);
    PARAMETER_CHECK_POSITIVE(number_of_counters);
    PARAMETER_CHECK_IF_NOT_NULL(counter_ids);
    PARAMETER_CHECK_IF_NOT_NULL(counters);

    sai_object_id_t switch_id = sai_switch_id_query(object_id);

    // checks also if object type is OID
    sai_status_t status = meta_sai_validate_oid(object_type, &object_id, switch_id, false);

    CHECK_STATUS_SUCCESS(status);

    auto info = sai_metadata_get_object_type_info(object_type);

    PARAMETER_CHECK_IF_NOT_NULL(info);

    if (info->statenum == nullptr)
    {
        SWSS_LOG_ERROR("%s does not support stats", info->objecttypename);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    // check if all counter ids are in enum range

    for (uint32_t idx = 0; idx < number_of_counters; idx++)
    {
        if (sai_metadata_get_enum_value_name(info->statenum, counter_ids[idx]) == nullptr)
        {
            SWSS_LOG_ERROR("vlaue %d is not in range on %s", counter_ids[idx], info->statenum->name);

            return SAI_STATUS_INVALID_PARAMETER;
        }
    }

    // check mode

    if (sai_metadata_get_enum_value_name(&sai_metadata_enum_sai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode vlaue %d is not in range on %s", mode, sai_metadata_enum_sai_stats_mode_t.name);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t Meta::getStats(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const sai_stat_id_t *counter_ids,
        _Out_ uint64_t *counters,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, SAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = saiInterface.getStats(object_type, object_id, number_of_counters, counter_ids, counters);

    // no post validation required

    return status;
}

sai_status_t Meta::getStatsExt(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const sai_stat_id_t *counter_ids,
        _In_ sai_stats_mode_t mode,
        _Out_ uint64_t *counters,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, mode);

    CHECK_STATUS_SUCCESS(status);

    status = saiInterface.getStatsExt(object_type, object_id, number_of_counters, counter_ids, mode, counters);

    // no post validation required

    return status;
}

sai_status_t Meta::clearStats(
        _In_ sai_object_type_t object_type,
        _In_ sai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const sai_stat_id_t *counter_ids,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    uint64_t counters;
    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, &counters, SAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = saiInterface.clearStats(object_type, object_id, number_of_counters, counter_ids);

    // no post validation required

    return status;
}

sai_status_t Meta::bulkRemove(
        _In_ sai_object_type_t object_type,
        _In_ uint32_t object_count,
        _In_ const sai_object_id_t *object_id,
        _In_ sai_bulk_op_error_mode_t mode,
        _Out_ sai_status_t *object_statuses,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // all objects must be same type and come from the same switch
    // TODO check multiple switches

    PARAMETER_CHECK_IF_NOT_NULL(object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        object_statuses[idx] = SAI_STATUS_NOT_EXECUTED;
    }

    PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_POSITIVE(object_count);
    PARAMETER_CHECK_IF_NOT_NULL(object_id);

    if (sai_metadata_get_enum_value_name(&sai_metadata_enum_sai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode vlaue %d is not in range on %s", mode, sai_metadata_enum_sai_stats_mode_t.name);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    std::vector<sai_object_meta_key_t> vmk;

    // actually we could make copy of current db and actually execute to see if all will succeed

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        sai_status_t status = meta_sai_validate_oid(object_type, &object_id[idx], SAI_NULL_OBJECT_ID, false);

        CHECK_STATUS_SUCCESS(status);

        sai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id[idx] } } };

        vmk.push_back(meta_key);

        status = meta_generic_validation_remove(meta_key);

        CHECK_STATUS_SUCCESS(status);
    }

    auto status = saiInterface.bulkRemove(object_type, object_count, object_id, mode, object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        if (object_statuses[idx] == SAI_STATUS_SUCCESS)
        {
            meta_generic_validation_post_remove(vmk[idx]);
        }
    }

    return status;
}

sai_status_t Meta::bulkRemove(
        _In_ uint32_t object_count,
        _In_ const sai_route_entry_t *route_entry,
        _In_ sai_bulk_op_error_mode_t mode,
        _Out_ sai_status_t *object_statuses,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // all objects must be same type and come from the same switch
    // TODO check multiple switches

    PARAMETER_CHECK_IF_NOT_NULL(object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        object_statuses[idx] = SAI_STATUS_NOT_EXECUTED;
    }

    //PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_POSITIVE(object_count);
    PARAMETER_CHECK_IF_NOT_NULL(route_entry);

    if (sai_metadata_get_enum_value_name(&sai_metadata_enum_sai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode vlaue %d is not in range on %s", mode, sai_metadata_enum_sai_stats_mode_t.name);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    std::vector<sai_object_meta_key_t> vmk;

    // actually we could make copy of current db and actually execute to see if all will succeed

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        sai_status_t status = meta_sai_validate_route_entry(&route_entry[idx], false);

        CHECK_STATUS_SUCCESS(status);

        sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_ROUTE_ENTRY, .objectkey = { .key = { .route_entry = route_entry[idx] } } };

        vmk.push_back(meta_key);

        status = meta_generic_validation_remove(meta_key);

        CHECK_STATUS_SUCCESS(status);
    }

    auto status = saiInterface.bulkRemove(object_count, route_entry, mode, object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        if (object_statuses[idx] == SAI_STATUS_SUCCESS)
        {
            meta_generic_validation_post_remove(vmk[idx]);
        }
    }

    return status;
}

sai_status_t Meta::bulkRemove(
        _In_ uint32_t object_count,
        _In_ const sai_nat_entry_t *nat_entry,
        _In_ sai_bulk_op_error_mode_t mode,
        _Out_ sai_status_t *object_statuses,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // all objects must be same type and come from the same switch
    // TODO check multiple switches

    PARAMETER_CHECK_IF_NOT_NULL(object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        object_statuses[idx] = SAI_STATUS_NOT_EXECUTED;
    }

    //PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_POSITIVE(object_count);
    PARAMETER_CHECK_IF_NOT_NULL(nat_entry);

    if (sai_metadata_get_enum_value_name(&sai_metadata_enum_sai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode vlaue %d is not in range on %s", mode, sai_metadata_enum_sai_stats_mode_t.name);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    std::vector<sai_object_meta_key_t> vmk;

    // actually we could make copy of current db and actually execute to see if all will succeed

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        sai_status_t status = meta_sai_validate_nat_entry(&nat_entry[idx], false);

        CHECK_STATUS_SUCCESS(status);

        sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_NAT_ENTRY, .objectkey = { .key = { .nat_entry = nat_entry[idx] } } };

        vmk.push_back(meta_key);

        status = meta_generic_validation_remove(meta_key);

        CHECK_STATUS_SUCCESS(status);
    }

    auto status = saiInterface.bulkRemove(object_count, nat_entry, mode, object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        if (object_statuses[idx] == SAI_STATUS_SUCCESS)
        {
            meta_generic_validation_post_remove(vmk[idx]);
        }
    }

    return status;
}

sai_status_t Meta::bulkRemove(
        _In_ uint32_t object_count,
        _In_ const sai_fdb_entry_t *fdb_entry,
        _In_ sai_bulk_op_error_mode_t mode,
        _Out_ sai_status_t *object_statuses,
        _Inout_ sairedis::SaiInterface& saiInterface)
{
    SWSS_LOG_ENTER();

    // all objects must be same type and come from the same switch
    // TODO check multiple switches

    PARAMETER_CHECK_IF_NOT_NULL(object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        object_statuses[idx] = SAI_STATUS_NOT_EXECUTED;
    }

    //PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_POSITIVE(object_count);
    PARAMETER_CHECK_IF_NOT_NULL(fdb_entry);

    if (sai_metadata_get_enum_value_name(&sai_metadata_enum_sai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode vlaue %d is not in range on %s", mode, sai_metadata_enum_sai_stats_mode_t.name);

        return SAI_STATUS_INVALID_PARAMETER;
    }

    std::vector<sai_object_meta_key_t> vmk;

    // actually we could make copy of current db and actually execute to see if all will succeed

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        sai_status_t status = meta_sai_validate_fdb_entry(&fdb_entry[idx], false);

        CHECK_STATUS_SUCCESS(status);

        sai_object_meta_key_t meta_key = { .objecttype = SAI_OBJECT_TYPE_FDB_ENTRY, .objectkey = { .key = { .fdb_entry = fdb_entry[idx] } } };

        vmk.push_back(meta_key);

        status = meta_generic_validation_remove(meta_key);

        CHECK_STATUS_SUCCESS(status);
    }

    auto status = saiInterface.bulkRemove(object_count, fdb_entry, mode, object_statuses);

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        if (object_statuses[idx] == SAI_STATUS_SUCCESS)
        {
            meta_generic_validation_post_remove(vmk[idx]);
        }
    }

    return status;
}

//
//
//
//sai_status_t (*sai_bulk_object_create_fn)(
//        _In_ sai_object_id_t switch_id,
//        _In_ uint32_t object_count,
//        _In_ const uint32_t *attr_count,
//        _In_ const sai_attribute_t **attr_list,
//        _In_ sai_bulk_op_error_mode_t mode,
//        _Out_ sai_object_id_t *object_id,
//        _Out_ sai_status_t *object_statuses);
//
//
//sai_status_t (*sai_bulk_create_route_entry_fn)(
//        _In_ uint32_t object_count,
//        _In_ const sai_route_entry_t *route_entry,
//        _In_ const uint32_t *attr_count,
//        _In_ const sai_attribute_t **attr_list,
//        _In_ sai_bulk_op_error_mode_t mode,
//        _Out_ sai_status_t *object_statuses);
//
