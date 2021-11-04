#include <iostream>
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <Eigen/Dense>

// https://mariusbancila.ro/blog/2020/06/06/a-custom-cpp20-range-view/
// https://stackoverflow.com/questions/58029724/create-ranges-custom-view-functions-operator-and-operator
// https://hannes.hauswedell.net/post/2018/04/11/view1/

template <typename Rng>
struct matrix_iterator
    : boost::stl_interfaces::proxy_iterator_interface<matrix_iterator<Rng>,
          std::random_access_iterator_tag, Eigen::Map<Eigen::Vector3f>>
{
    constexpr matrix_iterator() noexcept
        : m_iter(nullptr)
    {
    }

    constexpr matrix_iterator(std::ranges::iterator_t<Rng> iter) noexcept
        : m_iter(iter)
    {
    }

    Eigen::Map<Eigen::Vector3f> operator*() const noexcept
    {
        return Eigen::Map<Eigen::Vector3f>(&(*m_iter));
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
    std::ranges::iterator_t<Rng> m_iter;
};

template <typename Rng>
struct matrix_view : public std::ranges::view_interface<matrix_view<Rng>>
{
    matrix_view() = default;
    matrix_view(const matrix_view&) = default;
    matrix_view(matrix_view&&) = default;
    matrix_view& operator=(const matrix_view&) = default;
    matrix_view& operator=(matrix_view&&) = default;

    matrix_view(Rng rng)
        : m_rng{rng}
    {
    }

    auto begin() const
    {
        return matrix_iterator<Rng>{std::begin(m_rng)};
    }
    auto end() const
    {
        return matrix_iterator<Rng>{std::end(m_rng)};
    }

private:
    Rng m_rng;
};

template <class R>
matrix_view(R&& base)->matrix_view<std::ranges::views::all_t<R>>;

struct matrix_fn
{
    template <typename Rng>
    auto operator()(Rng&& rng) const
    {
        return matrix_view{std::forward<Rng>(rng)};
    }

    template <typename Rng>
    friend auto operator|(Rng&& rng, matrix_fn& c)
        -> decltype(c(std::forward<Rng>(rng)))
    {
        return c(std::forward<Rng>(rng));
    }

    template <typename Rng>
    friend auto operator|(Rng&& rng, const matrix_fn& c)
        -> decltype(c(std::forward<Rng>(rng)))
    {
        return c(std::forward<Rng>(rng));
    }

    template <typename Rng>
    friend auto operator|(Rng&& rng, matrix_fn&& c)
        -> decltype(std::move(c)(std::forward<Rng>(rng)))
    {
        return std::move(c)(std::forward<Rng>(rng));
    }
};

namespace views
{
constexpr matrix_fn matrix;
}

int main()
{
    std::vector<float> bla{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, -1.0f, -1.0f,
        -1.0f, 9.0f, 9.0f, 9.0f};
    auto rng = bla | views::matrix | std::ranges::views::filter([](auto i) {
        return i.x() != -1.0f && i.y() != -1.0f && i.z() != -1.0f;
    });

    for (auto i : rng)
    {
        std::cout << Eigen::Transpose(i) << std::endl;
    }
}
