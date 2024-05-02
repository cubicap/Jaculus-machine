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
                let c = a + b + 3;
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

            function haha(a: Int8, b: Int8, c: Int8, d: Int8, e: Int8, f: Int8, g: Int8, h: Int8,
                          i: Int8, j: Int8, k: Int8, l: Int8, m: Int8, n: Int8, o: Int8, p: Int8,
                          q: Int8, r: Int8, s: Int8, t: Int8, u: Int8, v: Int8, w: Int8, x: Int8,
                          y: Int8, z: Int8, aa: Int8, ab: Int8, ac: Int8, ad: Int8, ae: Int8, af: Int8): Int32 {
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
