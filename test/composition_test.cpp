// Copyright 2015, Tobias Hermann and the FunctionalPlus contributors.
// https://github.com/Dobiasd/FunctionalPlus
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <fplus/fplus.hpp>
#include <vector>

namespace {

    int APlusTwoTimesBFunc(int a, int b)
    {
        return a + 2 * b;
    }

    typedef std::deque<int> IntDeq;
    typedef std::deque<IntDeq> IntContCont;
    typedef IntDeq IntCont;
    typedef IntCont Row;

    std::uint64_t fibo(std::uint64_t n)
    {
        if (n < 2)
            return n;
        else
            return fibo(n-1) + fibo(n-2);
    }
    // version using continuation passing style (CPS)
    std::uint64_t fibo_cont(const std::function<std::uint64_t(std::uint64_t)>& cont, std::uint64_t n)
    {
        if (n < 2)
            return n;
        else
            return cont(n-1) + cont(n-2);
    }
}

class CompositionTestState {
public:
    explicit CompositionTestState(int x) : x_(x) {}
    void Add(int y) { x_ += y; }
    int Get() const { return x_; }
private:
    int x_;
};

TEST_CASE("composition_test, forward_apply")
{
    using namespace fplus;
    REQUIRE_EQ(forward_apply(3, square<int>), 9);
}

TEST_CASE("composition_test, lazy")
{
    using namespace fplus;
    const auto square_3_stub = lazy(square<int>, 3);
    REQUIRE_EQ(square_3_stub(), 9);
}

TEST_CASE("composition_test, fixed")
{
    using namespace fplus;
    const auto lazy_3 = fixed(3);
    REQUIRE_EQ(lazy_3(), 3);
}

TEST_CASE("composition_test, parameter_binding")
{
    using namespace fplus;
    Row row = {1,2,3};

    typedef IntContCont Mat;
    Mat mat;
    auto square = [](int x){ return x*x; };
    REQUIRE_EQ(bind_unary(square, 2)(), 4);
    auto squareRowElems = bind_1st_of_2(transform<decltype(square), Row>,
            square);
    Row squaredRow = squareRowElems(row);
    REQUIRE_EQ(squaredRow, IntCont({1,4,9}));

    auto int_division = [](int x, int y) { return x / y; };
    REQUIRE_EQ(bind_2nd_of_2(int_division, 2)(6), 3);

    auto add3 = [](int x, int y, int z) { return x + y + z; };
    REQUIRE_EQ(bind_1st_and_2nd_of_3(add3, 3, 5)(7), 15);
}

TEST_CASE("composition_test, compose")
{
    using namespace fplus;
    auto square = [](int x){ return x*x; };
    REQUIRE_EQ((compose(square, square)(2)), 16);
    REQUIRE_EQ((compose(square, square, square)(2)), 256);
    REQUIRE_EQ((compose(square, square, square, square)(2)), 65536);
    REQUIRE_EQ((compose(square, square, square, square, square)(1)), 1);
}

TEST_CASE("composition_test, flip")
{
    using namespace fplus;

    auto APlusTwoTimesB = [](int a, int b) { return a + 2 * b; };
    auto TwoTimesAPlusB = [](int a, int b) { return 2 * a + b; };
    REQUIRE_EQ((flip(APlusTwoTimesB)(2, 1)), 5);
    REQUIRE_EQ((flip(TwoTimesAPlusB)(1, 2)), 5);
}

TEST_CASE("composition_test, logical")
{
    using namespace fplus;
    auto is1 = [](int x) { return x == 1; };
    auto is2 = [](int x) { return x == 2; };
    REQUIRE_FALSE((logical_not(is1)(1)));
    REQUIRE((logical_not(is1)(2)));

    REQUIRE((logical_or(is1, is2)(1)));
    REQUIRE((logical_or(is1, is2)(2)));
    REQUIRE_FALSE((logical_or(is1, is2)(3)));
    REQUIRE_FALSE((logical_and(is1, is2)(1)));
    REQUIRE((logical_and(is1, is1)(1)));
    REQUIRE_FALSE((logical_xor(is1, is1)(1)));
    REQUIRE((logical_xor(is2, is1)(1)));
    REQUIRE_FALSE((logical_xor(is2, is2)(1)));
}

TEST_CASE("composition_test, apply_to_pair")
{
    using namespace fplus;
    auto APlusTwoTimesB = [](int a, int b) { return a + 2 * b; };
    REQUIRE_EQ((apply_to_pair(APlusTwoTimesB, std::make_pair(1, 2))), 5);
    REQUIRE_EQ((apply_to_pair(APlusTwoTimesBFunc, std::make_pair(1, 2))), 5);
}

TEST_CASE("composition_test, state")
{
    using namespace fplus;
    CompositionTestState state(1);
    REQUIRE_EQ(state.Get(), 1);
    auto stateAdd = std::mem_fn(&CompositionTestState::Add);

    stateAdd(state, 2);
    REQUIRE_EQ(state.Get(), 3);

    //auto stateAddBoundFPP = Bind1of2(stateAdd, &state); // crashes VC2015 compiler
    //stateAddBoundFPP(3);

    auto stateAddBoundStl = std::bind(&CompositionTestState::Add, std::placeholders::_1, std::placeholders::_2);
    stateAddBoundStl(state, 3);
    REQUIRE_EQ(state.Get(), 6);
}

TEST_CASE("composition_test, memoize")
{
    using namespace fplus;
    auto f = memoize(square<int>);
    REQUIRE_EQ(f(2), 4);
    REQUIRE_EQ(f(2), 4);
    REQUIRE_EQ(f(3), 9);
    REQUIRE_EQ(f(3), 9);

    const auto add = [](int x, int y) -> int
    {
        return x + y;
    };
    auto add_memo = memoize_binary(add);
    REQUIRE_EQ(add_memo(2, 3), 5);
    REQUIRE_EQ(add_memo(2, 3), 5);
    REQUIRE_EQ(add_memo(1, 2), 3);
    REQUIRE_EQ(add_memo(1, 2), 3);

    const auto fibo_memo = memoize_recursive(fibo_cont);

    for (std::uint64_t n = 0; n < 10; ++n)
    {
        REQUIRE_EQ(fibo_memo(n), fibo(n));
    }
}

TEST_CASE("composition_test, constructor_as_function")
{
    using namespace fplus;

    struct foo
    {
        foo(int a, int b) : a_(a), b_(2*b) {}
        int a_;
        int b_;
    };
    const auto create_foo = constructor_as_function<foo, int, int>;
    const auto my_foo = create_foo(1,2);
    REQUIRE_EQ(my_foo.a_, 1);
    REQUIRE_EQ(my_foo.b_, 4);
}
