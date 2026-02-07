#include <karm/entry>

import Vaerk.Dtb;
import Karm.Cli;

using namespace Karm;
using namespace Vaerk;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken) {
    auto inputArg = Cli::operand<Str>("dtb"s, "Path to the device tree blob"s);

    Cli::Command cmd{
        "vaerk-dtb"s,
        "Dump a device tree blob"s,
        {
            Cli::Section{"Input"s, {inputArg}},
        }
    };

    co_trya$(cmd.execAsync(ctx));
    if (not cmd)
        co_return Ok();

    if (not inputArg.value())
        co_return Error::invalidInput("no dtb file provided");

    auto url = Ref::parseUrlOrPath(inputArg.value(), co_try$(Sys::pwd()));
    auto file = co_try$(Sys::File::open(url));
    auto map = co_try$(Sys::mmap(file));
    auto dtb = co_try$(Dtb::Blob::open(map.bytes()));

    Io::Emit e{Sys::out()};
    dtb.dump(e);

    co_return Ok();
}
