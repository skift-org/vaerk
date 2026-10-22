#include <karm/entry>

import Vaerk.Acpi;
import Karm.Cli;

using namespace Karm;
using namespace Karm::Literals;

using namespace Vaerk;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto inputArg = Cli::operand<Str>("acpi-table"s, "Path to the acpi table"s);

    Cli::Command cmd{
        "acpi-dump"s,
        "Dump a acpi table"s,
        {
            Cli::Section{"Input"s, {inputArg}},
        }
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    if (not inputArg.value())
        co_return Error::invalidInput("no acpi table provided");

    auto url = Ref::parseUrlOrPath(inputArg.value(), env.cwd());
    auto file = co_try$(Sys::File::open(url));
    auto map = co_try$(Sys::mmap(file));
    auto dtb = co_try$(Dtb::Blob::open(map.bytes()));

    Io::Emit e{Sys::out()};
    dtb.dump(e);

    co_return Ok();
}
