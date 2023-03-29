#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <memory>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>

#include "util.h"


TEST_CASE("To JS value", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    using val = std::tuple<std::string, std::function<jac::Value(jac::ContextRef)>, std::string, std::string>;
    auto values = GENERATE(
        val{"bool", [](jac::ContextRef ctx) { return jac::Value::from(ctx, true); }, "boolean", "true"},
        val{"int", [](jac::ContextRef ctx) { return jac::Value::from(ctx, 42); }, "number", "42"},
        val{"double", [](jac::ContextRef ctx) { return jac::Value::from(ctx, 42.7); }, "number", "42.7"},
        val{"const c string", [](jac::ContextRef ctx) { return jac::Value::from(ctx, "Hello World"); }, "string", "Hello World"},
        val{"non const c string", [](jac::ContextRef ctx) {
                const char* str = "Hello World";
                std::unique_ptr<char[]> strCopy(new char[strlen(str) + 1]);
                strcpy(strCopy.get(), str);

                return jac::Value::from(ctx, strCopy.get());
        }, "string", "Hello World"},
        val{"std::string", [](jac::ContextRef ctx) { return jac::Value::from(ctx, std::string("Hello World")); }, "string", "Hello World"},
        val{"null", [](jac::ContextRef ctx) { return jac::Value::null(ctx); }, "object", "null"},
        val{"undefined", [](jac::ContextRef ctx) { return jac::Value::undefined(ctx); }, "undefined", "undefined"},
        val{"vector<int>", [](jac::ContextRef ctx) { return jac::Value::from(ctx, std::vector<int>{1, 2, 3, 4, 5}); }, "object", "1,2,3,4,5"},
        val{"vector<std::string>", [](jac::ContextRef ctx) { return jac::Value::from(ctx, std::vector<std::string>{"1", "2", "3", "4", "5"}); }, "object", "1,2,3,4,5"},
        val{"tuple<int, string, double>", [](jac::ContextRef ctx) { return jac::Value::from(ctx, std::make_tuple(1, std::string("test"), 3.14)); }, "object", "1,test,3.14"}
    );

    auto [comment, valueGen, type, str] = values;

    DYNAMIC_SECTION(comment) {
        auto value = valueGen(machine._context);

        machine.registerGlobal("value", value);
        evalCode(machine, "report(typeof value); report(String(value));", "test", jac::EvalFlags::Global);

        REQUIRE(machine.getReports() == std::vector<std::string>{type, str});
    }
}


TEST_CASE("toString", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    using val = std::tuple<std::string, std::function<jac::Value(jac::ContextRef)>, std::string, std::string>;
    auto values = GENERATE(
        val{"bool", [](jac::ContextRef ctx) { return jac::Value::from(ctx, true); }, "boolean", "true"},
        val{"int", [](jac::ContextRef ctx) { return jac::Value::from(ctx, 42); }, "number", "42"},
        val{"double", [](jac::ContextRef ctx) { return jac::Value::from(ctx, 42.7); }, "number", "42.7"},
        val{"const c string", [](jac::ContextRef ctx) { return jac::Value::from(ctx, "Hello World"); }, "string", "Hello World"},
        val{"non const c string", [](jac::ContextRef ctx) {
                const char* str = "Hello World";
                std::unique_ptr<char[]> strCopy(new char[strlen(str) + 1]);
                strcpy(strCopy.get(), str);

                return jac::Value::from(ctx, strCopy.get());
        }, "string", "Hello World"},
        val{"std::string", [](jac::ContextRef ctx) { return jac::Value::from(ctx, std::string("Hello World")); }, "string", "Hello World"},
        val{"null", [](jac::ContextRef ctx) { return jac::Value::null(ctx); }, "object", "null"},
        val{"undefined", [](jac::ContextRef ctx) { return jac::Value::undefined(ctx); }, "undefined", "undefined"}
    );

    auto [comment, valueGen, type, str] = values;

    DYNAMIC_SECTION(comment) {
        auto value = valueGen(machine._context);

        std::string result(value.toString());

        REQUIRE(result == str);
    }
}


TEST_CASE("From JS value", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    using sgn = std::tuple<std::string, std::string, std::function<bool(jac::Value)>>;
    auto values = GENERATE(
        sgn{"bool", "true", [](jac::Value value) { return value.to<bool>(); }},
        sgn{"int", "Number(42)", [](jac::Value value) { return value.to<int>() == 42; }},
        sgn{"double", "Number(42.7)", [](jac::Value value) { return value.to<double>(); }},
        sgn{"std::string", "String('Hello World')", [](jac::Value value) { return value.to<std::string>() == "Hello World"; }},
        sgn{"null", "null", [](jac::Value value) { return value.isNull(); }},
        sgn{"undefined", "undefined", [](jac::Value value) { return value.isUndefined(); }},
        sgn{"vector<int>", "[1, 2, 3, 4, 5]", [](jac::Value value) { return value.to<std::vector<int>>() == std::vector<int>{1, 2, 3, 4, 5}; }},
        sgn{"vector<std::string>", "['1', '2', '3', '4', '5']", [](jac::Value value) {
            auto vec = value.to<std::vector<std::string>>();
            return vec == std::vector<std::string>{"1", "2", "3", "4", "5"};
        }},
        sgn{"tuple<int, string, double>", "[1, 'test', 3.14]", [](jac::Value value) {
            auto tuple = value.to<std::tuple<int, std::string, double>>();
            return std::get<0>(tuple) == 1 && std::get<1>(tuple) == "test" && std::get<2>(tuple) == 3.14;
        }}
    );

    auto [comment, valueGen, test] = values;

    DYNAMIC_SECTION(comment) {
        auto value = evalCode(machine, valueGen, "test", jac::EvalFlags::Global);

        CAPTURE(valueGen);
        CAPTURE(value.toString());

        REQUIRE(test(value));
    }
}


TEST_CASE("Object get", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("string key") {
        auto value = evalCode(machine, "({a: 'Hello World'})", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        REQUIRE(object.get("a").to<std::string>() == "Hello World");
    }

    SECTION("int key") {
        auto value = evalCode(machine, "({42: 'Hello World'})", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        REQUIRE(object.get(42).to<std::string>() == "Hello World");
    }

    SECTION("array index") {
        auto value = evalCode(machine, "([1, 2, 3])", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        REQUIRE(object.get(1).to<std::string>() == "2");
    }
}


TEST_CASE("Object set", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("string key") {
        auto value = evalCode(machine, "let x = {}; x", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        object.set("a", jac::Value::from(machine._context, "Hello World"));

        evalCode(machine, "report(x.a)", "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }

    SECTION("int key") {
        auto value = evalCode(machine, "let x = {}; x", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        object.set(42, jac::Value::from(machine._context, "Hello World"));

        evalCode(machine, "report(x[42])", "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }

    SECTION("array index") {
        auto value = evalCode(machine, "let x = []; x", "test", jac::EvalFlags::Global);
        auto object = value.to<jac::Object>();

        object.set(1, jac::Value::from(machine._context, "Hello World"));

        evalCode(machine, "report(x[1])", "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }
}


TEST_CASE("Object create", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("create") {
        auto object = jac::Object::create(machine._context);
        object.set("a", jac::Value::from(machine._context, "Hello World"));
        machine.registerGlobal("x", object);

        evalCode(machine, "report(x.a)", "test", jac::EvalFlags::Global);
        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }
}


TEST_CASE("Object hasProperty", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    auto value = evalCode(machine, "({a: 'Hello World'})", "test", jac::EvalFlags::Global);
    auto object = value.to<jac::Object>();

    REQUIRE(object.hasProperty("a"));
    REQUIRE_FALSE(object.hasProperty("b"));
}


TEST_CASE("Object defineProperty", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("value") {
        auto object = jac::Object::create(machine._context);

        object.defineProperty("a", jac::Value::from(machine._context, "Hello World"));
        REQUIRE(object.hasProperty("a"));
        REQUIRE(object.get<std::string>("a") == "Hello World");
        REQUIRE_THROWS_AS(object.set("a", jac::Value::from(machine._context, "Hallo Welt")), jac::Exception);
    }

    SECTION("writable") {
        auto object = jac::Object::create(machine._context);

        object.defineProperty("a", jac::Value::from(machine._context, "Hello World"), jac::PropFlags::Writable);
        REQUIRE(object.hasProperty("a"));
        REQUIRE(object.get<std::string>("a") == "Hello World");
        object.set("a", jac::Value::from(machine._context, "Hallo Welt"));
        REQUIRE(object.get<std::string>("a") == "Hallo Welt");
    }
}


TEST_CASE("Object deleteProperty", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    auto value = evalCode(machine, "({a: 'Hello World'})", "test", jac::EvalFlags::Global);
    auto object = value.to<jac::Object>();

    REQUIRE(object.hasProperty("a"));
    object.deleteProperty("a");
    REQUIRE_FALSE(object.hasProperty("a"));
}


TEST_CASE("Function") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("call") {
        auto value = evalCode(machine, "function x() { report('Hello World'); }; x", "test", jac::EvalFlags::Global);
        auto function = value.to<jac::Function>();

        function.call<void>();

        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }

    SECTION("call with arguments") {
        auto value = evalCode(machine, "function x(a, b) { report(a + b); }; x", "test", jac::EvalFlags::Global);
        auto function = value.to<jac::Function>();

        function.call<void>(1, 2);

        REQUIRE(machine.getReports() == std::vector<std::string>{"3"});
    }

    SECTION("call with return value") {
        auto value = evalCode(machine, "function x() { return 'Hello World'; }; x", "test", jac::EvalFlags::Global);
        auto function = value.to<jac::Function>();

        auto result = function.call<std::string>();

        REQUIRE(result == "Hello World");
    }

    SECTION("call with return value and arguments") {
        auto value = evalCode(machine, "function x(a, b) { return a + b; }; x", "test", jac::EvalFlags::Global);
        auto function = value.to<jac::Function>();

        int result = function.call<int>(1, 2);

        REQUIRE(result == 3);
    }

    SECTION("call with this") {
        auto value = evalCode(machine, R"(
            let x = {
                a: 42,
                b: function() {
                    report(this.a);
                }
            };
            x;
        )", "test", jac::EvalFlags::Global);

        auto object = value.to<jac::Object>();
        auto function = object.get("b").to<jac::Function>();

        function.callThis<void>(object);

        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("call with this, return value, arguments") {
        auto value = evalCode(machine, R"(
            let x = {
                a: 42,
                b: function(a, b) {
                    return this.a + a + b;
                }
            };
            x;
        )", "test", jac::EvalFlags::Global);

        auto object = value.to<jac::Object>();
        auto function = object.get("b").to<jac::Function>();

        int result = function.callThis<int>(object, 1, 2);

        REQUIRE(result == 45);
    }

    SECTION("invoke member") {
        auto value = evalCode(machine, R"(
            let x = {
                a: 42,
                b: function() {
                    report(this.a);
                }
            };
            x;
        )", "test", jac::EvalFlags::Global);

        auto object = value.to<jac::Object>();
        object.invoke<void>("b");

        REQUIRE(machine.getReports() == std::vector<std::string>{"42"});
    }

    SECTION("invoke member, return value, arguments") {
        auto value = evalCode(machine, R"(
            let x = {
                a: 42,
                b: function(a, b) {
                    return this.a + a + b;
                }
            };
            x;
        )", "test", jac::EvalFlags::Global);

        auto object = value.to<jac::Object>();
        int result = object.invoke<int>("b", 1, 2);

        REQUIRE(result == 45);
    }

    SECTION("call constructor (void)") {
        auto value = evalCode(machine, R"(
            class X {
                constructor() {
                    this.a = 42;
                    report('Hello World');
                }
            };
            X;
        )", "test", jac::EvalFlags::Global);

        auto function = value.to<jac::Function>();
        auto obj = function.callConstructor().to<jac::Object>();

        REQUIRE(obj.get<int>("a") == 42);
        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }

    SECTION("call constructor (int, int)") {
        auto value = evalCode(machine, R"(
            class X {
                constructor(a, b) {
                    this.a = a;
                    this.b = b;
                    report(a + b);
                }
            };
            X;
        )", "test", jac::EvalFlags::Global);

        auto function = value.to<jac::Function>();
        auto object = function.callConstructor(1, 2).to<jac::Object>();

        REQUIRE(object.get<int>("a") == 1);
        REQUIRE(object.get<int>("b") == 2);
        REQUIRE(machine.getReports() == std::vector<std::string>{"3"});
    }
}


TEST_CASE("Promise", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("resolve") {
        auto [promise, resolve, reject] = jac::Promise::create(machine._context);

        machine.registerGlobal("x", promise);

        evalCode(machine, R"(
            x.then(function(value) {
                report(value);
            });
        )", "test", jac::EvalFlags::Global);

        REQUIRE(machine.getReports() == std::vector<std::string>{});

        resolve.call<void>("Hello World");

        JSContext* context = machine._context;
        int err = JS_ExecutePendingJob(JS_GetRuntime(machine._context), &context);
        err = JS_ExecutePendingJob(JS_GetRuntime(machine._context), &context);
        REQUIRE(err == 0);

        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }

    SECTION("reject") {
        auto [promise, resolve, reject] = jac::Promise::create(machine._context);

        machine.registerGlobal("x", promise);

        evalCode(machine, R"(
            x.catch(function(value) {
                report(value);
            });
        )", "test", jac::EvalFlags::Global);

        REQUIRE(machine.getReports() == std::vector<std::string>{});

        reject.call<void>("Hello World");

        JSContext* context = machine._context;
        int err = JS_ExecutePendingJob(JS_GetRuntime(machine._context), &context);
        err = JS_ExecutePendingJob(JS_GetRuntime(machine._context), &context);
        REQUIRE(err == 0);

        REQUIRE(machine.getReports() == std::vector<std::string>{"Hello World"});
    }
}


TEST_CASE("is type", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    using sgn = std::tuple<std::string, std::string, bool, bool, bool, bool, bool>;
    auto [comment, code, isUndefined, isNull, isObject, isFunction, isArray] = GENERATE(
        sgn{"undefined", "undefined", true, false, false, false, false},
        sgn{"null", "null", false, true, false, false, false},
        sgn{"object", "({})", false, false, true, false, false},
        sgn{"function", "function x() {}; x", false, false, true, true, false},
        sgn{"number", "42", false, false, false, false, false},
        sgn{"string", "'Hello World'", false, false, false, false, false},
        sgn{"boolean", "true", false, false, false, false, false},
        sgn{"array", "[1, 2, 3]", false, false, true, false, true}
    );

    DYNAMIC_SECTION(comment) {
        auto value = evalCode(machine, code, "test", jac::EvalFlags::Global);

        REQUIRE(value.isUndefined() == isUndefined);
        REQUIRE(value.isNull() == isNull);
        REQUIRE(value.isObject() == isObject);
        REQUIRE(value.isFunction() == isFunction);
        REQUIRE(value.isArray() == isArray);
    }
}


TEST_CASE("json", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("parse object") {
        auto value = jac::Value::fromJSON(machine._context, R"({"a": 42})");
        REQUIRE(value.isObject());
        REQUIRE(value.to<jac::Object>().get<int>("a") == 42);
    }

    SECTION("parse array") {
        auto value = jac::Value::fromJSON(machine._context, "[1, 2, 3]");
        REQUIRE(value.to<jac::Object>().get<int>(0) == 1);
        REQUIRE(value.to<jac::Object>().get<int>(1) == 2);
        REQUIRE(value.to<jac::Object>().get<int>(2) == 3);
    }

    SECTION("parse string") {
        auto value = jac::Value::fromJSON(machine._context, R"("Hello World")");
        REQUIRE(value.to<std::string>() == "Hello World");
    }

    SECTION("parse number") {
        auto value = jac::Value::fromJSON(machine._context, "42");
        REQUIRE(value.to<int>() == 42);
    }

    SECTION("parse boolean") {
        auto value = jac::Value::fromJSON(machine._context, "true");
        REQUIRE(value.to<bool>() == true);
    }

    SECTION("stringify object") {
        auto value = jac::Value::fromJSON(machine._context, R"({"a": 42})");
        REQUIRE(value.stringify().to<std::string>() == R"({"a":42})");

        REQUIRE(value.stringify(4).to<std::string>() == "{\n    \"a\": 42\n}");
        REQUIRE(value.stringify(2).to<std::string>() == "{\n  \"a\": 42\n}");
    }

    SECTION("stringify array") {
        auto value = jac::Value::fromJSON(machine._context, "[1, 2, 3]");
        REQUIRE(value.stringify().to<std::string>() == "[1,2,3]");

        REQUIRE(value.stringify(4).to<std::string>() == "[\n    1,\n    2,\n    3\n]");
        REQUIRE(value.stringify(2).to<std::string>() == "[\n  1,\n  2,\n  3\n]");
    }
}


TEST_CASE("ArrayBuffer", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("create") {
        auto buffer = jac::ArrayBuffer::create(machine._context, 10);
        machine.registerGlobal("x", buffer);

        REQUIRE(buffer.size() == 10);

        auto view = buffer.typedView<uint8_t>();
        REQUIRE(view.size() == 10);
        REQUIRE(std::vector<uint8_t>(view.begin(), view.end()) == std::vector<uint8_t>(10, 0));

        for (size_t i = 0; i < 10; ++i) {
            view[i] = i;
        }

        evalCode(machine, R"(
            let int8 = new Int8Array(x);
            for (var i = 0; i < int8.length; ++i) {
                report(int8[i]);
            }
        )", "test", jac::EvalFlags::Global);

        REQUIRE(machine.getReports() == std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"});
    }

    SECTION("Get from JS") {
        auto val = evalCode(machine, R"(
            let x = new ArrayBuffer(10);
            x;
        )", "test", jac::EvalFlags::Global);

        auto buffer = val.to<jac::ArrayBuffer>();

        REQUIRE(buffer.size() == 10);

        auto view = buffer.typedView<uint8_t>();
        REQUIRE(view.size() == 10);
        REQUIRE(std::vector<uint8_t>(view.begin(), view.end()) == std::vector<uint8_t>(10, 0));

        for (size_t i = 0; i < 10; ++i) {
            view[i] = i;
        }

        evalCode(machine, R"(
            let int8 = new Int8Array(x);
            for (var i = 0; i < int8.length; ++i) {
                report(int8[i]);
            }
        )", "test", jac::EvalFlags::Global);
    }

    SECTION("From typed Array") {
        auto val = evalCode(machine, R"(
            let x = new Int8Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9]);
            x.buffer;
        )", "test", jac::EvalFlags::Global);

        auto buffer = val.to<jac::ArrayBuffer>();

        REQUIRE(buffer.size() == 10);

        auto view = buffer.typedView<uint8_t>();
        REQUIRE(view.size() == 10);
        std::vector<uint8_t> expected{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        REQUIRE(std::vector<uint8_t>(view.begin(), view.end()) == expected);

        for (size_t i = 0; i < 10; ++i) {
            view[i] = i;
        }

        evalCode(machine, R"(
            for (var i = 0; i < x.length; ++i) {
                report(x[i]);
            }
        )", "test", jac::EvalFlags::Global);
    }
}
