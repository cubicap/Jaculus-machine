#include <jac/machine/machine.h>
#include <jac/features/stdioFeature.h>
#include <jac/features/eventLoopFeature.h>
#include <jac/features/eventQueueFeature.h>
#include <jac/features/timersFeature.h>
#include <jac/features/yieldFeature.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>
#include <jac/machine/values.h>
#include <string>

#include <jac/features/replFeature.h>

using Machine =
    EventLoopTerminal<
    TimersFeature<
    YieldFeature<
    EventLoopFeature<
    EventQueueFeature<
    StdioFeature<
    jac::ModuleLoaderFeature<
    jac::FilesystemFeature<
    jac::MachineBase
>>>>>>>>;


const char* code = R"(
import * as printer from 'printer';
import * as test from 'test_files/test.js';

printer.print_sm("__Hello World");
test.hola();

exit(0);
)";

const char* asyncTest = R"(
// import * as printer from 'printer';

async function test1() {
    while (true) {
        // print("test1");
        await _yield();
    }
}
async function test2() {
    while (true) {
        // print("test2");
        await _yield();
    }
}

test1();
test2();


createTimer(1000, () => {
    print("Yields: " + yieldCount());
    print("Count: " + count)
}, true);

createTimer(5000, () => {
    print("Yields: " + yieldCount());
    print("Count: " + count)
    print("Exiting...");
    exit(0);
}, true);



let count = 0;

function* test_generator1() {
    while (true) {
        // print("test1");
        count++;
        yield;
    }
}

function* test_generator2() {
    while (true) {
        // print("test2");
        count++;
        yield;
    }
}


runParalel(test_generator1(), test_generator2());

let gen1 = test_generator1();
let gen2 = test_generator2();
async function run() {
    while (true) {
        gen1.next();
        gen2.next();
        fake_yield();
    }
}
run();

)";



// int runRepl() {
//     std::cout << "Welcome to Jaculus machine! Type 'exit' to exit." << std::endl;
//     using ReplMachine =
//         EventLoopTerminal<
//         ReplFeature<
//         AsyncTimersFeature<
//         YieldFeature<
//         AsyncEventLoopFeature<
//         PrinterFeature<
//         jac::ModuleLoaderFeature<
//         jac::FilesystemFeature<
//         jac::MachineBase
//     >>>>>>>>;

//     ReplMachine machine;
//     machine.initialize();

//     auto replThread = std::thread([&machine]() {
//         std::string prompt;
//         while (true) {
//             std::cout << "> " << std::flush;
//             std::getline(std::cin, prompt);
//             if (prompt == "exit") {
//                 break;
//             }
//             if (prompt == "\\") {
//                 prompt = "";
//                 std::string line;
//                 while (true) {
//                     std::cout << ". " << std::flush;
//                     std::getline(std::cin, line);
//                     if (line == "\\") {
//                         break;
//                     }
//                     prompt += "\n" + line;
//                 }
//             }
//             auto result = machine.prompt(prompt);
//             std::cout << result.toString() << std::endl;
//         }

//         machine.exit(0);
//     });

//     while (true) {
//         try {
//             machine.runEventLoop();

//             replThread.join();
//             std::cout << "Exit code: " << machine.getExitCode() << std::endl;

//             return 0;
//         } catch (jac::Exception& e) {
//             std::cout << "evloop: exception: " << e.toString() << std::endl;
//             std::cout << "evloop: stack: " << e.stackTrace() << std::endl;
//         }
//     }

//     return 0;
// }


int main() {
    // return runRepl();

    std::cout << "Start" << std::endl;
    std::cout << "Running on thread: " << std::this_thread::get_id() << std::endl;

    for (int i = 0; i < 3; i++) {
        std::cout << "--- Run " << i << " ---" << std::endl;
        Machine machine;
        std::cout << "Machine created on thread: " << std::this_thread::get_id() << std::endl;
        machine.initialize();
        std::cout << "Machine initialized on thread: " << std::this_thread::get_id() << std::endl;
        try {
            auto val = machine.eval(asyncTest, "main.js", JS_EVAL_TYPE_MODULE);
            // auto val = machine.evalFile("loop.js");
            machine.runEventLoop();

            std::cout << "Exit code: " << machine.getExitCode() << std::endl;
        } catch (jac::Exception& e) {
            std::cout << "Exception: " << e.toString() << std::endl;
            std::cout << "Stack: " << e.stackTrace() << std::endl;
        }
        std::cout << "--- End " << i << " ---" << std::endl;
    }
}
