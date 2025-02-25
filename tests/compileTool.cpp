#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <jac/machine/compiler/ast.h>
#include <jac/machine/compiler/cfg.h>
#include <jac/machine/compiler/cfgDot.h>
#include <jac/machine/compiler/cfgEmit.h>
#include <jac/machine/compiler/cfgSimplify.h>


int main(const int argc, const char* argv[]) {
    // --path <file>

    std::string path;

    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--path") {
            if (i + 1 >= argc) {
                std::cerr << "Missing argument for --path" << std::endl;
                return 1;
            }
            path = argv[++i];
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
    std::cerr << "--- code ---\n" << code << "---" << std::endl;


    bool hadError = false;
    std::vector<std::string> reports;
    jac::lex::Scanner scanner(code, [&hadError, &reports](int line, int col, const std::string& msg) {
        hadError = true;
        reports.push_back("Lex error: " + msg + " at " + std::to_string(line) + ":" + std::to_string(col));
    });

    if (hadError) {
        for (const auto& report : reports) {
            std::cerr << report << '\n';
        }
        throw std::runtime_error("Lex error");
    }

    auto tokens = scanner.scan();

    jac::ast::ParserState state(tokens);

    auto fun = jac::ast::FunctionDeclaration<false, false, false>::parse(state);
    if (!fun || !state.isEnd()) {
        jac::lex::Token errorToken = state.getErrorToken();
        std::cerr << "Parse error: " << state.getErrorMessage()
                    << " at " << errorToken.line << ":" << errorToken.column << '\n';
        throw std::runtime_error("Parse error");
    }

    auto sig = jac::cfg::getSignature(*fun);
    auto cfg = jac::cfg::emit(*fun, sig, {});

    // std::cerr << "--- cfg ---\n";

    // jac::cfg::dotprint::print(std::cout, cfg);

    std::cerr << "--- simplified cfg ---\n";

    jac::cfg::removeEmptyBlocks(cfg);
    jac::cfg::dotprint::print(std::cout, cfg);

    return 0;
}
