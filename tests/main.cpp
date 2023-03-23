#include <jac/machine/machine.h>
#include <jac/features/basicStreamFeature.h>
#include <jac/features/stdioFeature.h>
#include <jac/features/eventLoopFeature.h>
#include <jac/features/eventQueueFeature.h>
#include <jac/features/timersFeature.h>
#include <jac/features/yieldFeature.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>
#include <jac/machine/values.h>
#include <jac/machine/class.h>
#include <string>


using Machine =
    EventLoopTerminal<
    TimersFeature<
    YieldFeature<
    EventLoopFeature<
    EventQueueFeature<
    ModuleLoaderFeature<
    FilesystemFeature<
    StdioFeature<
    BasicStreamFeature<
    jac::MachineBase
>>>>>>>>>;


const char* code = R"(
import { stderr, stdout, stdin } from 'stdio';

stdout.print('Enter text:');
let input = stdin.readLine();
stdout.println('You entered: ' + input);

exit(0);
)";


int main() {
    std::cout << "Start" << std::endl;
    std::cout << "Running on thread: " << std::this_thread::get_id() << std::endl;

    Machine machine;
    machine.initialize();
    try {
        auto val = machine.eval(code, "main.js", JS_EVAL_TYPE_MODULE);
        machine.eventLoop_run();

        std::cout << "Exit code: " << machine.eventLoop_getExitCode() << std::endl;
    } catch (jac::Exception& e) {
        std::cout << "Exception: " << e.toString() << std::endl;
        std::cout << "Stack: " << e.stackTrace() << std::endl;
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
