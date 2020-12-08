#include "pch.h"
#include "TcpClient.h"
#include "log/ZLogger.h"
#include <atomic>
#include <thread>

std::atomic_uint16_t g_seq = 1;
const uint32_t kMaxBufferLen = 1024 * 10;// 10 KB
asio::io_context g_io_context;

void init_tcp_client() {
    std::thread t([]() {
        g_io_context.run();
    });
}

TcpClient::TcpClient(/*asio::io_context& io_context*/) : socket_io_context_(g_io_context), socket_(g_io_context) {
    buffer_ = new uint8_t(kMaxBufferLen);
    ::memset(buffer_, 0, kMaxBufferLen);
}

bool TcpClient::connect(std::string ip, uint16_t port) {
    try {
        tcp::resolver resolver(socket_io_context_);
        tcp::resolver::results_type endpoints =
            resolver.resolve(ip + ":" + std::to_string(port), "daytime");

        auto r = asio::connect(socket_, endpoints);
        is_connect_ = true;
        return true;

    } catch (std::exception& e) {
        LogWarn(e.what());
        return false;
    }
}

void TcpClient::close() {

}

bool TcpClient::sendReq(CommondType command_id, uint8_t* data, uint32_t len, const ResponseCb& cb) {
    if (g_seq >= UINT16_MAX) {
        g_seq = 1;
    }

    ++g_seq;

    IMHeader header;
    header.command_id = command_id;
    header.seq_ = g_seq;
    header.len = len;
    header.reserved = 0;

    ::memcpy(buffer_, &header, kHeaderLen);
    uint8_t* next_buff = buffer_ + kHeaderLen;
    ::memcpy(next_buff, data, len);

    Request r{ header, std::move(cb) };

    if (map_request_.find(header.seq_) != map_request_.end()) {
        map_request_.insert(make_pair(header.seq_, std::move(r)));

    } else {
        LogWarn("error,duplicate key:{}", header.seq_);
        return false;
    }

    // write
    asio::write(socket_, asio::buffer(buffer_, len + kHeaderLen));

    /*asio::streambuf b;
    std::ostream os(&b);
    os << header;*/
    return true;
}

bool TcpClient::sendNotify(CommondType command_id, uint8_t* data, uint32_t len) {
    return false;
}
