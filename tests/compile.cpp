#include <string>

#include <jac/features/filesystemFeature.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>

#ifdef COMPILE_CFG_ONLY
    #include "compiler/compileInterpEvalFeature.h"
#else
    #include <jac/features/aotEvalFeature.h>
#endif

#undef CHECK


#include "util.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>


TEST_CASE("Basic", "[aot]") {
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

    SECTION("Simpler") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                return a + b + 1;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"4"});
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

    SECTION("Shadow") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                let c: Int32 = 0;
                {
                    let c: Int32 = a + b;
                    b = c;
                }
                return b;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"3"});
    }

    SECTION("Dead code") {
        machine.initialize();
        std::string code(R"(
            function fun(): Int32 {
                let c: Int32 = 0;
                return 1;

                c = 2;
                return c;
            }
            report(fun());
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1"});
    }

    SECTION("Dead code block") {
        machine.initialize();
        std::string code(R"(
            function fun(): Int32 {
                let c: Int32 = 0;
                {
                    return 1;
                    c = 2;
                }
                return c;
            }
            report(fun());
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1"});
    }

    SECTION("Dead code branch") {
        machine.initialize();
        std::string code(R"(
            function fun(): Int32 {
                let c: Int32 = 0;
                if (c == 0) {
                    return 1;
                    c = 2;
                }
                else {
                    return 3;
                }
                return c;
            }
            report(fun());
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1"});
    }
}


TEST_CASE("Control flow", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

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
                if (n == 0 || n == 1) {
                    return n;
                }

                let first: Int32 = fib(n - 1);
                let second: Int32 = fib(n - 2);
                return first + second;
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

    SECTION("While") {
        machine.initialize();
        std::string code(R"(
            function pow(a: Int32, b: Int32): Int32 {
                let c: Int32 = 1;
                while (b > 0) {
                    c = c * a;
                    b -= 1;
                }

                return c;
            }

            report(pow(2, 8));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"256"});
    }

    SECTION("Empty while") {
        machine.initialize();
        std::string code(R"(
            function pow(a: Int32, b: Int32): Int32 {
                while (b == 0) {}

                return a;
            }

            report(pow(2, 8));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2"});
    }

    SECTION("Do while") {
        machine.initialize();
        std::string code(R"(
            function add(a: Int32, b: Int32): Int32 {
                let c: Int32 = 0;
                do {
                    c += a;
                    b = b - 1;
                } while (b > 0);

                return c;
            }

            report(add(2, 8));
            report(add(3, 0));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"16", "3"});
    }

    SECTION("For") {
        machine.initialize();
        std::string code(R"(
            function mul(a: Int32, b: Int32): Int32 {
                let c: Int32 = 0;
                for (let i: Int32 = 0; i < b; i += 1) {
                    c = c + a;
                }

                return c;
            }

            report(mul(2, 8));
            report(mul(3, 0));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"16", "0"});
    }

    SECTION("Iterative fib") {
        machine.initialize();
        std::string code(R"(
            function fib(n: Int32): Int32 {
                let a: Int32 = 0;
                let b: Int32 = 1;
                let c: Int32 = 0;
                for (let i: Int32 = 0; i < n; i++) {
                    c = a + b;
                    a = b;
                    b = c;
                }

                return a;
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

    SECTION("Early return") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                if (a > b) {
                    return a;
                }
                else {
                    return b;
                }
            }

            report(fun(1, 2));
            report(fun(4, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2", "4"});
    }

    SECTION("Bool conversion") {
        machine.initialize();
        std::string code(R"(
            function pos(a: Int32): Int32 {
                while (a) {
                    if (a) {
                        for (; a;) {
                            return 1;
                        }
                        return 2;
                    }
                    return 3;
                }
                return 4;
            }
            function neg(a: Int32): Int32 {
                while (a) {
                    return 1;
                }
                if (a) {
                    return 2;
                }
                for (;a ;) {
                    return 3;
                }
                return 4;
            }

            report(pos(1));
            report(neg(0));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1", "4"});
    }

    SECTION("Break") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32): Int32 {
                let c: Int32 = 0;
                for (let i: Int32 = 0; i < a; ++i) {
                    let j: Int32 = 0;
                    while (j < a) {
                        c++;
                        if (j == i) {
                            break;
                        }
                        j += 1;
                    }
                }
                return c;
            }

            report(fun(4));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"10"});
    }

    SECTION("Continue") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32): Int32 {
                let c: Int32 = 0;
                for (let i: Int32 = 0; i < a; i++) {
                    let j: Int32 = 0;
                    while (j < a) {
                        if (++j == i) {
                            continue;
                        }
                        c++;
                    }
                }
                return c;
            }

            report(fun(4));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"13"});
    }
}


TEST_CASE("Types", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

    SECTION("Int32") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32): Int32 {
                return a + 1;
            }

            report(fun(1234));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1235"});
    }

    SECTION("Float") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Float): Float {
                return a + 1.2;
            }

            report(fun(1234.5));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1235.7"});
    }

    SECTION("Bool") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Bool): Bool {
                return !a;
            }

            report(fun(true));
            report(fun(false));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"false", "true"});
    }

    SECTION("Void") {
        machine.initialize();
        std::string code(R"(
            function fun(): Void {
                return;
            }

            report(fun());
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"undefined"});
    }

    SECTION("Mixed") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Float, c: Bool): Float {
                if (a < b) {
                    return (a + b) + c;
                }
                else {
                    return (a + c) - b;
                }
            }

            report(fun(1, 2.5, false));
            report(fun(3, 2.5, true));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"3.5", "1.5"});
    }
}


TEST_CASE("Any", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

    SECTION("Copy") {
        machine.initialize();
        std::string code(R"(
            function fun(a: any): any {
                let b: any = a;
                return b;
            }

            let o = new Object();
            o.x = 1234;
            report(fun(o).x);
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1234"});
    }

    SECTION("Copy multiple") {
        machine.initialize();
        std::string code(R"(
            function fun(a: any, b: any): any {
                let c: any = a;
                let d: any = b;

                return d;
            }

            let o = new Object();
            o.x = 1234;
            let p = new Object();
            p.x = 5678;
            report(fun(o, p).x);
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"5678"});
    }

    SECTION("Mixed") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: any): any {
                let c: Int32 = a;
                let d: any = b;

                return d;
            }

            let o = new Object();
            o.x = 1234;
            let p = new Object();
            p.x = 5678;
            report(fun(o, p).x);
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"5678"});
    }

    SECTION("Wrap-unwrap") {
        machine.initialize();
        std::string code(R"(
            function toInt32(a: any): Int32 { return a; }
            function toFloat(a: any): Float { return a; }
            function toBool(a: any): Bool { return a; }

            function _fromInt32(a: Int32): Int32 { return a; }
            function _fromFloat(a: Float): Float { return a; }
            function _fromBool(a: Bool): Bool { return a; }

            report(toInt32(1234));
            report(toFloat(5678.9));
            report(toBool(true));

            report(_fromInt32(1234));
            report(_fromFloat(5678.9));
            report(_fromBool(false));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{
            "1234", "5678.9", "true",
            "1234", "5678.9", "false"
        });
    }

    SECTION("Call") {
        machine.initialize();
        std::string code(R"(
            function id(a: any): any {
                return a;
            }

            function fun(a: any): any {
                return id(a);
            }

            report(id(7));
            report(fun(2.5));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"7", "2.5"});
    }

    SECTION("Arithmetic") {
        machine.initialize();
        std::string code(R"(
            function add(a: any, b: any): any {
                return a + b;
            }
            function sub(a: any, b: any): any {
                return a - b;
            }
            function mul(a: any, b: any): any {
                return a * b;
            }
            function div(a: any, b: any): any {
                return a / b;
            }
            function rem(a: any, b: any): any {
                return a % b;
            }

            report(add(1, 2));
            report(add(1.5, 2.5));
            report(add(true, false));

            report(sub(1, 2));
            report(sub(1.5, 2.5));
            report(sub(true, false));

            report(mul(1, 2));

            report(div(1, 2));

            report(rem(5, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{
            "3", "4", "1",
            "-1", "-1", "1",
            "2",
            "0.5",
            "1"
        });
    }
}


TEST_CASE("Object", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

    SECTION("Object") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): Object {
                let b: Object = a;
                let c: Object = b;

                return c;
            }

            let o = new Object();
            o.x = 1234;
            report(test(o).x);
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1234"});
    }

    SECTION("Get member") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): any {
                let c: any = a.b;

                return c;
            }

            let o = new Object();
            o.b = 42;
            report(test(o));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Get member brackets") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: any): any {
                let c: any = a["b"];
                return c;
            }

            let o = new Object();
            o["b"] = 42;
            report(test(o, 42));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Set member") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: any): Void {
                a.b = b;
            }

            let o = new Object();
            test(o, 42);
            report(o.b);
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Set member brackets") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: any): Void {
                a["b"] = b;
            }

            let o = new Object();
            test(o, 42);
            report(o.b);
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Set member int int") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: Int32): Void {
                a[0] = b;
            }

            let o = new Object();
            test(o, 42);
            report(o[0]);
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Get member int") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): any {
                let c: any = a[0];
                return c;
            }

            let o = new Object();
            o[0] = 42;
            report(test(o));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Set member int") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: any): Void {
                a[0] = b;
            }

            let o = new Object();
            test(o, 42);
            report(o[0]);
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Call(void)") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): any {
                return a();
            }

            function memFn() {  // no type annotations -> not compiled
                return 42;
            }

            report(test(memFn));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Call member(void)") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): any {
                let c: any = a.b();
                return c;
            }

            function memFn() {  // no type annotations -> not compiled
                return 42;
            }

            let o = new Object();
            o.b = memFn;
            report(test(o));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Call with args") {
        machine.initialize();
        auto code = R"(
            function test(a: Object, b: Int32): any {
                let c: any = a.b(b);
                return c;
            }

            function memFn(a) {  // no type annotations -> not compiled
                return a + 1;
            }

            let o = new Object();
            o.b = memFn;
            report(test(o, 41));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("Call this") {
        machine.initialize();
        auto code = R"(
            function test(a: Object): any {
                let c: any = a.b();
                return c;
            }

            function memFn() {  // no return type -> not compiled
                return this.a;
            }

            let o = new Object();
            o.a = 42;
            o.b = memFn;
            report(test(o));
        )";

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }
}


TEST_CASE("Operators", "[aot]") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::AotEvalFeature,
        TestReportFeature
    >;

    Machine machine;

    SECTION("Arithmetic") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                return a + b - 2 * 4 / 2 % 3;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2"});
    }

    SECTION("Unary") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32): Int32 {
                return -a + +a;
            }

            report(fun(1));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"0"});
    }

    SECTION("Bitwise") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                return a & b | a ^ b;
            }

            report(fun(1, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"3"});
    }

    SECTION("Shift") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                return (a << b) >> 1;
            }

            report(fun(1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2"});
    }

    SECTION("Assignment") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32, c: Int32): Int32 {
                a += b;
                a -= c;
                a *= b;
                a /= c;
                a %= b;
                a &= b;
                a |= c;
                a ^= b;
                a <<= c;
                a >>= b;
                return a;
            }

            report(fun(1, 2, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2"});
    }

    SECTION("Update") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Int32, b: Int32): Int32 {
                a++;
                --b;
                let c: Int32 = --a + b++;
                return c;
            }

            report(fun(1, 2));
            report(fun(3, 4));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"2", "6"});
    }

    SECTION("Short-circuit") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Bool, b: Bool, c: Bool): Bool {
                return a && b || c;
            }

            report(fun(true, false, true));
            report(fun(false, false, false));
            report(fun(false, true, false));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"true", "false", "false"});
    }

    SECTION("Assignment short-circuit") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Bool, b: Bool, c: Bool): Bool {
                a &&= b;
                c ||= a;
                return c;
            }

            report(fun(true, false, true));
            report(fun(false, false, false));
            report(fun(false, true, false));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"true", "false", "false"});
    }

    SECTION("Conditional") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Bool, b: Int32, c: Int32): Int32 {
                return a ? b : c;
            }

            report(fun(true, 1, 2));
            report(fun(false, 1, 2));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1", "2"});
    }

    SECTION("Complex conditional") {
        machine.initialize();
        std::string code(R"(
            function fun(a: Bool, b: Bool, c: Int32, d: Int32, e: Int32): Int32 {
                return a && b ? c : a || b ? d : e;
            }

            report(fun(true, true, 1, 2, 3));
            report(fun(true, false, 1, 2, 3));
            report(fun(false, true, 1, 2, 3));
            report(fun(false, false, 1, 2, 3));
        )");

        evalCode(machine, code, "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"1", "2", "2", "3"});
    }
}
