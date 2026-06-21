#include "parser.h"

#include <stack>
#include <unordered_map>

#include "internal/utils.h"

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
        [[maybe_unused]] auto _ = build();
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
 * The RHS will be inserted later by the @link AST::build() @endlink loop.
 * This is a sort of Pratt parser with the difference that it doesn't recurse, making it totally safe where recursion is
 * prohibited.
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

    if (parent->op_type() == BINARY && (parent->token.is_arithmetic_operator() || parent->token.is_comparison_operator() || parent->token.is_logical_operator())) {
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

/**
 * Pops from the stack all those operators whose operands have been provided,
 * e.g., if there is a binary operator in the stack that has 2 children, then it will be popped
 * @param parentNodes the stack containing the parent nodes
 * @param ifToBeCompleted the if that may be completed
 * @param scanner the scanner
 */
int popCompleteNodes(std::stack<ASTNode*>& parentNodes, ASTNode** ifToBeCompleted, Scanner& scanner) {
    int n_errors = 0;

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
            if (parentNodes.top()->children.size() == 2 && *ifToBeCompleted == nullptr) {
                // if ifToBeCompleted != nullptr, it means that there is an if waiting to be completed
                //  and we cannot pop elements from the stack yet, as
                //  the "complete" elements in the stack may also be if statements waiting to be completed
                //  those statements must wait for the current if statement to be completed.
                // if those elements are not ifs, they'll be popped anyway, after the if is completed
                //  or it was found that it couldn't be completed due to a missing 'else'
                if (parentNodes.top()->token.type == IF) {
                    // if it's an if, which is complete as a binary operator, then it's a candidate
                    //  to be completed as a ternary operator, if it's followed by else (ignoring } and ;)
                    *ifToBeCompleted = parentNodes.top(); // note that there is no use-after-free issues here as it's a pointer
                }
                parentNodes.pop(); // both operands have been provided
                keep_poping = true;
            }
            break;
        case TERNARY:
            if (parentNodes.top()->token.type == FUN && parentNodes.top()->children.size() == 3) {
                const auto parent = parentNodes.top();
                // check the function is correctly formed
                if (parent->children[0]->token.type != IDENTIFIER) {
                    std::cerr << error_in_file_prefix(scanner.filepath, parent->children[0]->token)
                        << "Function definition expects an identifier as a name. "
                        << "Received " << parent->children[0]->token.lexeme << " instead." << std::endl;
                    ++n_errors;
                }
                if (parent->children[1]->token.type != LEFT_PAREN) {
                    std::cerr << error_in_file_prefix(scanner.filepath, parent->children[1]->token)
                        << "Function definition expects a group of arguments after the name. "
                        << "Received " << parent->children[1]->token.lexeme << " instead." << std::endl;
                    ++n_errors;
                }
                if (parent->children[2]->token.type != LEFT_BRACE) {
                    std::cerr << error_in_file_prefix(scanner.filepath, parent->children[2]->token)
                        << "Function definition expects a group of arguments after the name. "
                        << "Received " << parent->children[2]->token.lexeme << " instead." << std::endl;
                    ++n_errors;
                }
            }
            if (parentNodes.top()->children.size() == 3
                && parentNodes.top()->token.type != LEFT_PAREN /* left paren should be closed with a right paren */) {
                parentNodes.pop(); // all operands have been provided
                keep_poping = true;
            }
            break;
        case QUATERNARY:
            if (parentNodes.top()->children.size() == 4) {
                parentNodes.pop(); // all operands have been provided
                keep_poping = true;
            }
            break;
        default:{};
        }
    }

    return n_errors;
}

int AST::build() const {
    int n_errors = 0;

    // points to the node that is an if statement pending to be completed as ternary, this is used for the "else" keyword
    ASTNode* ifToBeCompleted = nullptr;
    bool keep_consuming_tokens = true;

    std::stack<ASTNode*> parentNodes;
    std::unordered_map<const ASTNode*, int> comma_counts; // TODO REMOVE (only needed for evaluation system)
    parentNodes.push(root.get());
    while (keep_consuming_tokens) {
        Token token = scanner.next_token();

        switch (token.type) {
        case SEMICOLON:
        case RIGHT_BRACE:
        case ELSE:
            break;
        default:
            // if there was an if that was a complete binary operator, i.e., if <condition> <exec-if-true>
            //  and it was meant to be completed as a ternary operator, i.e., if <condition> <if-true> <if-false>
            //  it can only be completed iff between the if and the 'else' is nothing else but ; and }
            if (ifToBeCompleted != nullptr) {
                ifToBeCompleted = nullptr;

                // the complete nodes that were in the stack, were not popped in order to wait for the if to be complete
                //  but now that we now it wasn't complete, we need to pop them
                popCompleteNodes(parentNodes, &ifToBeCompleted, scanner);
            }
        }

        auto parent = parentNodes.top();

        switch (token.type) {
        // all these have operands that are expected to be encountered next
        case MINUS: // as binary operator is handled in the default case
        case PLUS: // as binary operator is handled in the default case
        case NOT:
        case LEFT_BRACE:
        case LEFT_PAREN:
        case VAR:
        case PRINT:
        case IF:
        case FOR:
        case WHILE:
        case RETURN:
        case FUN: {
            auto operator_node = std::make_unique<ASTNode>(ASTNode{
                .token = token,
                .parent = parent,
            });
            if (token.type == MINUS || token.type == PLUS || token.type == NOT)
                operator_node->must_be_op_type = UNARY;
            if (token.type == LEFT_PAREN && parent->token.type == FOR) // for()
                operator_node->must_be_op_type = TERNARY; // for receives a group, and that group must be ternary
            parent->children.push_back(std::move(operator_node)); // The tree itself should be the owner of all the nodes (hence the std::move)
            operator_node = nullptr;
            parentNodes.push(parent->children.back().get());

            if (token.type == IF) {
                // for now let's assume it's just an if <condition> <exec-if-true>
                // however, if we find an else later on, we'll make this ternary, if <condition> <if-true> <if-false>
                parentNodes.top()->must_be_op_type = BINARY;
            }
            break;
        }
        case ELSE: {
            if (ifToBeCompleted == nullptr) {
                std::cerr << error_in_file_prefix(scanner.filepath, token.line, token.col)
                        << "Invalid use of 'else' keyword. Must strictly be the next statement after the 'if' statement"
                        << std::endl;
                ++n_errors;
                break;
            }
            // re-open the if as ternary, i.e., if <condition> <exec-if-true> <exec-if-false>
            ifToBeCompleted->must_be_op_type = TERNARY;
            parentNodes.push(ifToBeCompleted);
            // parent = ifToBeCompleted; // not needed, the loop will start from the beginning
            break;
        }
        case SEMICOLON:
            if (parentNodes.top()->token.type == PRINT || parentNodes.top()->token.type == RETURN) {
                // print/return statement args end when the ';' is found
                if (parentNodes.top()->children.empty() && parentNodes.top()->token.type == PRINT)
                    // According to evaluation system: Print statements expect an expression,
                    // i.e., print should have children
                    ++n_errors;
                parentNodes.pop();
            } else if (parentNodes.top()->parent != nullptr && parentNodes.top()->parent->token.type == FOR) {
                const auto& forNode = parentNodes.top(); // node actually points to a group ()
                if (forNode->children.empty()) {
                    // we've received a for(; <condition?>; <increment?>)
                    //  and we need to convert it to for(<no-op>; <condition?>; <increment?>)
                    forNode->children.emplace_back(std::make_unique<ASTNode>(ASTNode{
                        .token = BasicToken<underlying_t>{IGNORE, "no-op", std::monostate(), 0, 0},
                        .children = {},
                        .parent = forNode
                    }));
                }

                if (forNode->children.size() == 1) {
                    // we've received a for(<init>; <condition?>; <increment?>)
                    if (scanner.peek_next().type == SEMICOLON) { // condition was not given, i.e.,
                        // we've received a for(<init>; ; <increment?>)
                        //  and we need to convert it to for(<init>; true; <increment?>)
                        forNode->children.emplace_back(std::make_unique<ASTNode>(ASTNode{
                            .token = BasicToken<underlying_t>{TRUE, "true", std::string("true"), 0, 0},
                            .children = {},
                            .parent = forNode
                        }));
                    } // else condition was given, i.e., we've received a for(<init>; <condition>; <increment?>)
                }
            }
            break;
        case RIGHT_BRACE: // indicates the '{' (which should be the current parent) has been closed
            if (parentNodes.top()->token.type != LEFT_BRACE) {
                // if we reached a '}', it should mean the current parent is '{', if it's not, then there may be a
                // syntax error somewhere...
                std::cerr << error_in_file_prefix(scanner.filepath, token.line, token.col)
                        << "Premature closure of block. Check statements inside. "
                        << "Will try my best to recover from this, but there are no guarantees I'll recover successfully" << std::endl;
                while (parentNodes.size() > 1 && parentNodes.top()->token.type != LEFT_BRACE) {
                    // pop all the non-braces nodes to "recover" from this
                    ++n_errors;
                    parentNodes.pop();
                }
            }
            if (parentNodes.size() > 1)
                parentNodes.pop(); // the parent for the next node shouldn't be the current group
            break;
        case COMMA:
            if (parentNodes.size() > 1)  // TODO REMOVE (only needed for evaluation system)
                comma_counts[parentNodes.top()]++; // track commas without polluting AST
            break;
        // these shouldn't go to the AST
        case UNTERMINATED_STRING:
        case UNRECOGNIZED:
        case INVALID_NUMBER:
            std::cerr << token.string(scanner.filepath) << std::endl;
            ++n_errors;
            break;
        case EOF_TOKEN:
            keep_consuming_tokens = false;
            break;
        default:
            // This is an iterative implementation of a Pratt parser.
            // The core logic handles operator precedence by rotating nodes in the AST.
            auto next = scanner.peek_next();
            bool valid_binary_operand = token.can_be_arithmetic_operand() || token.can_be_comparison_operand();
            bool valid_binary_operator = next.is_arithmetic_operator() || next.is_comparison_operator() || next.is_logical_operator();
            if (valid_binary_operand && valid_binary_operator) {
                handle_binary_operators(token, parent, parentNodes);
                break;
            }

            // handle assignment operator (or, colon :, for named args
            // tree should look like (=) -> {identifier, value}
            if (next.type == EQ || next.type == COLON) {
                auto assignmentNode = std::make_unique<ASTNode>(ASTNode{
                    .token = next,
                    .parent = parent,
                });
                parent->children.push_back(std::move(assignmentNode));
                assignmentNode = nullptr;
                parentNodes.push(parent->children.back().get());
                parent = parentNodes.top();
                scanner.skip_next(); // skip the next token (=) as it has already been processed
            }

            switch (token.type) {
            // handling the closing of parenthesis here is needed 'cause the group may be used as arg for a binary
            // operation, e.g., (10 - 4) * 2, and we need to handle that, the group should be child of *
            // that's why we can't put this in the switch above
            case RIGHT_PAREN: // indicates the '(' (which should be the current parent) has been closed
                if (parentNodes.top()->token.type != LEFT_PAREN && parentNodes.top()->token.type != FUNC_CALL) {
                    // if we reached a ')', it should mean the current parent is '(', if it's not, then there may be a
                    // syntax error somewhere...
                    std::cerr << error_in_file_prefix(scanner.filepath, token.line, token.col)
                            << "Premature closure of parenthesis. Check statements inside. "
                            << "Will try my best to recover from this, but there are no guarantees I'll recover successfully" << std::endl;
                    while (parentNodes.size() > 1 && parentNodes.top()->token.type != LEFT_PAREN) {
                        // pop all the non-parenthesis nodes to "recover" from this
                        ++n_errors;
                        parentNodes.pop();
                    }
                } else {
                    // TODO REMOVE (only needed for evaluation system)
                    auto group_node = parentNodes.top();

                    // Validate commas strictly via mathematical counts
                    bool is_func_def_params = (group_node->token.type == LEFT_PAREN && group_node->parent != nullptr && group_node->parent->token.type == FUN);
                    bool is_func_call_args = (group_node->token.type == FUNC_CALL);

                    if (is_func_def_params || is_func_call_args) {
                        int expected_commas = group_node->children.empty() ? 0 : group_node->children.size() - 1;
                        int actual_commas = comma_counts[group_node]; // defaults to 0 if not found
                        if (actual_commas != expected_commas) {
                            std::cerr << error_in_file_prefix(scanner.filepath, token.line, token.col)
                                      << (actual_commas < expected_commas ? "Expected ',' between arguments" : "Unexpected ',' in arguments") << std::endl;
                            ++n_errors;
                        }
                    } else if (group_node->token.type == LEFT_PAREN && comma_counts[group_node] > 0) {
                        std::cerr << error_in_file_prefix(scanner.filepath, token.line, token.col)
                                  << "Unexpected ',' inside grouping parenthesis" << std::endl;
                        ++n_errors;
                    }
                    // END TODO REMOVE (only needed for evaluation system)

                    // correctly closing parenthesis. Let's now correctly close the for () (iff it's a for)
                    if (parentNodes.top()->parent != nullptr && parentNodes.top()->parent->token.type == FOR) {
                        const auto& forNode = parentNodes.top(); // node actually points to a group ()
                        if (forNode->children.size() == 2) {
                            // we've received a for(<init>; <condition>;)
                            forNode->children.emplace_back(std::make_unique<ASTNode>(ASTNode{
                                .token = BasicToken<underlying_t>{IGNORE, "no-op", std::monostate(), 0, 0},
                                .children = {},
                                .parent = forNode
                            }));
                        }
                    }
                }
                if (parentNodes.size() > 1)
                    parentNodes.pop(); // the parent for the next node shouldn't be the current group
                break;
            case IDENTIFIER:
                if (scanner.peek_next().type == LEFT_PAREN // it's a function call <function id>()
                    && parentNodes.top()->token.type != FUN /* but not a function definition */) {
                    auto _ = scanner.next_token(); // discard the (

                    // in the end, the tree should contain Token{function name} ->(as children) {function args}
                    const Token funcCallToken = BasicToken<underlying_t>{
                        .type = FUNC_CALL,
                        .lexeme = token.lexeme,
                        .literal = token.lexeme,
                        .line = token.line,
                        .col = token.col,
                    };
                    auto node = std::make_unique<ASTNode>(ASTNode{
                        .token = funcCallToken,
                        .parent = parent,
                        .must_be_op_type = MULTI
                    });
                    parent->children.push_back(std::move(node));
                    parentNodes.push(parent->children.back().get());
                    node = nullptr;
                    break;
                }
            default:
                // current token is likely the (missing) argument of an operator,
                // just push it so that the parent (the operator) gets completed
                auto node = std::make_unique<ASTNode>(ASTNode{
                    .token = token,
                    .parent = parent,
                });
                parent->children.push_back(std::move(node));
                node = nullptr;
                break;
            }
        }

        // pop from the stack all those operators whose operands have been provided
        n_errors += popCompleteNodes(parentNodes, &ifToBeCompleted, scanner);
    }

    // at the end, the parentNodes should only contain the root node, if it contains any other elements, then
    // those elements are "incomplete" and are syntax errors
    while (!parentNodes.empty() && parentNodes.top()->token.type != AST_ROOT) {
        const auto incomplete = parentNodes.top();
        parentNodes.pop();
        std::string message;
        if (incomplete->token.type == LEFT_PAREN)
            message = "Unclosed parenthesis '('";
        else if (incomplete->token.type == LEFT_BRACE)
            message = "Unclosed brace '{'";
        else if (incomplete->op_type() == BINARY && incomplete->children.size() < 2)
            message = "Operator '" + incomplete->token.lexeme + "' is missing its right-hand side operand";
        else if (incomplete->op_type() == UNARY && incomplete->children.empty())
            message = "Operator '" + incomplete->token.lexeme + "' is missing its operand";
        else if (incomplete->op_type() == TERNARY && incomplete->children.size() < 3)
            message = "Operator '" + incomplete->token.lexeme + "' must have 3 operands";
        else
            message = "Incomplete expression near '" + incomplete->token.lexeme + "'";
        std::cerr << error_in_file_prefix(scanner.filepath, incomplete->token.line, incomplete->token.col) << message << std::endl;
        ++n_errors;
    }

    return n_errors;
}
}
