#pragma once


#include <utility>
#include <string>
#include <memory>
#include <uv.h>
#include "event.hpp"
#include "handle.hpp"
#include "util.hpp"


namespace uvw {


namespace details {


enum class UVFsEventFlags: std::underlying_type_t<uv_fs_event_flags> {
    WATCH_ENTRY = UV_FS_EVENT_WATCH_ENTRY,
    STAT = UV_FS_EVENT_STAT,
    RECURSIVE = UV_FS_EVENT_RECURSIVE
};


enum class UVFsEvent: std::underlying_type_t<uv_fs_event> {
    RENAME = UV_RENAME,
    CHANGE = UV_CHANGE
};


}


struct FsEventEvent: Event<FsEventEvent> {
    FsEventEvent(std::string fPath, Flags<details::UVFsEvent> f)
        : flgs{std::move(f)}, relPath{std::move(fPath)}
    { }

    const char * filename() const noexcept { return relPath.data(); }
    Flags<details::UVFsEvent> flags() const noexcept { return flgs; }

private:
    Flags<details::UVFsEvent> flgs;
    std::string relPath;
};


class FsEventHandle final: public Handle<FsEventHandle, uv_fs_event_t> {
    static void startCallback(uv_fs_event_t *handle, const char *filename, int events, int status) {
        FsEventHandle &fsEvent = *(static_cast<FsEventHandle*>(handle->data));
        if(status) { fsEvent.publish(ErrorEvent{status}); }
        else { fsEvent.publish(FsEventEvent{filename, static_cast<std::underlying_type_t<Event>>(events)}); }
    }

    using Handle::Handle;

public:
    using Watch = details::UVFsEvent;
    using Event = details::UVFsEventFlags;

    template<typename... Args>
    static std::shared_ptr<FsEventHandle> create(Args&&... args) {
        return std::shared_ptr<FsEventHandle>{new FsEventHandle{std::forward<Args>(args)...}};
    }

    bool init() {
        return initialize<uv_fs_event_t>(&uv_fs_event_init);
    }

    void start(std::string path, Flags<Watch> flags = Flags<Watch>{}) {
        invoke(&uv_fs_event_start, get<uv_fs_event_t>(), &startCallback, path.data(), flags);
    }

    void stop() {
        invoke(&uv_fs_event_stop, get<uv_fs_event_t>());
    }

    std::string path() noexcept {
        return details::path(&uv_fs_event_getpath, get<uv_fs_event_t>());
    }
};


}
