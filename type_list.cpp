#include <array>
#include <iostream>
#include <vector>
#include <tuple>
#include <type_traits>

template <typename... T>
struct mp_list {};

template <typename A, template <typename...> typename B>
struct mp_rename_impl;

template <template<typename...> typename A, typename... T,
          template<typename...> typename B>
struct mp_rename_impl<A<T...>,B>
{
    using type = B<T...>;
};

template <typename A, template <typename...> typename B>
using mp_rename = typename mp_rename_impl<A,B>::type;

template <typename L>
struct mp_size_impl;

template <template<typename...> typename L, typename... T>
struct mp_size_impl<L<T...>>
{
    using type = std::integral_constant<std::size_t,sizeof...(T)>;
};

template <typename L>
using mp_size = typename mp_size_impl<L>::type;

template <typename... T>
using mp_length = std::integral_constant<std::size_t,sizeof...(T)>;

template <typename L>
using mp_size_2 = typename mp_rename<L,mp_length>::type;

template <template<typename...> typename F, typename L>
using mp_apply = typename mp_rename<L,F>::type;

template <typename L, typename... T>
struct mp_push_front_impl;

template <template <typename ...> typename L, typename... U, typename... T>
struct mp_push_front_impl<L<U...>,T...>
{
    using type = L<T...,U...>;
};

template <typename L, typename... T>
using mp_push_front = typename mp_push_front_impl<L, T...>::type;

template <typename L>
struct mp_pop_front_impl;

template <template <typename...> typename L, typename T1, typename... T>
struct mp_pop_front_impl<L<T1,T...>>
{
    using type = L<T...>;
};

template <typename L>
using mp_pop_front = typename mp_pop_front_impl<L>::type;

template <typename L>
struct mp_front_impl;

template <template <typename...> typename L, typename T1, typename...T>
struct mp_front_impl<L<T1,T...>>
{
    using type = T1;
};

template <typename L>
using  mp_front = typename mp_front_impl<L>::type;

template <typename... L>
struct mp_empty_impl 
{
    using type = std::false_type;
};

template <template <typename...> typename... L>
struct mp_empty_impl<L<>...> 
{
    using type = std::true_type;
};


template <typename... L>
using mp_empty = typename mp_empty_impl<L...>::type;


template <template <typename...> typename F, typename L>
struct mp_transform_impl;

template <template <typename...> typename F, template <typename...> typename L, typename... T>
struct mp_transform_impl<F,L<T...>>
{
    using type = L<F<T>...>;
};

template <template <typename...> typename F, typename L>
using mp_transform = typename mp_transform_impl<F,L>::type;

template <typename T>
using unpack = typename T::type;

template <typename T>
using add_pointer = T*;

int main(int argc, char* argv[])
{

    static_assert(
        std::is_same<mp_empty<std::tuple<int, double>, std::pair<int,double>>::type,
            std::false_type>::value == true,
        "Not equal");

    static_assert(
        std::is_same<mp_empty<std::tuple<>, std::tuple<>>::type,
            std::true_type>::value == true,
        "Not equal");


    static_assert(
        std::is_same<mp_front<std::tuple<int, double>>, int>::value == true,
        "Not equal");

    static_assert(std::is_same<mp_pop_front<std::tuple<char>>,
                      std::tuple<>>::value == true,
        "Not equal");

    using packed =  mp_transform<std::add_pointer,std::tuple<int,double>>;
    static_assert(std::is_same<mp_transform<unpack, packed>,
                      std::tuple<int*, double*>>::value == true,
        "Not equal");

    static_assert(
        std::is_same<mp_transform<add_pointer, std::tuple<int, double>>,
            std::tuple<int*, double*>>::value == true,
        "Not equal");

    static_assert(std::is_same<mp_transform<add_pointer,std::tuple<int,double>>,
                  std::tuple<int*,double*>>::value == true, "Not equal");
    
    
    static_assert(
        std::is_same<mp_push_front<std::tuple<int, double>, double, char>,
        std::tuple<double,char,int, double>>::value == true,
        "Not equal");

    
    static_assert(mp_apply<mp_length,std::pair<double,double>>::type::value == 2, "Not equal");

    static_assert(mp_length<int, double, char>::type::value == 3, "Not equal");

    static_assert(mp_size<std::tuple<double, char, int>>::type::value == 3, "Not equal");

    static_assert(mp_size_2<std::tuple<double, char, int>>::type::value == 3, "Not equal");

    static_assert(
        std::is_same<mp_rename<std::tuple<int, double>, std::pair>,
        std::pair<int, double>>::value == true, "Not equal");

    return 0;
}

