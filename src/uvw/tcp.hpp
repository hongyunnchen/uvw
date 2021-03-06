#pragma once


#include <type_traits>
#include <utility>
#include <memory>
#include <string>
#include <chrono>
#include <uv.h>
#include "event.hpp"
#include "request.hpp"
#include "stream.hpp"
#include "util.hpp"


namespace uvw {


namespace details {


enum class UVTcpFlags: std::underlying_type_t<uv_tcp_flags> {
    IPV6ONLY = UV_TCP_IPV6ONLY
};


}


class TcpHandle final: public StreamHandle<TcpHandle, uv_tcp_t> {
    using StreamHandle::StreamHandle;

public:
    using Time = std::chrono::seconds;
    using Bind = details::UVTcpFlags;
    using IPv4 = details::IPv4;
    using IPv6 = details::IPv6;

    template<typename... Args>
    static std::shared_ptr<TcpHandle> create(Args&&... args) {
        return std::shared_ptr<TcpHandle>{new TcpHandle{std::forward<Args>(args)...}};
    }

    bool init() {
        return initialize<uv_tcp_t>(&uv_tcp_init);
    }

    template<typename T, typename... Args>
    bool init(T&& t, Args&&... args) {
        return initialize<uv_tcp_t>(&uv_tcp_init_ex, std::forward<T>(t), std::forward<Args>(args)...);
    }

    void open(OSSocketHandle sock) {
        invoke(&uv_tcp_open, get<uv_tcp_t>(), sock);
    }

    void noDelay(bool value = false) {
        invoke(&uv_tcp_nodelay, get<uv_tcp_t>(), value);
    }

    void keepAlive(bool enable = false, Time time = Time{0}) {
        invoke(&uv_tcp_keepalive, get<uv_tcp_t>(), enable, time.count());
    }

    void simultaneousAccepts(bool enable = true) {
        invoke(&uv_tcp_simultaneous_accepts, get<uv_tcp_t>(), enable);
    }

    template<typename I = IPv4>
    void bind(std::string ip, unsigned int port, Flags<Bind> flags = Flags<Bind>{}) {
        typename details::IpTraits<I>::Type addr;
        details::IpTraits<I>::AddrFunc(ip.data(), port, &addr);
        invoke(&uv_tcp_bind, get<uv_tcp_t>(), reinterpret_cast<const sockaddr *>(&addr), flags);
    }

    template<typename I = IPv4>
    void bind(Addr addr, Flags<Bind> flags = Flags<Bind>{}) {
        bind<I>(addr.ip, addr.port, flags);
    }

    template<typename I = IPv4>
    Addr sock() const noexcept {
        return details::address<I>(&uv_tcp_getsockname, get<uv_tcp_t>());
    }

    template<typename I = IPv4>
    Addr peer() const noexcept {
        return details::address<I>(&uv_tcp_getpeername, get<uv_tcp_t>());
    }

    template<typename I = IPv4>
    void connect(std::string ip, unsigned int port) {
        typename details::IpTraits<I>::Type addr;
        details::IpTraits<I>::AddrFunc(ip.data(), port, &addr);

        auto listener = [ptr = shared_from_this()](const auto &event, details::ConnectReq &) {
            ptr->publish(event);
        };

        auto connect = loop().resource<details::ConnectReq>();
        connect->once<ErrorEvent>(listener);
        connect->once<ConnectEvent>(listener);
        connect->connect(&uv_tcp_connect, get<uv_tcp_t>(), reinterpret_cast<const sockaddr *>(&addr));
    }

    template<typename I = IPv4>
    void connect(Addr addr) {
        connect<I>(addr.ip, addr.port);
    }
};


}
