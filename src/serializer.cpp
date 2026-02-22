#include "serializer.h"

#include <iostream>
#include <numeric>
#include <sstream>
#include <stack>

using namespace lox;

int Serializer::serialize(Scanner& scanner) {
    int n_unrecognized = 0;
    while (true) {
        Token token = scanner.next_token();
        if (token.type == EOF_TOKEN) {
            std::cout << token.string(scanner.filepath) << std::endl;
            break;
        }

        if (token.type == UNRECOGNIZED || token.type == UNTERMINATED_STRING) {
            std::cerr << token.string(scanner.filepath) << std::endl;
            ++n_unrecognized;
            continue;
        }

        std::cout << token.string(scanner.filepath) << std::endl;
    }

    return n_unrecognized;
}

/**
 *
 * @param type token operator type
 * @return true if the serialization of the given operation should start with '(' and end with ')'
 */
bool operator_should_be_enclosed_by_paren(const TokenOpType type) {
    switch (type) {
    case UNARY:
    case BINARY:
    case MULTI:
        return true;
    default:
        return false;
    }
}

void add_to_buff(const ASTNode* node, std::stringstream& buff) {
    switch (node->token.type) {
    case RIGHT_PAREN: // right parenthesis is not serializable
        return;
    default: {}
    }

    // the parent is serialized as group, thus, its children (which are currently being serialized by this func)
    //  should be prepended with a space
    if (operator_should_be_enclosed_by_paren(node->parent->op_type()))
        buff << " ";

    if (operator_should_be_enclosed_by_paren(node->op_type()))
        buff << "(";

    switch (node->token.type) {
    case LEFT_PAREN:
        buff << "group";
        return;
    default: {}
    }

    bool should_use_lexeme = false;
    std::string serialization = std::visit([&should_use_lexeme]<typename E>(E&& lit) {
        if constexpr (std::is_same_v<std::decay_t<E>, std::string>)
            return lit;
        if constexpr (std::is_same_v<std::decay_t<E>, RealNumber>)
            return to_string(lit);
        if constexpr (std::is_same_v<std::decay_t<E>, std::monostate>) {
            should_use_lexeme = true;
            return std::string("");
        }
        return std::string("");
    }, node->token.get_literal());

    if (should_use_lexeme)
        serialization = node->token.lexeme;

    buff << serialization;
}

/**
 * Adds the children nodes of the given node to the given stack so that it can be traversed in a preorder manner
 * (preorder traversal)
 * @param node the current node whose children will be added to the given stack
 * @param nodes the stack of child nodes (pending nodes to be visited)
 */
void push_children_into_stack(const ASTNode* node, std::stack<const ASTNode*>& nodes) {
    if (operator_should_be_enclosed_by_paren(node->op_type()))
        nodes.push(nullptr); // mark the end of the children of a parent node

    if (!node->children.empty()) {
        for (size_t i = node->children.size() - 1; i != 0; --i)
            nodes.push(node->children.at(i).get());
        nodes.push(node->children.at(0).get());
    }
}

int Serializer::serialize(const AST& ast) {
    std::stack<const ASTNode*> nodes;
    std::stringstream buff;

    push_children_into_stack(ast.root.get(), nodes);
    while (!nodes.empty()) {
        auto curr = nodes.top();
        nodes.pop();
        if (curr == nullptr) { // by now, we've previously processed all the children of a parent
            buff << ")";
            continue;
        }

        add_to_buff(curr, buff);
        push_children_into_stack(curr, nodes);
    }

    std::cout << buff.str() << std::endl;

    return 0;
}