#include "parser.h"

#include <stack>

namespace lox {
TokenOpType ASTNode::op_type() const {
    if (must_be_op_type.has_value())
        return must_be_op_type.value();

    const auto type = token.op_type();
    switch (type) {
    case UNARY_OR_BINARY:
        // a validation should have been performed earlier, if size > 2, an error should have been printed
        return this->children.size() == 1 ? UNARY : BINARY;
    default:
        return type;
    }
}

AST::AST(Scanner& scanner, bool autobuild) : scanner(scanner), root(std::make_unique<ASTNode>(ASTNode{AST_ROOT, "", std::monostate{}, 0, 0})) {
    if (autobuild)
        build();
}

/**
 * @brief Reorders the AST for operator precedence.
 *
 * This function handles cases like `a * b - c`, where the AST is initially built as `(* a (- b c))`.
 * It swaps the nodes to correctly represent operator precedence, resulting in `(- (* a b) c)`.
 *
 * @param operator_node The new operator node being inserted (e.g., `-`).
 * @param parent The current parent node in the AST (e.g., `*`).
 * @param parentNodes The stack of parent nodes in the AST.
 */
void reorder_for_operator_precedence(std::unique_ptr<ASTNode> operator_node, ASTNode* parent, std::stack<ASTNode*>& parentNodes) {
    auto parent_expression = parent;
    auto grandparent_expression = parent_expression->parent;
    operator_node->parent = grandparent_expression;
    auto lhs_operand_node = std::move(grandparent_expression->children.back()); // lhs should be the parent expression itself
    operator_node->children.push_back(std::move(lhs_operand_node));
    lhs_operand_node = nullptr;

    // replace the parent expression with the operator node
    grandparent_expression->children.back() = std::move(operator_node);
    operator_node = nullptr;
    parent_expression->parent = grandparent_expression->children.back().get(); // parent expression parent is the operator node
    parentNodes.top() = grandparent_expression->children.back().get();
}


void AST::handle_binary_operators(const Token& curr_token, ASTNode* parent, std::stack<ASTNode*>& parentNodes) const {
    auto operator_node = std::make_unique<ASTNode>(ASTNode{
        .token = scanner.next_token(),
        .parent = parent,
        .must_be_op_type = BINARY
    });

    if (curr_token.type == RIGHT_PAREN) { // the whole parenthesis expression should be the lhs operand
        reorder_for_operator_precedence(std::move(operator_node), parent, parentNodes);
        return;
    }

    if (parent->op_type() == BINARY) {
        if (parent->token.op_priority() >= operator_node->token.op_priority()) {
            auto rhs_node = std::make_unique<ASTNode>(ASTNode{
                .token = curr_token,
                .parent = parent
            });
            parent->children.push_back(std::move(rhs_node));

            // e.g. a * b - c, where parent = (* a b), operator_node = -
            // in this case we should do operator_node = (- (* a b) <should be c>)
            // reorder is needed otherwise it'll end up as (* a (- b c))
            reorder_for_operator_precedence(std::move(operator_node), parent, parentNodes);
            return;
        }
    }

    // not a parenthesis or binary expression, may be a literal or a number
    std::unique_ptr<ASTNode> lhs_operand_node = std::make_unique<ASTNode>(ASTNode{
        .token = curr_token,
        .parent = operator_node.get()
    });
    operator_node->children.push_back(std::move(lhs_operand_node));
    lhs_operand_node = nullptr;
    parent->children.push_back(std::move(operator_node));
    operator_node = nullptr;
    parentNodes.push(parent->children.back().get());
}

void AST::build() {
    std::stack<ASTNode*> parentNodes;
    parentNodes.push(root.get());
    while (true) {
        Token token = scanner.next_token();
        if (token.type == EOF_TOKEN)
            break;

        auto parent = parentNodes.top();

        switch (token.type) {
        // all these have operands that are expected to be encountered next
        case MINUS: // as binary operator is handled in the default case
        case PLUS: // as binary operator is handled in the default case
        case NOT:
        case LEFT_PAREN: {
            auto operator_node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            parent->children.push_back(std::move(operator_node)); // The tree itself should be the owner of all the nodes (hence the std::move)
            operator_node = nullptr;
            parentNodes.push(parent->children.back().get());
            break;
        }
        default:
            // handle binary arithmetic operators
            if (token.can_be_arithmetic_operand() && scanner.peek_next().is_arithmetic_operator()) {
                handle_binary_operators(token, parent, parentNodes);
                break;
            }

            auto node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            parent->children.push_back(std::move(node));
            node = nullptr;

            if (token.type == RIGHT_PAREN)
                parentNodes.pop(); // the parent for the next node shouldn't be the current group
        }

        // pop from the stack all those operators whose operands have been provided
        bool keep_poping = true;
        while (!parentNodes.empty() && keep_poping) {
            keep_poping = false;
            switch (parentNodes.top()->op_type()) {
            case UNARY: // operand has just been added in lines above
                if (parentNodes.top()->children.size() == 1) {
                    parentNodes.pop(); // operand has been provided
                    keep_poping = true;
                }
                break;
            case BINARY:
                if (parentNodes.top()->children.size() == 2) {
                    parentNodes.pop(); // both operands have been provided
                    keep_poping = true;
                }
                break;
            default:{};
            }
        }
    }



    // TODO SYNTAX ERRORS
    // pop from the stack all those operators whose operands have been provided
    bool keep_poping = true;
    while (!parentNodes.empty() && keep_poping) {
        keep_poping = false;
        switch (parentNodes.top()->op_type()) {
        case UNARY: // operand has just been added in lines above
            if (parentNodes.top()->children.size() == 1) {
                parentNodes.pop(); // operand has been provided
                keep_poping = true;
            } else
                // syntax error
            break;
        case BINARY:
            if (parentNodes.top()->children.size() == 2) {
                parentNodes.pop(); // both operands have been provided
                keep_poping = true;
            } else
                // syntax error
            break;
        default:{};
        }
    }
}
}
