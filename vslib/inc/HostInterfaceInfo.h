#pragma once

extern "C" {
#include "sai.h"
}

#include "swss/selectableevent.h"

#include <memory>
#include <thread>
#include <string.h>

namespace saivs
{
    class HostInterfaceInfo
    {
        private:

            HostInterfaceInfo(const HostInterfaceInfo&) = delete;

        public:

            HostInterfaceInfo(
                    _In_ int ifindex,
                    _In_ int socket,
                    _In_ int tapfd,
                    _In_ const std::string& tapname,
                    _In_ sai_object_id_t portId);

            virtual ~HostInterfaceInfo();

        public:

            void process_packet_for_fdb_event(
                    _In_ const uint8_t *buffer,
                    _In_ size_t size) const;

        private:

            void veth2tap_fun();

            void tap2veth_fun();

        public: // TODO to private

            int m_ifindex;

            int m_packet_socket;

            int m_tapfd;

            std::string m_name;

            sai_object_id_t m_portId;

            sai_object_id_t m_hostif_vid;

            bool m_run_thread;

        private:

            std::shared_ptr<std::thread> m_e2t;
            std::shared_ptr<std::thread> m_t2e;

            swss::SelectableEvent m_e2tEvent;
            swss::SelectableEvent m_t2eEvent;
    };
}
