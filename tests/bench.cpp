#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <jac/features/aotEvalFeature.h>
#include <jac/features/basicStreamFeature.h>
#include <jac/features/evalFeature.h>
#include <jac/features/eventLoopFeature.h>
#include <jac/features/eventQueueFeature.h>
#include <jac/features/stdioFeature.h>
#include <jac/features/util/ostreamjs.h>
#include <jac/machine/class.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>


using MachineInterp = jac::ComposeMachine<
    jac::MachineBase,
    jac::EvalFeature,
    jac::BasicStreamFeature,
    jac::StdioFeature,
    jac::EventQueueFeature,
    jac::EventLoopFeature,
    jac::EventLoopTerminal
>;
using MachineAot = jac::ComposeMachine<
    jac::MachineBase,
    jac::AotEvalFeature,
    jac::BasicStreamFeature,
    jac::StdioFeature,
    jac::EventQueueFeature,
    jac::EventLoopFeature,
    jac::EventLoopTerminal
>;


template<typename Machine>
void run(std::string& code, const auto& defines) {
    Machine machine;
    initializeIo(machine);
    machine.initialize();

    for (const auto& [id, val] : defines) {
        machine.context().getGlobalObject().defineProperty(std::string(id), jac::Value::from(machine.context(), std::string(val)));
    }

    try {
        machine.evalModuleWithEventLoop(code, "main.js");
    } catch (jac::Exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        std::cerr << "Stack: " << e.stackTrace() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}


template<typename Machine>
void repeat(std::string& code, int count, const auto& defines) {
    for (int i = 0; i < count; ++i) {
        std::cerr << '\r' << i << " / " << count << ' ' << std::flush;
        run<Machine>(code, defines);
    }

    std::cerr << '\r' << count << " / " << count << std::endl;
}


int main(const int argc, const char* argv[]) {
    // --path <file> --count <count> --mode <interp|aot> [-D<name>=<value>]

    std::string path;
    int count = 1;
    std::string mode = "interp";
    std::vector<std::pair<std::string_view, std::string_view>> defines;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);

        if (arg == "--path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --path" << std::endl;
                return 1;
            }
            path = argv[++i];
        }
        else if (arg == "--count") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --count" << std::endl;
                return 1;
            }
            count = std::stoi(argv[++i]);
        }
        else if (arg == "--mode") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --mode" << std::endl;
                return 1;
            }
            mode = argv[++i];
        }
        else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'D') {
            auto pos = arg.find('=');
            if (pos == std::string::npos) {
                std::cerr << "Invalid define format: " << arg << std::endl;
                return 1;
            }
            defines.emplace_back(arg.substr(2, pos - 2), arg.substr(pos + 1));
        }
        else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    if (path.empty()) {
        std::cerr << "Path is required" << std::endl;
        return 1;
    }

    std::string code;
    {
        if (!std::filesystem::exists(path)) {
            std::cerr << "File does not exist: " << path << std::endl;
            return 1;
        }
        if (!std::filesystem::is_regular_file(path)) {
            std::cerr << "Not a file: " << path << std::endl;
            return 1;
        }
        std::ifstream file(path);
        if (!file || !file.is_open()) {
            std::cerr << "Failed to open file: " << path << std::endl;
            return 1;
        }

        while (file) {
            std::string line;
            std::getline(file, line);
            code += line + '\n';
        }
    }

    std::cerr << "Running '" << path << "' " << count << " times in " << mode << " mode" << std::endl;

    if (mode == "interp") {
        repeat<MachineInterp>(code, count, defines);
    }
    else if (mode == "aot") {
        repeat<MachineAot>(code, count, defines);
    }
    else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }

    return 0;
}
