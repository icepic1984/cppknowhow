#include <iostream>
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <Eigen/Dense>

template <typename Iterator, typename Sentinel>
struct subrange
    : public std::ranges::view_interface<subrange<Iterator, Sentinel>>
{
    subrange() = default;
    constexpr subrange(Iterator it, Sentinel s)
        : first_(it)
        , last_(s)
    {
    }

    constexpr auto begin() const
    {
        return first_;
    }
    constexpr auto end() const
    {
        return last_;
    }

private:
    Iterator first_;
    Sentinel last_;
};

// std::view::all() returns one of several types, depending on what you pass
// it.  Here, we're keeping it simple; all() always returns a subrange.
template <typename Range>
auto all(Range&& range)
{
    return subrange<decltype(range.begin()), decltype(range.end())>(
        range.begin(), range.end());
}

// A template alias that denotes the type of all(r) for some Range r.
template <typename Range>
using all_view = decltype(all(std::declval<Range>()));

struct matrix_iterator
    : boost::stl_interfaces::proxy_iterator_interface<matrix_iterator,
          std::random_access_iterator_tag, Eigen::Map<Eigen::Vector3f>>
{
    constexpr matrix_iterator() noexcept
        : m_iter(nullptr)
    {
    }

    constexpr matrix_iterator(float* iter) noexcept
        : m_iter(iter)
    {
    }

    Eigen::Map<Eigen::Vector3f> operator*() const noexcept
    {
        return Eigen::Map<Eigen::Vector3f>{m_iter};
    }
    constexpr matrix_iterator& operator+=(std::ptrdiff_t i) noexcept
    {
        m_iter += i * 3;
        return *this;
    }
    constexpr auto operator-(matrix_iterator other) const noexcept
    {
        return (m_iter - other.m_iter) / 3;
    }

private:
    float* m_iter;
};

struct matrix_view : public std::ranges::view_interface<matrix_view>
{
    matrix_view() = default;
    matrix_view(std::vector<float>& vec)
        : m_begin{matrix_iterator{vec.data()}}
        , m_end{matrix_iterator{vec.data() + vec.size()}}

    {
    }

    constexpr auto begin() const
    {
        return m_begin;
    }
    constexpr auto end() const
    {
        return m_end;
    }

private:
    matrix_iterator m_begin{};
    matrix_iterator m_end{};
};

// struct matrix : public std::ranges::view_interface<matrix>
// {
//     struct iterator
//     matrix() = default;

// };

int main()
{
    std::vector<float> bla{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    matrix_iterator start{bla.data()};
    matrix_iterator end{bla.data() + 6};

    matrix_view view{bla};

    for (auto i : view)
    {
        std::cout << i.x() << " " << i.y() << " " << i.z() << std::endl;
    }

    std::for_each(start, end, [](auto i) {
        std::cout << i.x() << " " << i.y() << " " << i.z() << std::endl;
    });

    Eigen::Map<Eigen::Vector3f> bup{bla.data()};

    // for (auto i : all(bla))
    // {
    //     std::cout << i << std::endl;
    // }
}
