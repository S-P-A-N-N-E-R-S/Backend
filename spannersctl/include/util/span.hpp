#pragma once

#include <cstddef>  // for size_t
#include <string>
#include <vector>

namespace cli {

// Implementation from Timo, slightly adapted. See
// https://github.com/tglane/cpp_utility/blob/d4c3bb34568b8f8f66c4cc3cbe17fee1ce750600/span.hpp.
template <typename T>
class span
{
public:
    span() = delete;
    span(const span &) noexcept = default;
    span &operator=(const span &) noexcept = default;
    span(span &&) noexcept = default;
    span &operator=(span &&) noexcept = default;
    ~span() noexcept = default;

    span(T *start, size_t length) noexcept
        : m_start{start}
        , m_size{length}
    {
    }

    span(T *start, T *end) noexcept
        : m_start{start}
        , m_size{static_cast<size_t>(std::distance(start, end) + 1)}
    {
    }

    template <size_t S>
    span(T (&buffer)[S]) noexcept
        : m_start{buffer}
        , m_size{S}
    {
    }

    template <typename ITER>
    span(ITER start, ITER end) noexcept
        : m_start{&(*start)}
        , m_size{static_cast<size_t>(std::distance(&(*start), &(*end)))}
    {
    }

    span(std::vector<T> &vec)
        : m_start{vec.data()}
        , m_size{vec.size()}
    {
    }

    constexpr T *get() const
    {
        return m_start;
    }
    constexpr T *data() const
    {
        return m_start;
    }

    constexpr size_t size() const
    {
        return m_size;
    }

    constexpr bool empty() const
    {
        return m_size == 0;
    }

    constexpr span<T> first(size_t count) const
    {
        return span<T>{m_start, count};
    }

    constexpr span<T> last(size_t count) const
    {
        return span<T>{m_start + (m_size - count), count};
    }

    /// Short for ::last(size() - 1)
    constexpr span<T> tail() const
    {
        return this->last(m_size - 1);
    }

    constexpr T &operator[](size_t index)
    {
        return m_start[index];
    }
    constexpr const T &operator[](size_t index) const
    {
        return m_start[index];
    }

    constexpr T *begin() const
    {
        return m_start;
    }
    constexpr T *end() const
    {
        return &(m_start[m_size]);
    }

    constexpr T &front() const
    {
        return m_start[0];
    }
    constexpr T &back() const
    {
        return m_start[m_size - 1];
    }

private:
    T *m_start;
    size_t m_size;
};

/// Template deduction guides for class span
template <typename ITERATOR>
span(ITERATOR, ITERATOR) -> span<typename std::iterator_traits<ITERATOR>::value_type>;

template <typename CONTAINER_TYPE>
span(const CONTAINER_TYPE &)
    -> span<typename std::remove_reference<decltype(std::declval<CONTAINER_TYPE>().front())>::type>;
}
