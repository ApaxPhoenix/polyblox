#include <algorithm>

#include "graphics/library/drivers/queue.hpp"


namespace core::graphics::library::drivers {

    void Queue::push(const Key& key, const Command& command) noexcept {
        this->keys.push_back(key);
        this->commands.push_back(command);
    }

    Buffer Queue::flush() noexcept {
        this->sort();
        this->buffer.keys = this->keys.data();
        this->buffer.commands = this->commands.data();
        this->buffer.count = this->keys.size();
        return this->buffer;
    }

    void Queue::clear() noexcept {
        this->keys.clear();
        this->commands.clear();
    }

    void Queue::sort() noexcept {
        const std::size_t size = this->keys.size();
        if (size == 0) return;

        this->scratch.resize(size);
        this->sorted.resize(size);

        constexpr std::size_t stride = 11;
        constexpr std::size_t buckets = 1 << stride;
        constexpr std::size_t mask = buckets - 1;
        std::size_t histograms[buckets];

        Key* input = this->keys.data();
        Key* output = this->scratch.data();
        Command* source = this->commands.data();
        Command* target = this->sorted.data();

        for (std::size_t shift = 0; shift < 64; shift += stride) {
            std::fill_n(histograms, buckets, 0);

            for (std::size_t i = 0; i < size; ++i) {
                histograms[input[i].value >> shift & mask]++;
            }

            std::size_t offset = 0;
            for (std::size_t & histogram : histograms) {
                const std::size_t count = histogram;
                histogram = offset;
                offset += count;
            }

            for (std::size_t i = 0; i < size; ++i) {
                const std::size_t bucket = input[i].value >> shift & mask;
                const std::size_t destination = histograms[bucket]++;
                output[destination] = input[i];
                target[destination] = source[i];
            }

            std::swap(input, output);
            std::swap(source, target);
        }

        if (input != this->keys.data()) {
            std::copy_n(this->scratch.data(), size, this->keys.data());
            std::copy_n(this->sorted.data(), size, this->commands.data());
        }
    }

}