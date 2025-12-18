// GrlRPC Framework Header
// This file will be implemented in task 6.x and 9.x

#ifndef GRLRPC_RPC_FRAMEWORK_H
#define GRLRPC_RPC_FRAMEWORK_H

#include <string>
#include <cstdint>
#include <functional>
#include <memory>

namespace grlrpc {

// Forward declarations - will be implemented in task 6.x and 9.x

enum class RpcStatus {
    SUCCESS = 0,
    METHOD_NOT_FOUND = 1,
    SERIALIZATION_ERROR = 2,
    NETWORK_ERROR = 3,
    TIMEOUT = 4,
    INVALID_REQUEST = 5,
    UNKNOWN_ERROR = 6
};

struct RpcRequestData;
struct RpcResponseData;
class MessageCodec;
class GrlRpcServer;
class GrlRpcClient;

} // namespace grlrpc

#endif // GRLRPC_RPC_FRAMEWORK_H
