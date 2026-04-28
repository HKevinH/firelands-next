#ifndef FIRELANDS_INFRASTRUCTURE_NETWORK_REST_REST_AUTH_SERVER_H
#define FIRELANDS_INFRASTRUCTURE_NETWORK_REST_REST_AUTH_SERVER_H

#include <application/services/AuthService.h>
#include <application/services/WebSessionService.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <string>
#include <thread>

namespace Firelands {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class RestAuthServer {
public:
  RestAuthServer(std::shared_ptr<AuthService> authService,
                 std::shared_ptr<WebSessionService> webSessionService,
                 const std::string &address, uint16_t port);

  ~RestAuthServer();

  bool Start();
  void Stop();

private:
  void DoAccept();
  void HandleRequest(http::request<http::string_body> &&req,
                     tcp::socket &socket);

  std::shared_ptr<AuthService> _authService;
  std::shared_ptr<WebSessionService> _webSessionService;

  std::string _address;
  uint16_t _port;

  net::io_context _ioc;
  std::unique_ptr<tcp::acceptor> _acceptor;
  std::thread _workerThread;
  bool _running;
};

} // namespace Firelands

#endif // FIRELANDS_INFRASTRUCTURE_NETWORK_REST_REST_AUTH_SERVER_H
