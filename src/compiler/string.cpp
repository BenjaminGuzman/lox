#include "../include/compiler.h"
#include "scanner/token.h"
#include <string>
#include <cstring>
#include <unordered_set>

static std::unordered_set<std::string> interned_strings;

extern "C" {
// extern "C" is needed 'cause llvm may call this functions by their exact name.
// Name mangling may make the OS to not find the symbols
    /**
     * Concatenates two C-strings and returns an interned string pointer
     * @param a the first string
     * @param b the second string
     * @return a pointer to the interned concatenated string
     */
    char* lox_concat_string(const char* a, const char* b) {
        auto [it, inserted] = interned_strings.insert(std::string(a) + std::string(b));

        // the pointer to the actual string data is safe to return as std::unordered_set
        //  will not change the pointer value when re-hashing, only of course when
        //  changing (e.g., deleting) the element itself
        // But as of now, the interned_strings set is a "constant" set
        //  (that will be deleted when memory is garbage collected)
        return const_cast<char*>(it->c_str());
    }

    /**
     * Frees all dynamically allocated strings during the program's execution
     * This acts as a simple string pool to avoid memory leaks
     */
    void lox_free_strings() {
        interned_strings.clear();
    }

    /**
     * Compares two C-strings for equality
     * @param a the first string
     * @param b the second string
     * @return true if the strings are equal, false otherwise
     */
    bool lox_string_eq(const char* a, const char* b) {
        return std::strcmp(a, b) == 0;
    }
}

namespace lox {

llvm::Value* Compiler::compileString(const std::unique_ptr<ASTNode>& astNode) const {
    const std::string_view sv = StringTokenView(astNode->token).literal();
    const std::string str(sv);
    return builder->CreateGlobalString(str);
}

} // lox
