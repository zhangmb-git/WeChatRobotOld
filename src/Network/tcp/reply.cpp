#include "reply.hpp"
std::vector<asio::const_buffer> reply::to_buffers() {
    std::vector<asio::const_buffer>  buffers;
    std::string test = "hello";
    buffers.push_back(asio::const_buffer(test.data(), test.length()));
    return  buffers;
}