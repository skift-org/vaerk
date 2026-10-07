module;

#include <karm/macros>

export module Vaerk.Efi:base;

import Karm.Core;
import Karm.Ref;
import :spec;

using namespace Karm;

namespace Efi {

Handle _handle = nullptr;
SystemTable* _st = nullptr;
Efi::LoadedImageProtocol* _li = nullptr;

export Handle imageHandle() { return _handle; }

export SystemTable* st() { return _st; }

export BootService* bs() {
    return st()->boot;
}

export RuntimeService* rt() {
    return st()->runtime;
}

export void init(Handle handle, SystemTable* st) {
    _handle = handle;
    _st = st;
}

export template <typename P>
inline Res<P*> openProtocol(Handle handle) {
    P* result = nullptr;
    Ref::Guid guid = P::GUID;
    try$(bs()->openProtocol(handle, &guid, (void**)&result, imageHandle(), nullptr, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL));
    return Ok(result);
}

export template <typename P>
inline Res<P*> openProtocol() {
    return openProtocol<P>(imageHandle());
}

export template <typename P>
inline Res<P*> locateProtocol() {
    P* result = nullptr;
    Ref::Guid guid = P::GUID;
    try$(bs()->locateProtocol(&guid, nullptr, (void**)&result));
    return Ok(result);
}

export Efi::LoadedImageProtocol* li() {
    if (not _li) {
        _li = Efi::openProtocol<Efi::LoadedImageProtocol>().expect();
    }

    return _li;
}

} // namespace Efi
