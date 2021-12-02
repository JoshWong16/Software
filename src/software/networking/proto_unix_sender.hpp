#pragma once

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <string>

#include "software/logger/logger.h"

template <class SendProto>
class ProtounixSender
{
   public:
    /**
     * Creates a ProtounixSender that sends the SendProto over the network to the
     * given address and port.
     *
     * The ProtounixSender will be assigned a random available port on creation
     * so it doesn't conflict with anything else on the system.
     *
     * @param io_service The io_service to use to service outgoing SendProto data
     * @param ip_address The ip address to send data on
     * (IPv4 in dotted decimal or IPv6 in hex string)
     *  example IPv4: 192.168.0.2
     *  example IPv6: ff02::c3d0:42d2:bb8%wlp4s0 (the interface is specified after %)
     * @param port The port to send SendProto data to
     * @param multicast If true, joins the multicast group of given ip_address
     */
    ProtounixSender(boost::asio::io_service& io_service, const std::string& ip_address,
                   unsigned short port, bool multicast);

    virtual ~ProtounixSender();

    /**
     * Sends a protobuf message to the initialized ip address and port
     * This function returns after the message has been sent.
     *
     * @param message The protobuf message to send
     */
    void sendProto(const SendProto& message);

   private:
    // A unix socket to send data over
    boost::asio::local::datagram_protocol::socket socket_;

    // The endpoint for the receiver
    boost::asio::local::datagram_protocol::endpoint receiver_endpoint;

    // Buffer to hold serialized protobuf data
    std::string data_buffer;
};

template <class SendProto>
ProtounixSender<SendProto>::ProtounixSender(boost::asio::io_service& io_service,
                                          const std::string& ip_address,
                                          const unsigned short port, bool multicast)
    : socket_(io_service)
{
    boost::asio::local::datagram_protocol::endpoint addr(ip_address);
    receiver_endpoint = addr;
    socket_.open();
}

template <class SendProto>
void ProtounixSender<SendProto>::sendProto(const SendProto& message)
{
    message.SerializeToString(&data_buffer);
    socket_.send_to(boost::asio::buffer(data_buffer), receiver_endpoint);
}

template <class SendProto>
ProtounixSender<SendProto>::~ProtounixSender()
{
    socket_.close();
}

#include "software/networking/proto_unix_sender.hpp"
