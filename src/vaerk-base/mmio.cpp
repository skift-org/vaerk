export module Vaerk.Base:mmio;

namespace Vaerk {

export template <typename T>
T mmioRead(void* addr) {
    return *static_cast<T volatile*>(addr);
}

export template <typename T>
void mmioWrite(void* addr, T value) {
    *static_cast<T volatile*>(addr) = value;
}

} // namespace Vaerk