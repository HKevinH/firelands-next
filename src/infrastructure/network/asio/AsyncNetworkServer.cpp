#include <infrastructure/network/asio/AsyncNetworkServer.h>
#include <infrastructure/network/sessions/AuthSession.h>
#include <shared/Logger.h>

namespace Firelands {

    AsyncNetworkServer::AsyncNetworkServer(std::shared_ptr<AuthService> authService, std::shared_ptr<RealmListService> realmService)
        : _authService(std::move(authService)), _realmService(std::move(realmService)) {
    }

    AsyncNetworkServer::~AsyncNetworkServer() {
        Stop();
    }

    bool AsyncNetworkServer::Start(const std::string& address, uint16 port) {
        try {
            tcp::endpoint endpoint(boost::asio::ip::make_address(address), port);
            _acceptor = std::make_unique<tcp::acceptor>(_ioContext, endpoint);
            
            LOG_INFO("Network Server started on {}:{}", address, port);

            DoAccept();
            return true;
        } catch (std::exception& e) {
            LOG_ERROR("Failed to start network server: {}", e.what());
            return false;
        }
    }

    void AsyncNetworkServer::Stop() {
        _ioContext.stop();
        if (_acceptor) {
            _acceptor->close();
        }
    }

    void AsyncNetworkServer::Update() {
        _ioContext.poll();
    }

    void AsyncNetworkServer::DoAccept() {
        _acceptor->async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    LOG_INFO("New connection from {}", socket.remote_endpoint().address().to_string());
                    std::make_shared<AuthSession>(std::move(socket), _authService, _realmService)->Start();
                }

                DoAccept();
            });
    }

} // namespace Firelands
