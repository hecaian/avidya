/*
 * Copyright(C) lichuang
 */
#include <arpa/inet.h>
#include "eventrpc/event.h"
#include "eventrpc/log.h"
#include "eventrpc/net_utility.h"
#include "eventrpc/utility.h"
#include "eventrpc/message_connection_manager.h"
#include "eventrpc/message_server.h"
namespace eventrpc {
class MessageServerEvent;
struct MessageServer::Impl {
 public:
  Impl();

  ~Impl();

  void Start();

  void Stop();

  void set_host_and_port(const string &host, uint32 port) {
    host_ = host;
    port_ = port;
  }

  void set_dispatcher(Dispatcher *dispatcher) {
    dispatcher_ = dispatcher;
  }

  void set_message_handler_factory(ServerMessageHandlerFactory *factory) {
    manager_.set_message_handler_factory(factory);
  }

  int HandleAccept();

  void Close();

 public:
  string host_;
  uint32 port_;
  int listen_fd_;
  MessageServerEvent *event_;
  Dispatcher *dispatcher_;
  MessageConnectionManager manager_;
};

struct MessageServerEvent : public Event {
 public:
  MessageServerEvent(int fd, uint32 event_flags, MessageServer::Impl *server)
    : Event(fd, event_flags),
    server_(server) {
    }

  virtual ~MessageServerEvent() {
  }

  int HandleRead() {
    return server_->HandleAccept();
  }

  int HandleWrite() {
    return -1;
  }

 public:
  MessageServer::Impl *server_;
};

MessageServer::Impl::Impl()
  : host_(""),
    port_(0),
    listen_fd_(0),
    event_(NULL),
    dispatcher_(NULL) {
}

MessageServer::Impl::~Impl() {
  Close();
}

void MessageServer::Impl::Start() {
  listen_fd_ = NetUtility::Listen(host_.c_str(), port_);
  ASSERT_TRUE(listen_fd_ > 0);
  event_ = new MessageServerEvent(listen_fd_,
                                  EVENT_READ, this);
  dispatcher_->AddEvent(event_);
  ASSERT_NE(static_cast<MessageServerEvent*>(NULL), event_);
}

void MessageServer::Impl::Stop() {
  Close();
}

void MessageServer::Impl::Close() {
  if (event_->fd_ > 0) {
    LOG_INFO() << "shut down listen on " << host_ << " : " << port_;
    dispatcher_->DeleteEvent(event_);
    delete event_;
  }
}

int MessageServer::Impl::HandleAccept() {
  int fd;
  char buffer[30];
  while (true) {
    struct sockaddr_in address;
    fd = NetUtility::Accept(listen_fd_, &address);
    if (fd == -1) {
      break;
    }
    VLOG_INFO() << "accept connection from "
      << inet_ntop(AF_INET, &address.sin_addr, buffer, sizeof(buffer))
      << ":" << ntohs(address.sin_port);
    MessageConnection* connection = manager_.GetConnection();
    connection->set_fd(fd);
    connection->set_client_address(address);
    connection->set_dispacher(dispatcher_);
    dispatcher_->AddEvent(connection->event());
  }
  return 0;
}

MessageServer::MessageServer() {
  impl_ = new Impl();
}

MessageServer::~MessageServer() {
  delete impl_;
}

void MessageServer::Start() {
  impl_->Start();
}

void MessageServer::Stop() {
  impl_->Stop();
}

void MessageServer::set_host_and_port(const string &host, uint32 port) {
  impl_->set_host_and_port(host, port);
}

void MessageServer::set_dispatcher(Dispatcher *dispatcher) {
  impl_->set_dispatcher(dispatcher);
}

void MessageServer::set_message_handler_factory(
    ServerMessageHandlerFactory *factory) {
  impl_->set_message_handler_factory(factory);
}
}; // namespace eventrpc
