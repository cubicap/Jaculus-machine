#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/features/aotEvalFeature.h>
#include <jac/features/filesystemFeature.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>

#include "util.h"


TEST_CASE("Eval", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

    SECTION("Id") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32): Int32 {
                return a;
            }

            report(fun(1234));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1234"});
    }

    SECTION("Simple") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                let c: Int32 = a + b + 3;
                c = c * 2;
                c = +(-c);
                return c;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"-12"});
    }

    SECTION("Many args") {
        machine.initialize();
        std::string code(R"(

            function haha(a: Int32, b: Int32, c: Int32, d: Int32, e: Int32, f: Int32, g: Int32, h: Int32,
                          i: Int32, j: Int32, k: Int32, l: Int32, m: Int32, n: Int32, o: Int32, p: Int32,
                          q: Int32, r: Int32, s: Int32, t: Int32, u: Int32, v: Int32, w: Int32, x: Int32,
                          y: Int32, z: Int32, aa: Int32, ab: Int32, ac: Int32, ad: Int32, ae: Int32, af: Int32): Int32 {
                return a + b + c + d + e + f + g + h
                     + i + j + k + l + m + n + o + p
                     + q + r + s + t + u + v + w + x
                     + y + z + aa + ab + ac + ad + ae + af;
            }

            report(haha(1, -2, 3, -4, 5, -6, 7, -8,
                       9, -10, 11, -12, 13, -14, 15, -16,
                       17, -18, 19, -20, 21, -22, 23, -24,
                       25, -26, 27, -28, 29, -30, 31, -32));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"-16"});
    }

    SECTION("Two") {
        machine.initialize();
        std::string code(R"(
            function id(a: Int32): Int32 {
                return a;
            }

            function fun(a: Int32, b: Int32): Int32 {
                return a + b;
            }

            report(id(7));
            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"7", "3"});
    }

    SECTION("Call") {
        machine.initialize();
        std::string code(R"(
            function id(a: Int32): Int32 {
                return a;
            }

            function fun(a: Int32, b: Int32): Int32 {
                return id(a) + id(b);
            }

            report(id(7));
            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"7", "3"});
    }

    SECTION("Block") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                let c: Int32 = 0;
                {
                    c = a + b;
                }
                return c;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"3"});
    }

    SECTION("If") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                let c: Int32 = 0;
                if (a > b) {
                    c = a;
                }
                else {
                    c = b;
                }
                return c;
            }

            report(fun(1, 2));
            report(fun(4, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2", "4"});
    }

    SECTION("Nested if") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32, c: Int32): Int32 {
                if (a == 0) {
                    if (b > c) {
                        return b;
                    }
                    else {
                        return c;
                    }
                }
                else {
                    return a;
                }
            }

            report(fun(1, 2, 3));
            report(fun(0, 2, 3));
            report(fun(0, 4, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1", "3", "4"});
    }

    SECTION("Recursive fib") {
        machine.initialize();
        std::string code(R"(
            function fib(n: Int32): Int32 {
                if (n == 0) {
                    return 0;
                }
                else if (n == 1) {
                    return 1;
                }

                return fib(n - 1) + fib(n - 2);
            }

            report(fib(0));
            report(fib(1));
            report(fib(2));
            report(fib(3));
            report(fib(4));
            report(fib(5));
            report(fib(6));
            report(fib(7));
            report(fib(8));
            report(fib(9));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"0", "1", "1", "2", "3", "5", "8", "13", "21", "34"});
    }
}
