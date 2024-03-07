echo "Running command \"clang-tidy -config-file=.clang-tidy -p build/compile_commands.json [files]\""
echo "Files:"

find \
    src tests \
    \( -iname *.h -o -iname *.cpp \) \
    -exec echo " " {} \; \
    -exec clang-tidy -config-file=.clang-tidy -p build/compile_commands.json {} +
