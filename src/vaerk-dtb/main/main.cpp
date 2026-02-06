#include <karm/entry>

import Vaerk.Dtb;

using namespace Karm;
using namespace Vaerk;

void dumpTree(Dtb::Node node, Io::Emit& e) {
    e("{#} {{", node.name());
    e.indentNewline();
    for (auto prop : node.iterProp()) {
        auto str = prop.extraStr();
        if (str) {
            e("{#} = {#}\n", prop.name, str);
        } else {
            e("{#} = {:#02x}\n", prop.name, prop.extra);
        }
    }
    for (auto child : node.iterChildren()) {
        dumpTree(child, e);
    }
    e.deindent();
    e("}\n");
}

Async::Task<> entryPointAsync(Sys::Context&, Async::CancellationToken) {
    auto file = co_try$(Sys::File::open("file:cv181x_milkv_duos_sd.dtb"_url));
    auto map = co_try$(Sys::mmap(file));
    auto dtb = co_try$(Dtb::Blob::open(map.bytes()));

    Sys::println("magic: {:x}", dtb.header().magic);
    Sys::println("total size: {}", DataSize{dtb.header().totalSize});
    Sys::println("version: {}", dtb.header().version);
    Sys::println("last compatible version: {}", dtb.header().lastCompatibleVersion);

    Sys::println("memory reservation:");
    for (auto& reserved : dtb.memoryReservations()) {
        Sys::println("  - {:#08x}-{:#08x}", reserved.address, reserved.address + reserved.size);
    }

    Sys::println("tree:");
    Io::Emit e{Sys::out()};
    dumpTree(dtb.root(), e);

    co_return Ok();
}
