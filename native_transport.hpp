#pragma once

#include <bonefish/native/native_message_queue.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/io_service.hpp>
#include <memory>
#include <msgpack.hpp>

namespace autobahn {
class wamp_transport;
class wamp_transport_handler;
} // namespace autobahn

namespace bonefish {
class native_connector;
class native_endpoint;
} // namespace bonefish

class native_transport :
        public autobahn::wamp_transport,
        public std::enable_shared_from_this<native_transport>
{
public:
    native_transport(
            boost::asio::io_service& io_service,
            const std::shared_ptr<bonefish::native_connector>& connector);

    virtual ~native_transport() override = default;

    virtual boost::future<void> connect() override;
    virtual boost::future<void> disconnect() override;
    virtual bool is_connected() const override;

    virtual void send(autobahn::wamp_message&& message) override;
    virtual void set_pause_handler(autobahn::wamp_transport::pause_handler&& handler) override;
    virtual void set_resume_handler(autobahn::wamp_transport::resume_handler&& handler) override;
    virtual void pause() override;
    virtual void resume() override;
    virtual void attach(
            const std::shared_ptr<autobahn::wamp_transport_handler>& handler) override;
    virtual void detach() override;
    virtual bool has_handler() const override;

private:
    boost::asio::io_service& m_io_service;

    std::shared_ptr<autobahn::wamp_transport_handler> m_transport_handler;
    bonefish::native_message_queue m_receive_message_queue;

    std::shared_ptr<bonefish::native_connector> m_connector;
    std::shared_ptr<bonefish::native_endpoint> m_server_endpoint;
    std::shared_ptr<bonefish::native_endpoint> m_component_endpoint;
};

#include "native_transport.ipp"
