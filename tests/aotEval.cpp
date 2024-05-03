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
}
