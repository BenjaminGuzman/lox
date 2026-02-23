#include "parser.h"

#include <stack>

namespace lox {
TokenOpType ASTNode::op_type() const {
    if (must_be_op_type.has_value())
        return must_be_op_type.value();

    switch (const auto type = token.op_type()) {
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

void ensure_right_associativity(const ASTNode* parent) {
    ASTNode* grandparent;
    while ((grandparent = parent->parent) != nullptr
        && grandparent->op_type() == BINARY
        && grandparent->token.op_priority() >= parent->token.op_priority()) {
        //         gg                gg
        //        /                /
        //       +(g)             +(p)
        //      /  \     ->      /   \
        //     a   +(p)         +(g)  ?
        //        /  \          /  \
        //       *    ?        a    *
        //      / \                / \
        //     b   c              b   c
        // for the expression a + b * c + d
        auto gg = grandparent->parent;

        // modify grandparent
        auto parent_value = std::move(grandparent->children.back());
        grandparent->children.back() = std::move(parent_value->children.back());
        grandparent->parent = parent_value->parent;

        // modify parent
        parent_value->children.back() = std::move(gg->children.back());
        parent_value->parent = gg;

        // modify grand-grandparent
        gg->children.back() = std::move(parent_value);
        }
}

/**
 * This method replaces the parent (p) by, and adds it as child of, the given operator node (o)
 *          g                g
 *         /                /
 *        p                o
 *      / | \    ->       /
 *     children          p
 *                     / | \
 *                    children
 * g = grandparent
 * @param parent the parent to be replaced by, and added as child of, the given operator node
 * @param operator_node the operator replacing the parent
 * @param parentNodes the stack of parent nodes
 */
void replace_parent_and_make_it_child(ASTNode* parent, std::unique_ptr<ASTNode> operator_node, std::stack<ASTNode*>& parentNodes) {
    auto grandparent = parent->parent;
    auto& parent_ref = grandparent->children.back();
    operator_node->parent = grandparent;
    operator_node->children.push_back(std::move(parent_ref));
    parent_ref = nullptr;

    grandparent->children.back() = std::move(operator_node);
    operator_node = nullptr;
    const auto& operator_node_ref = grandparent->children.back();
    parent->parent = operator_node_ref.get();
    parentNodes.top() = operator_node_ref.get();

    ensure_right_associativity(parentNodes.top());
}

/**
 * Handles binary operators by creating the operator node, and inserting the LHS operand
 * which may be an already existing operand (e.g., a group coming from a parenthesis, or an operator from a nested
 * operation).
 * The RHS will be inserted later by the @link AST::build() @endlink loop
 * @param curr_token the current token (which may be the LHS of the binary operation)
 * @param parent the current parent
 * @param parentNodes the stack of parents
 * @note the call to @link scanner.next_token() @endlink should return a binary operator!
 */
void AST::handle_binary_operators(const Token& curr_token, ASTNode* parent, std::stack<ASTNode*>& parentNodes) const {
    ASTNode* unary_expression = nullptr;
    if (parent->op_type() == UNARY) { // finish the unary expression, e.g., (- 70)
        auto node = std::make_unique<ASTNode>(ASTNode{
            .token = curr_token,
            .parent = parent,
        });
        parent->children.push_back(std::move(node));
        unary_expression = parentNodes.top();
        parentNodes.pop(); // unary expression is complete
        parent = parentNodes.top();
    }

    auto operator_node = std::make_unique<ASTNode>(ASTNode{
        .token = scanner.next_token(),
        .parent = parent,
        .must_be_op_type = BINARY
    });

    if (parent->op_type() == BINARY) {
        if (parent->token.op_priority() >= operator_node->token.op_priority()) {
            // complete the binary expression
            if (parent->children.size() < 2) {
                // due to how we parse unary operators (in the build() loop),
                // the current operator may already be an RHS of a binary operator, and thus,
                // the expression may already be complete. Especially when operands are (- 70)
                // if not, then simply construct and add the RHS
                auto rhs_node = std::make_unique<ASTNode>(ASTNode{
                    .token = curr_token,
                    .parent = parent
                });
                parent->children.push_back(std::move(rhs_node));
            }

            // parent should be a leaf of the current operator, as this implies it (the parent) will be computed first, i.e., it has higher priority
            // e.g. (? means it should have a value but there is none, yet)
            //         g                g
            //        /                /
            //       *                +
            //      / \    ->        / \
            //     a   ?            *   ?
            //                     / \
            //                    a   b
            // for the expression a * b + c (c = ?, will be added later). g = grandparent
            // this is needed to 1) preserve operator priority/precedence and 2) preserve left-to-right precedence

            replace_parent_and_make_it_child(parent, std::move(operator_node), parentNodes);
            return;
        }

        // parent has less priority => it should go up, new operator should go bottom so that it can be computed first
        //  => parent should have as child the new operator

        // complete the binary expression
        // e.g. (? means it should have a value but there is none, yet)
        //         g                g
        //        /                /
        //       +                +
        //      / \    ->        / \
        //     a   ?            a   *
        //                         / \
        //                        b   ?
        // for the expression a + b * c (c = ?, will be added later). g = grandparent
        auto lhs_operand_node = std::make_unique<ASTNode>(ASTNode{
            .token = curr_token,
            .parent = operator_node.get()
        });
        operator_node->children.push_back(std::move(lhs_operand_node));
        operator_node->parent = parent;
        parent->children.push_back(std::move(operator_node));
        parentNodes.top() = parent->children.back().get();
        return;
    }

    if (curr_token.type == RIGHT_PAREN) { // the whole parenthesis expression should be the lhs operand
        // parent should be a leaf of the current operator
        //         g                g
        //        /                /
        //       (                +
        //     / | \    ->       / \
        //    children          (   ?
        //                    / | \
        //                   children
        replace_parent_and_make_it_child(parent, std::move(operator_node), parentNodes);
        return;
    }

    // not a parenthesis, binary or unary expression, may be a literal or a number
    std::unique_ptr<ASTNode> lhs_operand_node;
    if (unary_expression != nullptr) {
        lhs_operand_node = std::move(unary_expression->parent->children.back());
        unary_expression->parent->children.pop_back();
        lhs_operand_node->parent = operator_node.get();
    } else
        lhs_operand_node = std::make_unique<ASTNode>(ASTNode{
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
            if (token.type == MINUS || token.type == PLUS || token.type == NOT)
                operator_node->must_be_op_type = UNARY;
            parent->children.push_back(std::move(operator_node)); // The tree itself should be the owner of all the nodes (hence the std::move)
            operator_node = nullptr;
            parentNodes.push(parent->children.back().get());
            break;
        }
        default:
            // handle binary arithmetic operators (TODO improve this, start from right to left)
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
