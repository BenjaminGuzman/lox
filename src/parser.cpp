#include "parser.h"

#include <stack>

namespace lox {
TokenOpType ASTNode::op_type() const {
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

void AST::build() {
    std::stack<ASTNode*> parentNodes;
    parentNodes.push(root.get());
    while (true) {
        Token token = scanner.next_token();
        if (token.type == EOF_TOKEN)
            break;

        auto parent = parentNodes.top();

        switch (token.type) {
        case LEFT_PAREN: {
            auto group = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            parent->children.push_back(std::move(group)); // The tree itself should be the owner of all the nodes (hence the std::move)
            parentNodes.push(parent->children.back().get());
            break;
        }
        case RIGHT_PAREN: {
            parentNodes.pop(); // the parent for the next node shouldn't be the current group
            break;
        }
        case NOT: {
            Token next_token = scanner.next_token();
            auto operator_node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            auto operand_node = std::make_unique<ASTNode>(ASTNode{
                .token = next_token,
                .parent = operator_node.get(),
            });
            operator_node->children.push_back(std::move(operand_node));
            parent->children.push_back(std::move(operator_node));
            break;
        }
        case MINUS:
        case PLUS: {
            auto op_node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });

            // construct the RHS
            Token next_token = scanner.next_token();
            auto rhs_node = std::make_unique<ASTNode>(ASTNode{
                .token = next_token,
                .parent = op_node.get()
            });
            bool is_next_token_a_number = next_token.type == NUMBER; // FIXME

            // check if we have LHS (it may have been added as child of the current parent, but it should be a child
            //  of the current operator node)
            bool is_prev_token_a_number = !parent->children.empty() && parent->children.back()->token.type == NUMBER;
            if (is_prev_token_a_number && is_next_token_a_number) { // check if it's a binary operator
                // move the LHS from the parent to the operator node
                auto& lhs_node = parent->children.back();
                lhs_node->parent = op_node.get();
                op_node->children.push_back(std::move(lhs_node));
                parent->children.pop_back();

                // add RHS
                op_node->children.push_back(std::move(rhs_node));
            } else if (is_next_token_a_number) { // check if it's a unary operator
                op_node->children.push_back(std::move(rhs_node));
            } else {
                std::cerr << "Operands are wrong for operator '" << op_node->token.lexeme << "' at " << scanner.filepath << ":" << token.line << ":" << token.col
                          << ". Ignoring these tokens...";
            }

            parent->children.push_back(std::move(op_node));
            break;
        }
        default:
            auto node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            parent->children.push_back(std::move(node));
        }
    }
}
}
