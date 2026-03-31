/*
 * X# (Xsharp) Recursive Descent Parser Implementation
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Lifecycle ===== */
void parser_init(Parser *p, Token *tokens, int token_count) {
    p->tokens = tokens;
    p->token_count = token_count;
    p->current = 0;
    p->error_count = 0;
    p->had_error = false;
    p->panic_mode = false;
}

/* ===== Helpers ===== */
static Token *current(Parser *p) {
    if (p->current >= p->token_count) return &p->tokens[p->token_count - 1];
    return &p->tokens[p->current];
}

static Token *previous(Parser *p) {
    if (p->current == 0) return &p->tokens[0];
    return &p->tokens[p->current - 1];
}

static bool check(Parser *p, TokenType type) {
    return current(p)->type == type;
}

static bool is_at_end(Parser *p) {
    return current(p)->type == TOK_EOF;
}

static Token *advance(Parser *p) {
    if (!is_at_end(p)) p->current++;
    return previous(p);
}

static bool match(Parser *p, TokenType type) {
    if (!check(p, type)) return false;
    advance(p);
    return true;
}

static void error_at(Parser *p, Token *tok, const char *msg) {
    if (p->panic_mode) return;
    p->panic_mode = true;
    p->had_error = true;
    if (p->error_count < PARSER_MAX_ERRORS) {
        snprintf(p->errors[p->error_count], PARSER_MAX_ERROR,
                 "[line %d:%d] Error at '%.*s': %s",
                 tok->line, tok->column, (int)tok->length, tok->start, msg);
        p->error_count++;
    }
}

static Token *consume(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) return advance(p);
    error_at(p, current(p), msg);
    return current(p);
}

static void synchronize(Parser *p) {
    p->panic_mode = false;
    while (!is_at_end(p)) {
        if (previous(p)->type == TOK_SEMICOLON) return;
        switch (current(p)->type) {
            case TOK_FORGE: case TOK_ENTITY: case TOK_REALM: case TOK_QUEST:
            case TOK_MORPH: case TOK_ETERNAL: case TOK_ORACLE: case TOK_CYCLE:
            case TOK_WHILE: case TOK_UNLEASH: case TOK_SUMMON: case TOK_ENGRAVE:
                return;
            default: advance(p);
        }
    }
}

static char *token_to_string(Token *tok) {
    char *s = malloc(tok->length + 1);
    memcpy(s, tok->start, tok->length);
    s[tok->length] = '\0';
    return s;
}

/* ===== Forward declarations ===== */
static AstNode *parse_expression(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_declaration(Parser *p);
static AstNode *parse_assignment(Parser *p);

/* ===== Expression Parsing (Pratt-style precedence) ===== */
typedef enum {
    PREC_NONE, PREC_ASSIGNMENT, PREC_TERNARY, PREC_PIPE,
    PREC_OR, PREC_AND, PREC_BIT_OR, PREC_BIT_XOR, PREC_BIT_AND,
    PREC_EQUALITY, PREC_COMPARISON, PREC_SHIFT, PREC_TERM,
    PREC_FACTOR, PREC_POWER, PREC_UNARY, PREC_CALL, PREC_PRIMARY
} Precedence;

static Precedence get_precedence(TokenType type) {
    switch (type) {
        case TOK_PIPE_GREATER:  return PREC_PIPE;
        case TOK_OR_OR:         return PREC_OR;
        case TOK_AND_AND:       return PREC_AND;
        case TOK_PIPE:          return PREC_BIT_OR;
        case TOK_CARET:         return PREC_BIT_XOR;
        case TOK_AMPERSAND:     return PREC_BIT_AND;
        case TOK_EQUAL_EQUAL: case TOK_BANG_EQUAL: return PREC_EQUALITY;
        case TOK_LESS: case TOK_GREATER: case TOK_LESS_EQUAL: case TOK_GREATER_EQUAL: return PREC_COMPARISON;
        case TOK_LSHIFT: case TOK_RSHIFT: return PREC_SHIFT;
        case TOK_PLUS: case TOK_MINUS: return PREC_TERM;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return PREC_FACTOR;
        case TOK_POWER: return PREC_POWER;
        default: return PREC_NONE;
    }
}

static AstNode *parse_primary(Parser *p) {
    Token *tok = current(p);

    /* Numbers */
    if (match(p, TOK_INT_LITERAL)) {
        AstNode *node = ast_new_node(NODE_LITERAL_INT, tok->line, tok->column);
        node->as.literal_int.value = tok->literal.int_val;
        return node;
    }
    if (match(p, TOK_FLOAT_LITERAL)) {
        AstNode *node = ast_new_node(NODE_LITERAL_FLOAT, tok->line, tok->column);
        node->as.literal_float.value = tok->literal.float_val;
        return node;
    }

    /* Strings */
    if (match(p, TOK_STRING_LITERAL)) {
        AstNode *node = ast_new_node(NODE_LITERAL_STRING, tok->line, tok->column);
        /* Strip quotes */
        node->as.literal_string.value = strndup(tok->start + 1, tok->length - 2);
        return node;
    }

    /* Booleans */
    if (match(p, TOK_TRUTH)) {
        AstNode *node = ast_new_node(NODE_LITERAL_BOOL, tok->line, tok->column);
        node->as.literal_bool.value = true;
        return node;
    }
    if (match(p, TOK_LIES)) {
        AstNode *node = ast_new_node(NODE_LITERAL_BOOL, tok->line, tok->column);
        node->as.literal_bool.value = false;
        return node;
    }

    /* Null */
    if (match(p, TOK_ABYSS)) {
        return ast_new_node(NODE_LITERAL_NULL, tok->line, tok->column);
    }

    /* Self */
    if (match(p, TOK_SELF)) {
        return ast_new_node(NODE_SELF_EXPR, tok->line, tok->column);
    }

    /* Identifier */
    if (match(p, TOK_IDENTIFIER)) {
        AstNode *node = ast_new_node(NODE_IDENTIFIER, tok->line, tok->column);
        node->as.identifier.name = token_to_string(tok);
        return node;
    }

    /* Parenthesized expression */
    if (match(p, TOK_LPAREN)) {
        AstNode *expr = parse_expression(p);
        consume(p, TOK_RPAREN, "Expected ')' after expression");
        return expr;
    }

    /* Arsenal literal */
    if (match(p, TOK_LBRACKET)) {
        AstNode *node = ast_new_node(NODE_ARSENAL_LITERAL, tok->line, tok->column);
        ast_node_list_init(&node->as.arsenal_literal.elements);
        if (!check(p, TOK_RBRACKET)) {
            do {
                ast_node_list_push(&node->as.arsenal_literal.elements, parse_expression(p));
            } while (match(p, TOK_COMMA));
        }
        consume(p, TOK_RBRACKET, "Expected ']' after array elements");
        return node;
    }

    /* Spell (lambda) */
    if (match(p, TOK_SPELL)) {
        AstNode *node = ast_new_node(NODE_SPELL_EXPR, tok->line, tok->column);
        param_list_init(&node->as.spell.params);
        consume(p, TOK_LPAREN, "Expected '(' after 'spell'");
        if (!check(p, TOK_RPAREN)) {
            do {
                Parameter param = {0};
                Token *name = consume(p, TOK_IDENTIFIER, "Expected parameter name");
                param.name = token_to_string(name);
                if (match(p, TOK_COLON)) {
                    Token *type = advance(p);
                    param.type.name = token_to_string(type);
                }
                param_list_push(&node->as.spell.params, param);
            } while (match(p, TOK_COMMA));
        }
        consume(p, TOK_RPAREN, "Expected ')' after parameters");
        if (match(p, TOK_ARROW)) {
            Token *ret = advance(p);
            node->as.spell.return_type.name = token_to_string(ret);
        }
        consume(p, TOK_LBRACE, "Expected '{' for spell body");
        AstNode *body = ast_new_node(NODE_BLOCK_STMT, tok->line, tok->column);
        ast_node_list_init(&body->as.block.statements);
        while (!check(p, TOK_RBRACE) && !is_at_end(p)) {
            AstNode *stmt = parse_declaration(p);
            if (stmt) ast_node_list_push(&body->as.block.statements, stmt);
        }
        consume(p, TOK_RBRACE, "Expected '}' after spell body");
        node->as.spell.body = body;
        return node;
    }

    /* Conjure (new) */
    if (match(p, TOK_CONJURE)) {
        AstNode *node = ast_new_node(NODE_CONJURE_EXPR, tok->line, tok->column);
        Token *name = consume(p, TOK_IDENTIFIER, "Expected entity name after 'conjure'");
        node->as.conjure.entity_name = token_to_string(name);
        ast_node_list_init(&node->as.conjure.args);
        if (match(p, TOK_LPAREN)) {
            if (!check(p, TOK_RPAREN)) {
                do {
                    ast_node_list_push(&node->as.conjure.args, parse_expression(p));
                } while (match(p, TOK_COMMA));
            }
            consume(p, TOK_RPAREN, "Expected ')' after arguments");
        }
        return node;
    }

    /* Engrave as expression */
    if (match(p, TOK_ENGRAVE)) {
        AstNode *node = ast_new_node(NODE_ENGRAVE_STMT, tok->line, tok->column);
        ast_node_list_init(&node->as.engrave.args);
        consume(p, TOK_LPAREN, "Expected '(' after 'engrave'");
        if (!check(p, TOK_RPAREN)) {
            do {
                ast_node_list_push(&node->as.engrave.args, parse_expression(p));
            } while (match(p, TOK_COMMA));
        }
        consume(p, TOK_RPAREN, "Expected ')' after arguments");
        return node;
    }

    error_at(p, tok, "Expected expression");
    advance(p);
    return ast_new_node(NODE_LITERAL_NULL, tok->line, tok->column);
}

static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);

    for (;;) {
        if (match(p, TOK_LPAREN)) {
            /* Function call */
            AstNode *call = ast_new_node(NODE_CALL_EXPR, expr->line, expr->column);
            call->as.call.callee = expr;
            ast_node_list_init(&call->as.call.args);
            if (!check(p, TOK_RPAREN)) {
                do {
                    ast_node_list_push(&call->as.call.args, parse_expression(p));
                } while (match(p, TOK_COMMA));
            }
            consume(p, TOK_RPAREN, "Expected ')' after arguments");
            expr = call;
        } else if (match(p, TOK_DOT)) {
            /* Member access */
            Token *name = consume(p, TOK_IDENTIFIER, "Expected property name after '.'");
            AstNode *access = ast_new_node(NODE_MEMBER_ACCESS, expr->line, expr->column);
            access->as.member_access.object = expr;
            access->as.member_access.member = token_to_string(name);
            expr = access;
        } else if (match(p, TOK_LBRACKET)) {
            /* Index */
            AstNode *index = ast_new_node(NODE_INDEX_EXPR, expr->line, expr->column);
            index->as.index_expr.object = expr;
            index->as.index_expr.index = parse_expression(p);
            consume(p, TOK_RBRACKET, "Expected ']' after index");
            expr = index;
        } else if (match(p, TOK_INCREMENT)) {
            AstNode *inc = ast_new_node(NODE_POSTFIX_INC, expr->line, expr->column);
            inc->as.inc_dec.operand = expr;
            expr = inc;
        } else if (match(p, TOK_DECREMENT)) {
            AstNode *dec = ast_new_node(NODE_POSTFIX_DEC, expr->line, expr->column);
            dec->as.inc_dec.operand = expr;
            expr = dec;
        } else {
            break;
        }
    }
    return expr;
}

static AstNode *parse_unary(Parser *p) {
    if (match(p, TOK_MINUS) || match(p, TOK_BANG) || match(p, TOK_TILDE)) {
        Token *op = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new_node(NODE_UNARY_EXPR, op->line, op->column);
        node->as.unary.op = op->type;
        node->as.unary.operand = operand;
        return node;
    }
    if (match(p, TOK_INCREMENT)) {
        Token *op = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new_node(NODE_PREFIX_INC, op->line, op->column);
        node->as.inc_dec.operand = operand;
        return node;
    }
    if (match(p, TOK_DECREMENT)) {
        Token *op = previous(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_new_node(NODE_PREFIX_DEC, op->line, op->column);
        node->as.inc_dec.operand = operand;
        return node;
    }
    return parse_postfix(p);
}

static AstNode *parse_binary(Parser *p, int min_prec) {
    AstNode *left = parse_unary(p);

    while (!is_at_end(p)) {
        TokenType op_type = current(p)->type;
        Precedence prec = get_precedence(op_type);
        if (prec <= (Precedence)min_prec) break;

        Token *op = advance(p);

        if (op_type == TOK_PIPE_GREATER) {
            AstNode *right = parse_binary(p, prec);
            AstNode *node = ast_new_node(NODE_PIPE_EXPR, op->line, op->column);
            node->as.pipe.left = left;
            node->as.pipe.right = right;
            left = node;
        } else if (op_type == TOK_DOT_DOT) {
            AstNode *right = parse_binary(p, prec);
            AstNode *node = ast_new_node(NODE_RANGE_EXPR, op->line, op->column);
            node->as.range.start = left;
            node->as.range.end = right;
            left = node;
        } else {
            /* Right-associative for power */
            int next_prec = (op_type == TOK_POWER) ? prec - 1 : prec;
            AstNode *right = parse_binary(p, next_prec);
            AstNode *node = ast_new_node(NODE_BINARY_EXPR, op->line, op->column);
            node->as.binary.op = op_type;
            node->as.binary.left = left;
            node->as.binary.right = right;
            left = node;
        }
    }
    return left;
}

static AstNode *parse_assignment(Parser *p) {
    AstNode *expr = parse_binary(p, PREC_NONE);

    if (match(p, TOK_EQUAL)) {
        AstNode *value = parse_assignment(p);
        AstNode *node = ast_new_node(NODE_ASSIGNMENT, expr->line, expr->column);
        node->as.assignment.target = expr;
        node->as.assignment.value = value;
        return node;
    }

    /* Compound assignment */
    TokenType ct = current(p)->type;
    if (ct == TOK_PLUS_EQUAL || ct == TOK_MINUS_EQUAL || ct == TOK_STAR_EQUAL ||
        ct == TOK_SLASH_EQUAL || ct == TOK_PERCENT_EQUAL) {
        Token *op = advance(p);
        AstNode *value = parse_assignment(p);
        AstNode *node = ast_new_node(NODE_COMPOUND_ASSIGN, op->line, op->column);
        node->as.compound_assign.target = expr;
        node->as.compound_assign.value = value;
        node->as.compound_assign.op = op->type;
        return node;
    }

    return expr;
}

static AstNode *parse_expression(Parser *p) {
    return parse_assignment(p);
}

/* ===== Statement parsing ===== */
static AstNode *parse_block(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_BLOCK_STMT, tok->line, tok->column);
    ast_node_list_init(&node->as.block.statements);
    while (!check(p, TOK_RBRACE) && !is_at_end(p)) {
        AstNode *decl = parse_declaration(p);
        if (decl) ast_node_list_push(&node->as.block.statements, decl);
    }
    consume(p, TOK_RBRACE, "Expected '}' after block");
    return node;
}

static AstNode *parse_oracle_stmt(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_ORACLE_STMT, tok->line, tok->column);
    consume(p, TOK_LPAREN, "Expected '(' after 'oracle'");
    node->as.oracle.condition = parse_expression(p);
    consume(p, TOK_RPAREN, "Expected ')' after condition");
    consume(p, TOK_LBRACE, "Expected '{' after oracle condition");
    node->as.oracle.then_branch = parse_block(p);
    node->as.oracle.else_branch = NULL;
    if (match(p, TOK_OTHERWISE)) {
        if (match(p, TOK_ORACLE)) {
            node->as.oracle.else_branch = parse_oracle_stmt(p);
        } else {
            consume(p, TOK_LBRACE, "Expected '{' after 'otherwise'");
            node->as.oracle.else_branch = parse_block(p);
        }
    }
    return node;
}

static AstNode *parse_cycle_stmt(Parser *p) {
    Token *tok = previous(p);
    consume(p, TOK_LPAREN, "Expected '(' after 'cycle'");

    /* Check for cycle-in: cycle (morph x in items) */
    /* Standard C-style for: cycle (init; cond; update) */
    AstNode *node = ast_new_node(NODE_CYCLE_STMT, tok->line, tok->column);

    /* Init */
    if (match(p, TOK_SEMICOLON)) {
        node->as.cycle.init = NULL;
    } else if (check(p, TOK_MORPH)) {
        node->as.cycle.init = parse_declaration(p);
    } else {
        node->as.cycle.init = parse_expression(p);
        consume(p, TOK_SEMICOLON, "Expected ';' after loop init");
    }

    /* Condition */
    if (!check(p, TOK_SEMICOLON)) {
        node->as.cycle.condition = parse_expression(p);
    } else {
        node->as.cycle.condition = NULL;
    }
    consume(p, TOK_SEMICOLON, "Expected ';' after loop condition");

    /* Update */
    if (!check(p, TOK_RPAREN)) {
        node->as.cycle.update = parse_expression(p);
    } else {
        node->as.cycle.update = NULL;
    }
    consume(p, TOK_RPAREN, "Expected ')' after cycle clauses");

    consume(p, TOK_LBRACE, "Expected '{' after cycle");
    node->as.cycle.body = parse_block(p);
    return node;
}

static AstNode *parse_while_stmt(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_WHILE_STMT, tok->line, tok->column);
    consume(p, TOK_LPAREN, "Expected '(' after 'while'");
    node->as.while_stmt.condition = parse_expression(p);
    consume(p, TOK_RPAREN, "Expected ')' after condition");
    consume(p, TOK_LBRACE, "Expected '{' after while condition");
    node->as.while_stmt.body = parse_block(p);
    return node;
}

static AstNode *parse_shield_stmt(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_SHIELD_STMT, tok->line, tok->column);
    consume(p, TOK_LBRACE, "Expected '{' after 'shield'");
    node->as.shield.try_body = parse_block(p);
    consume(p, TOK_DEFLECT, "Expected 'deflect' after shield block");
    consume(p, TOK_LPAREN, "Expected '(' after 'deflect'");
    Token *var = consume(p, TOK_IDENTIFIER, "Expected error variable name");
    node->as.shield.catch_var = token_to_string(var);
    consume(p, TOK_RPAREN, "Expected ')' after error variable");
    consume(p, TOK_LBRACE, "Expected '{' after deflect");
    node->as.shield.catch_body = parse_block(p);
    return node;
}

static AstNode *parse_statement(Parser *p) {
    if (match(p, TOK_ORACLE))   return parse_oracle_stmt(p);
    if (match(p, TOK_CYCLE))    return parse_cycle_stmt(p);
    if (match(p, TOK_WHILE))    return parse_while_stmt(p);
    if (match(p, TOK_SHIELD))   return parse_shield_stmt(p);
    if (match(p, TOK_LBRACE))   return parse_block(p);

    if (match(p, TOK_UNLEASH)) {
        Token *tok = previous(p);
        AstNode *node = ast_new_node(NODE_UNLEASH_STMT, tok->line, tok->column);
        if (!check(p, TOK_SEMICOLON) && !check(p, TOK_RBRACE)) {
            node->as.unleash.value = parse_expression(p);
        } else {
            node->as.unleash.value = NULL;
        }
        match(p, TOK_SEMICOLON);
        return node;
    }

    if (match(p, TOK_SHATTER_CYCLE)) {
        Token *tok = previous(p);
        match(p, TOK_SEMICOLON);
        return ast_new_node(NODE_SHATTER_CYCLE_STMT, tok->line, tok->column);
    }

    if (match(p, TOK_SKIP)) {
        Token *tok = previous(p);
        match(p, TOK_SEMICOLON);
        return ast_new_node(NODE_SKIP_STMT, tok->line, tok->column);
    }

    if (match(p, TOK_SHATTER)) {
        Token *tok = previous(p);
        AstNode *node = ast_new_node(NODE_SHATTER_STMT, tok->line, tok->column);
        node->as.shatter.expr = parse_expression(p);
        match(p, TOK_SEMICOLON);
        return node;
    }

    /* Expression statement */
    AstNode *expr = parse_expression(p);
    /* Check if it was an engrave statement parsed as expression */
    if (expr->type == NODE_ENGRAVE_STMT) {
        match(p, TOK_SEMICOLON);
        return expr;
    }
    AstNode *stmt = ast_new_node(NODE_EXPR_STMT, expr->line, expr->column);
    stmt->as.expr_stmt.expr = expr;
    match(p, TOK_SEMICOLON);
    return stmt;
}

/* ===== Declaration parsing ===== */
static TypeAnnotation parse_type(Parser *p) {
    TypeAnnotation t = {0};
    Token *type_tok = advance(p);
    t.name = token_to_string(type_tok);
    if (match(p, TOK_LBRACKET)) {
        t.is_arsenal = true;
        Token *elem = advance(p);
        t.element_type = token_to_string(elem);
        consume(p, TOK_RBRACKET, "Expected ']' after element type");
    }
    return t;
}

static AstNode *parse_var_decl(Parser *p) {
    Token *tok = previous(p);
    bool is_const = (tok->type == TOK_ETERNAL);
    AstNode *node = ast_new_node(is_const ? NODE_CONST_DECL : NODE_VAR_DECL,
                                  tok->line, tok->column);
    Token *name = consume(p, TOK_IDENTIFIER, "Expected variable name");
    node->as.var_decl.name = token_to_string(name);
    node->as.var_decl.type.name = NULL;
    if (match(p, TOK_COLON)) {
        node->as.var_decl.type = parse_type(p);
    }
    if (match(p, TOK_EQUAL)) {
        node->as.var_decl.initializer = parse_expression(p);
    } else {
        node->as.var_decl.initializer = NULL;
    }
    match(p, TOK_SEMICOLON);
    return node;
}

static AstNode *parse_forge_decl(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_FORGE_DECL, tok->line, tok->column);
    Token *name = consume(p, TOK_IDENTIFIER, "Expected function name");
    node->as.forge.name = token_to_string(name);
    param_list_init(&node->as.forge.params);

    consume(p, TOK_LPAREN, "Expected '(' after function name");
    if (!check(p, TOK_RPAREN)) {
        do {
            Parameter param = {0};
            Token *pname = consume(p, TOK_IDENTIFIER, "Expected parameter name");
            param.name = token_to_string(pname);
            if (match(p, TOK_COLON)) {
                param.type = parse_type(p);
            }
            if (match(p, TOK_EQUAL)) {
                param.default_value = parse_expression(p);
            }
            param_list_push(&node->as.forge.params, param);
        } while (match(p, TOK_COMMA));
    }
    consume(p, TOK_RPAREN, "Expected ')' after parameters");

    node->as.forge.return_type.name = NULL;
    if (match(p, TOK_ARROW)) {
        node->as.forge.return_type = parse_type(p);
    }

    consume(p, TOK_LBRACE, "Expected '{' before function body");
    node->as.forge.body = parse_block(p);
    return node;
}

static AstNode *parse_entity_decl(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_ENTITY_DECL, tok->line, tok->column);
    Token *name = consume(p, TOK_IDENTIFIER, "Expected entity name");
    node->as.entity.name = token_to_string(name);
    node->as.entity.parent = NULL;

    if (match(p, TOK_COLON) || match(p, TOK_EXTENDS)) {
        Token *parent = consume(p, TOK_IDENTIFIER, "Expected parent entity name");
        node->as.entity.parent = token_to_string(parent);
    }

    consume(p, TOK_LBRACE, "Expected '{' before entity body");
    ast_node_list_init(&node->as.entity.members);
    while (!check(p, TOK_RBRACE) && !is_at_end(p)) {
        if (check(p, TOK_FORGE)) {
            advance(p);
            ast_node_list_push(&node->as.entity.members, parse_forge_decl(p));
        } else if (check(p, TOK_MORPH) || check(p, TOK_ETERNAL)) {
            advance(p);
            ast_node_list_push(&node->as.entity.members, parse_var_decl(p));
        } else {
            error_at(p, current(p), "Expected member declaration in entity");
            advance(p);
        }
    }
    consume(p, TOK_RBRACE, "Expected '}' after entity body");
    return node;
}

static AstNode *parse_realm_decl(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_REALM_DECL, tok->line, tok->column);
    Token *name = consume(p, TOK_IDENTIFIER, "Expected realm name");
    node->as.realm.name = token_to_string(name);
    ast_node_list_init(&node->as.realm.declarations);
    consume(p, TOK_LBRACE, "Expected '{' after realm name");
    while (!check(p, TOK_RBRACE) && !is_at_end(p)) {
        AstNode *decl = parse_declaration(p);
        if (decl) ast_node_list_push(&node->as.realm.declarations, decl);
    }
    consume(p, TOK_RBRACE, "Expected '}' after realm body");
    return node;
}

static AstNode *parse_summon_decl(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_SUMMON_DECL, tok->line, tok->column);
    Token *module = consume(p, TOK_IDENTIFIER, "Expected module name");
    char *path = token_to_string(module);
    /* Handle dotted paths: summon GameEngine.Scene */
    while (match(p, TOK_DOT)) {
        Token *next = consume(p, TOK_IDENTIFIER, "Expected module path component");
        char *next_s = token_to_string(next);
        size_t len = strlen(path) + 1 + strlen(next_s) + 1;
        char *new_path = malloc(len);
        snprintf(new_path, len, "%s.%s", path, next_s);
        free(path);
        free(next_s);
        path = new_path;
    }
    node->as.summon.module_path = path;
    node->as.summon.alias = NULL;
    if (match(p, TOK_AS)) {
        Token *alias = consume(p, TOK_IDENTIFIER, "Expected alias name");
        node->as.summon.alias = token_to_string(alias);
    }
    match(p, TOK_SEMICOLON);
    return node;
}

static AstNode *parse_quest_decl(Parser *p) {
    Token *tok = previous(p);
    AstNode *node = ast_new_node(NODE_QUEST_DECL, tok->line, tok->column);
    consume(p, TOK_LPAREN, "Expected '(' after 'quest'");
    consume(p, TOK_RPAREN, "Expected ')' after 'quest('");
    consume(p, TOK_LBRACE, "Expected '{' before quest body");
    node->as.quest.body = parse_block(p);
    return node;
}

static AstNode *parse_declaration(Parser *p) {
    if (p->panic_mode) synchronize(p);

    if (match(p, TOK_MORPH))    return parse_var_decl(p);
    if (match(p, TOK_ETERNAL))  return parse_var_decl(p);
    if (match(p, TOK_FORGE))    return parse_forge_decl(p);
    if (match(p, TOK_ENTITY))   return parse_entity_decl(p);
    if (match(p, TOK_REALM))    return parse_realm_decl(p);
    if (match(p, TOK_SUMMON))   return parse_summon_decl(p);
    if (match(p, TOK_QUEST))    return parse_quest_decl(p);

    if (match(p, TOK_AT_GPU)) {
        Token *tok = previous(p);
        AstNode *node = ast_new_node(NODE_GPU_BLOCK, tok->line, tok->column);
        consume(p, TOK_LBRACE, "Expected '{' after '@gpu'");
        node->as.gpu_block.body = parse_block(p);
        return node;
    }
    if (match(p, TOK_AT_AI)) {
        Token *tok = previous(p);
        AstNode *node = ast_new_node(NODE_AI_BLOCK, tok->line, tok->column);
        consume(p, TOK_LBRACE, "Expected '{' after '@ai'");
        node->as.ai_block.body = parse_block(p);
        return node;
    }

    return parse_statement(p);
}

/* ===== Public API ===== */
AstNode *parser_parse_program(Parser *p) {
    AstNode *program = ast_new_node(NODE_PROGRAM, 1, 1);
    ast_node_list_init(&program->as.program.declarations);

    while (!is_at_end(p)) {
        AstNode *decl = parse_declaration(p);
        if (decl) ast_node_list_push(&program->as.program.declarations, decl);
    }
    return program;
}

AstNode *parser_parse_expression(Parser *p) {
    return parse_expression(p);
}

bool parser_had_error(const Parser *p) { return p->had_error; }

void parser_print_errors(const Parser *p) {
    for (int i = 0; i < p->error_count; i++) {
        fprintf(stderr, "%s\n", p->errors[i]);
    }
}
