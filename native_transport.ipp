#include <autobahn/autobahn.hpp>
#include <bonefish/native/native_endpoint.hpp>
#include <bonefish/native/native_connector.hpp>
#include <stdexcept>

inline native_transport::native_transport(
            boost::asio::io_service& io_service,
            const std::shared_ptr<bonefish::native_connector>& connector)
    : wamp_transport()
    , m_io_service(io_service)
    , m_transport_handler()
    , m_connector(connector)
    , m_server_endpoint()
    , m_component_endpoint()
{
}

inline boost::future<void> native_transport::connect()
{
    if (m_component_endpoint) {
        boost::promise<void> result;
        result.set_exception(
                std::logic_error("native transport already connected"));
        return result.get_future();
    }

    m_component_endpoint = std::make_shared<bonefish::native_endpoint>();

    std::weak_ptr<native_transport> weak_this = this->shared_from_this();
    m_component_endpoint->set_send_message_handler(
        [this, weak_this](
                std::vector<msgpack::object>&& fields,
                msgpack::zone&& zone) {
            auto shared_this = weak_this.lock();
            if (!shared_this) {
                // This will get thrown in the context of the server.
                throw std::runtime_error("connection closed");
            }

            m_receive_message_queue.push_message(
                    std::move(fields), std::move(zone));

            m_io_service.post([this, weak_this]() {
                auto shared_this = weak_this.lock();
                if (!shared_this) {
                    // FIXME: This will cause the io service to bail!!
                    throw std::runtime_error("connection closed");
                }

                msgpack::zone zone;
                autobahn::wamp_message::message_fields fields;
                m_receive_message_queue.pop_message(fields, zone);

                autobahn::wamp_message message(std::move(fields), std::move(zone));
                m_transport_handler->on_message(std::move(message));
            });
        }
    );

    return m_connector->connect(m_component_endpoint).then([this, weak_this] (
            bonefish::native_endpoint_future connected) {
        auto shared_this = weak_this.lock();
        if (!shared_this) {
            // Nothing to do here as the native transport no longer exists.
            return;
        }
        m_server_endpoint = connected.get();
    });
}

inline boost::future<void> native_transport::disconnect()
{
    if (!m_component_endpoint) {
        boost::promise<void> result;
        result.set_exception(
                std::logic_error("native transport already disconnected"));
        return result.get_future();
    }

    return m_connector->disconnect(m_server_endpoint);
}

inline bool native_transport::is_connected() const
{
    return m_component_endpoint != nullptr;
}

inline void native_transport::send(autobahn::wamp_message&& message)
{
    auto send_message = m_server_endpoint->get_send_message_handler();
    send_message(std::move(message.fields()), std::move(message.zone()));
}

inline void native_transport::set_pause_handler(autobahn::wamp_transport::pause_handler&& handler)
{
    throw std::runtime_error("transport pause handler support not implemneted");
}

inline void native_transport::set_resume_handler(autobahn::wamp_transport::resume_handler&& handler)
{
    throw std::runtime_error("transport resume handler support not implemneted");
}

inline void native_transport::pause()
{
    throw std::runtime_error("transport pause support not implemneted");
}

inline void native_transport::resume()
{
    throw std::runtime_error("transport resume support not implemneted");
}

inline void native_transport::attach(
        const std::shared_ptr<autobahn::wamp_transport_handler>& handler)
{
    if (m_transport_handler) {
        throw std::logic_error("handler already attached");
    }

    m_transport_handler = handler;

    m_transport_handler->on_attach(this->shared_from_this());
}

inline void native_transport::detach()
{
    if (!m_transport_handler) {
        throw std::logic_error("no handler attached");
    }

    m_transport_handler->on_detach(true, "wamp.error.goodbye");
    m_transport_handler.reset();
}

inline bool native_transport::has_handler() const
{
    return m_transport_handler != nullptr;
}
