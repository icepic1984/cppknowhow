#include <iostream>

template <typename T>
std::string type_name()
{
    typedef typename std::remove_reference<T>::type TR;
    std::string r = typeid(TR).name();
    if (std::is_const<TR>::value)
        r += " const";
    if (std::is_volatile<TR>::value)
        r += " volatile";
    if (std::is_lvalue_reference<T>::value)
        r += "&";
    else if (std::is_rvalue_reference<T>::value)
        r += "&&";
    return r;
}

template <typename T>
void test(T&& t)
{
    std::cout << "T: " << type_name<T>() << std::endl;
    std::cout << "t: " << type_name<decltype(t)>() << std::endl;
}

template <typename T>
void test2(T t)
{
    std::cout << "T: " << type_name<T>() << std::endl;
    std::cout << "t: " << type_name<decltype(t)>() << std::endl;
}

int fac()
{
    return 10;
}

int main()
{
    // Type deduction.
    //
    // 3 cases see Meyer Item 1.
    // template <typename T>
    // void(ParamType param)
    // Case 1: ParamType is a reference pointer
    // Case 2: ParamType is a forwarding reference
    // Case 3: ParamType is Neither a Pointer nor a Reference
    //
    // Reference collapsing
    // See Item 28
    // For forwading references:
    // Widget& && -> Widget&
    // Widget -> Widget&&

    // If either reference is an lvalue reference, the result is an
    // lvalue reference. Otherwise (both are rvalue references) the
    // result is an rvalue reference.

    int i = 0;
    test(i);
    int& i2 = i;
    test(i2);
    const int& i3 = i;
    test(i3);
    const int i4 = 10;
    test(i4);
    test(10.0);

    int j = 0;
    test2(std::move(j));
    int& j2 = j;
    test2(j2);
    const int& j3 = j;
    test2(j3);
    const int j4 = 10;
    test2(j4);
    test2(10.0);

    // https://stackoverflow.com/questions/3716277/do-rvalue-references-allow-dangling-references
    auto&& w = fac();
    std::cout << type_name<decltype(w)>() << std::endl;
    std::cout << w << std::endl;
}
