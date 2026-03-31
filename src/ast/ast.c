/*
 * X# (Xsharp) AST Implementation
 * ================================
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Node type names ===== */
static const char* node_type_names[] = {
    [NODE_PROGRAM] = "Program",
    [NODE_LITERAL_INT] = "LiteralInt",
    [NODE_LITERAL_FLOAT] = "LiteralFloat",
    [NODE_LITERAL_STRING] = "LiteralString",
    [NODE_LITERAL_BOOL] = "LiteralBool",
    [NODE_LITERAL_NULL] = "LiteralNull",
    [NODE_LITERAL_RUNE] = "LiteralRune",
    [NODE_IDENTIFIER] = "Identifier",
    [NODE_BINARY_EXPR] = "BinaryExpr",
    [NODE_UNARY_EXPR] = "UnaryExpr",
    [NODE_CALL_EXPR] = "CallExpr",
    [NODE_MEMBER_ACCESS] = "MemberAccess",
    [NODE_INDEX_EXPR] = "IndexExpr",
    [NODE_ASSIGNMENT] = "Assignment",
    [NODE_COMPOUND_ASSIGN] = "CompoundAssign",
    [NODE_PIPE_EXPR] = "PipeExpr",
    [NODE_SPELL_EXPR] = "SpellExpr",
    [NODE_CONJURE_EXPR] = "ConjureExpr",
    [NODE_ARSENAL_LITERAL] = "ArsenalLiteral",
    [NODE_INTERPOLATED_STRING] = "InterpolatedString",
    [NODE_CAST_EXPR] = "CastExpr",
    [NODE_RANGE_EXPR] = "RangeExpr",
    [NODE_SPREAD_EXPR] = "SpreadExpr",
    [NODE_TERNARY_EXPR] = "TernaryExpr",
    [NODE_SELF_EXPR] = "SelfExpr",
    [NODE_PREFIX_INC] = "PrefixInc",
    [NODE_PREFIX_DEC] = "PrefixDec",
    [NODE_POSTFIX_INC] = "PostfixInc",
    [NODE_POSTFIX_DEC] = "PostfixDec",
    [NODE_EXPR_STMT] = "ExprStmt",
    [NODE_BLOCK_STMT] = "BlockStmt",
    [NODE_VAR_DECL] = "VarDecl",
    [NODE_CONST_DECL] = "ConstDecl",
    [NODE_ORACLE_STMT] = "OracleStmt",
    [NODE_CYCLE_STMT] = "CycleStmt",
    [NODE_CYCLE_IN_STMT] = "CycleInStmt",
    [NODE_WHILE_STMT] = "WhileStmt",
    [NODE_UNLEASH_STMT] = "UnleashStmt",
    [NODE_SHATTER_CYCLE_STMT] = "ShatterCycleStmt",
    [NODE_SKIP_STMT] = "SkipStmt",
    [NODE_ENGRAVE_STMT] = "EngraveStmt",
    [NODE_SHIELD_STMT] = "ShieldStmt",
    [NODE_SHATTER_STMT] = "ShatterStmt",
    [NODE_FORGE_DECL] = "ForgeDecl",
    [NODE_ENTITY_DECL] = "EntityDecl",
    [NODE_REALM_DECL] = "RealmDecl",
    [NODE_SUMMON_DECL] = "SummonDecl",
    [NODE_QUEST_DECL] = "QuestDecl",
    [NODE_GPU_BLOCK] = "GpuBlock",
    [NODE_AI_BLOCK] = "AiBlock",
};

const char* ast_node_type_name(AstNodeType type) {
    if (type >= 0 && type < NODE_TYPE_COUNT) {
        return node_type_names[type] ? node_type_names[type] : "Unknown";
    }
    return "Unknown";
}

/* ===== AstNodeList ===== */
void ast_node_list_init(AstNodeList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_node_list_push(AstNodeList* list, AstNode* node) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity < 8 ? 8 : list->capacity * 2;
        AstNode** tmp = (AstNode**)realloc(list->items, sizeof(AstNode*) * new_cap);
        if (!tmp) {
            fprintf(stderr, "ast_node_list_push: out of memory\n");
            return;
        }
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = node;
}

void ast_node_list_free(AstNodeList* list) {
    for (int i = 0; i < list->count; i++) {
        ast_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* ===== ParameterList ===== */
void param_list_init(ParameterList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void param_list_push(ParameterList* list, Parameter param) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity < 4 ? 4 : list->capacity * 2;
        Parameter* tmp = (Parameter*)realloc(list->items, sizeof(Parameter) * new_cap);
        if (!tmp) {
            fprintf(stderr, "param_list_push: out of memory\n");
            return;
        }
        list->items = tmp;
        list->capacity = new_cap;
    }
    list->items[list->count++] = param;
}

void param_list_free(ParameterList* list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i].name);
        free(list->items[i].type.name);
        free(list->items[i].type.element_type);
        if (list->items[i].default_value) {
            ast_free(list->items[i].default_value);
        }
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* ===== Node allocation ===== */
AstNode* ast_new_node(AstNodeType type, int line, int col) {
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "ast_new_node: out of memory\n");
        return NULL;
    }
    node->type = type;
    node->line = line;
    node->column = col;
    return node;
}

/* ===== Free helpers ===== */
static void free_type_annotation(TypeAnnotation* ta) {
    free(ta->name);
    free(ta->element_type);
    ta->name = NULL;
    ta->element_type = NULL;
}

void ast_free(AstNode* node) {
    if (!node)
        return;

    switch (node->type) {
    case NODE_PROGRAM:
        ast_node_list_free(&node->as.program.declarations);
        break;

    case NODE_LITERAL_STRING:
        free(node->as.literal_string.value);
        break;

    case NODE_IDENTIFIER:
        free(node->as.identifier.name);
        break;

    case NODE_BINARY_EXPR:
        ast_free(node->as.binary.left);
        ast_free(node->as.binary.right);
        break;

    case NODE_UNARY_EXPR:
        ast_free(node->as.unary.operand);
        break;

    case NODE_CALL_EXPR:
        ast_free(node->as.call.callee);
        ast_node_list_free(&node->as.call.args);
        break;

    case NODE_MEMBER_ACCESS:
        ast_free(node->as.member_access.object);
        free(node->as.member_access.member);
        break;

    case NODE_INDEX_EXPR:
        ast_free(node->as.index_expr.object);
        ast_free(node->as.index_expr.index);
        break;

    case NODE_ASSIGNMENT:
        ast_free(node->as.assignment.target);
        ast_free(node->as.assignment.value);
        break;

    case NODE_COMPOUND_ASSIGN:
        ast_free(node->as.compound_assign.target);
        ast_free(node->as.compound_assign.value);
        break;

    case NODE_PIPE_EXPR:
        ast_free(node->as.pipe.left);
        ast_free(node->as.pipe.right);
        break;

    case NODE_SPELL_EXPR:
        param_list_free(&node->as.spell.params);
        free_type_annotation(&node->as.spell.return_type);
        ast_free(node->as.spell.body);
        break;

    case NODE_CONJURE_EXPR:
        free(node->as.conjure.entity_name);
        ast_node_list_free(&node->as.conjure.args);
        break;

    case NODE_ARSENAL_LITERAL:
        ast_node_list_free(&node->as.arsenal_literal.elements);
        break;

    case NODE_INTERPOLATED_STRING:
        ast_node_list_free(&node->as.interp_string.parts);
        break;

    case NODE_CAST_EXPR:
        ast_free(node->as.cast.expr);
        free_type_annotation(&node->as.cast.target_type);
        break;

    case NODE_RANGE_EXPR:
        ast_free(node->as.range.start);
        ast_free(node->as.range.end);
        break;

    case NODE_SPREAD_EXPR:
        ast_free(node->as.spread.expr);
        break;

    case NODE_TERNARY_EXPR:
        ast_free(node->as.ternary.condition);
        ast_free(node->as.ternary.then_expr);
        ast_free(node->as.ternary.else_expr);
        break;

    case NODE_PREFIX_INC:
    case NODE_PREFIX_DEC:
    case NODE_POSTFIX_INC:
    case NODE_POSTFIX_DEC:
        ast_free(node->as.inc_dec.operand);
        break;

    case NODE_EXPR_STMT:
        ast_free(node->as.expr_stmt.expr);
        break;

    case NODE_BLOCK_STMT:
        ast_node_list_free(&node->as.block.statements);
        break;

    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
        free(node->as.var_decl.name);
        free_type_annotation(&node->as.var_decl.type);
        ast_free(node->as.var_decl.initializer);
        break;

    case NODE_ORACLE_STMT:
        ast_free(node->as.oracle.condition);
        ast_free(node->as.oracle.then_branch);
        ast_free(node->as.oracle.else_branch);
        break;

    case NODE_CYCLE_STMT:
        ast_free(node->as.cycle.init);
        ast_free(node->as.cycle.condition);
        ast_free(node->as.cycle.update);
        ast_free(node->as.cycle.body);
        break;

    case NODE_CYCLE_IN_STMT:
        free(node->as.cycle_in.var_name);
        ast_free(node->as.cycle_in.iterable);
        ast_free(node->as.cycle_in.body);
        break;

    case NODE_WHILE_STMT:
        ast_free(node->as.while_stmt.condition);
        ast_free(node->as.while_stmt.body);
        break;

    case NODE_UNLEASH_STMT:
        ast_free(node->as.unleash.value);
        break;

    case NODE_ENGRAVE_STMT:
        ast_node_list_free(&node->as.engrave.args);
        break;

    case NODE_SHIELD_STMT:
        ast_free(node->as.shield.try_body);
        free(node->as.shield.catch_var);
        ast_free(node->as.shield.catch_body);
        break;

    case NODE_SHATTER_STMT:
        ast_free(node->as.shatter.expr);
        break;

    case NODE_FORGE_DECL:
        free(node->as.forge.name);
        param_list_free(&node->as.forge.params);
        free_type_annotation(&node->as.forge.return_type);
        ast_free(node->as.forge.body);
        break;

    case NODE_ENTITY_DECL:
        free(node->as.entity.name);
        free(node->as.entity.parent);
        ast_node_list_free(&node->as.entity.members);
        break;

    case NODE_REALM_DECL:
        free(node->as.realm.name);
        ast_node_list_free(&node->as.realm.declarations);
        break;

    case NODE_SUMMON_DECL:
        free(node->as.summon.module_path);
        free(node->as.summon.alias);
        break;

    case NODE_QUEST_DECL:
        ast_free(node->as.quest.body);
        break;

    case NODE_GPU_BLOCK:
        ast_free(node->as.gpu_block.body);
        break;

    case NODE_AI_BLOCK:
        ast_free(node->as.ai_block.body);
        break;

    default:
        break;
    }

    free(node);
}

/* ===== Printing helpers ===== */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_type_annotation(const TypeAnnotation* ta) {
    if (ta->name) {
        if (ta->is_arsenal && ta->element_type) {
            printf("arsenal<%s>", ta->element_type);
        } else {
            printf("%s", ta->name);
        }
    } else {
        printf("(untyped)");
    }
}

void ast_print(const AstNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    print_indent(indent);
    printf("%s", ast_node_type_name(node->type));

    switch (node->type) {
    case NODE_PROGRAM:
        printf(" (%d declarations)\n", node->as.program.declarations.count);
        for (int i = 0; i < node->as.program.declarations.count; i++) {
            ast_print(node->as.program.declarations.items[i], indent + 1);
        }
        break;

    case NODE_LITERAL_INT:
        printf(" %lld\n", (long long)node->as.literal_int.value);
        break;

    case NODE_LITERAL_FLOAT:
        printf(" %g\n", node->as.literal_float.value);
        break;

    case NODE_LITERAL_STRING:
        printf(" \"%s\"\n", node->as.literal_string.value);
        break;

    case NODE_LITERAL_BOOL:
        printf(" %s\n", node->as.literal_bool.value ? "truth" : "lies");
        break;

    case NODE_LITERAL_NULL:
        printf(" abyss\n");
        break;

    case NODE_LITERAL_RUNE:
        printf(" '%c'\n", node->as.literal_rune.value);
        break;

    case NODE_IDENTIFIER:
        printf(" %s\n", node->as.identifier.name);
        break;

    case NODE_BINARY_EXPR:
        printf(" (op=%d)\n", node->as.binary.op);
        ast_print(node->as.binary.left, indent + 1);
        ast_print(node->as.binary.right, indent + 1);
        break;

    case NODE_UNARY_EXPR:
        printf(" (op=%d)\n", node->as.unary.op);
        ast_print(node->as.unary.operand, indent + 1);
        break;

    case NODE_CALL_EXPR:
        printf(" (%d args)\n", node->as.call.args.count);
        print_indent(indent + 1);
        printf("callee:\n");
        ast_print(node->as.call.callee, indent + 2);
        for (int i = 0; i < node->as.call.args.count; i++) {
            print_indent(indent + 1);
            printf("arg[%d]:\n", i);
            ast_print(node->as.call.args.items[i], indent + 2);
        }
        break;

    case NODE_MEMBER_ACCESS:
        printf(" .%s\n", node->as.member_access.member);
        ast_print(node->as.member_access.object, indent + 1);
        break;

    case NODE_INDEX_EXPR:
        printf("\n");
        print_indent(indent + 1);
        printf("object:\n");
        ast_print(node->as.index_expr.object, indent + 2);
        print_indent(indent + 1);
        printf("index:\n");
        ast_print(node->as.index_expr.index, indent + 2);
        break;

    case NODE_ASSIGNMENT:
        printf("\n");
        print_indent(indent + 1);
        printf("target:\n");
        ast_print(node->as.assignment.target, indent + 2);
        print_indent(indent + 1);
        printf("value:\n");
        ast_print(node->as.assignment.value, indent + 2);
        break;

    case NODE_COMPOUND_ASSIGN:
        printf(" (op=%d)\n", node->as.compound_assign.op);
        ast_print(node->as.compound_assign.target, indent + 1);
        ast_print(node->as.compound_assign.value, indent + 1);
        break;

    case NODE_PIPE_EXPR:
        printf("\n");
        ast_print(node->as.pipe.left, indent + 1);
        ast_print(node->as.pipe.right, indent + 1);
        break;

    case NODE_SPELL_EXPR:
        printf(" (%d params) -> ", node->as.spell.params.count);
        print_type_annotation(&node->as.spell.return_type);
        printf("\n");
        ast_print(node->as.spell.body, indent + 1);
        break;

    case NODE_CONJURE_EXPR:
        printf(" %s (%d args)\n", node->as.conjure.entity_name, node->as.conjure.args.count);
        for (int i = 0; i < node->as.conjure.args.count; i++) {
            ast_print(node->as.conjure.args.items[i], indent + 1);
        }
        break;

    case NODE_ARSENAL_LITERAL:
        printf(" [%d elements]\n", node->as.arsenal_literal.elements.count);
        for (int i = 0; i < node->as.arsenal_literal.elements.count; i++) {
            ast_print(node->as.arsenal_literal.elements.items[i], indent + 1);
        }
        break;

    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
        printf(" %s : ", node->as.var_decl.name);
        print_type_annotation(&node->as.var_decl.type);
        printf("\n");
        if (node->as.var_decl.initializer) {
            print_indent(indent + 1);
            printf("init:\n");
            ast_print(node->as.var_decl.initializer, indent + 2);
        }
        break;

    case NODE_ORACLE_STMT:
        printf("\n");
        print_indent(indent + 1);
        printf("condition:\n");
        ast_print(node->as.oracle.condition, indent + 2);
        print_indent(indent + 1);
        printf("then:\n");
        ast_print(node->as.oracle.then_branch, indent + 2);
        if (node->as.oracle.else_branch) {
            print_indent(indent + 1);
            printf("else:\n");
            ast_print(node->as.oracle.else_branch, indent + 2);
        }
        break;

    case NODE_CYCLE_STMT:
        printf("\n");
        if (node->as.cycle.init) {
            print_indent(indent + 1);
            printf("init:\n");
            ast_print(node->as.cycle.init, indent + 2);
        }
        if (node->as.cycle.condition) {
            print_indent(indent + 1);
            printf("cond:\n");
            ast_print(node->as.cycle.condition, indent + 2);
        }
        if (node->as.cycle.update) {
            print_indent(indent + 1);
            printf("update:\n");
            ast_print(node->as.cycle.update, indent + 2);
        }
        print_indent(indent + 1);
        printf("body:\n");
        ast_print(node->as.cycle.body, indent + 2);
        break;

    case NODE_CYCLE_IN_STMT:
        printf(" %s in\n", node->as.cycle_in.var_name);
        print_indent(indent + 1);
        printf("iterable:\n");
        ast_print(node->as.cycle_in.iterable, indent + 2);
        print_indent(indent + 1);
        printf("body:\n");
        ast_print(node->as.cycle_in.body, indent + 2);
        break;

    case NODE_WHILE_STMT:
        printf("\n");
        print_indent(indent + 1);
        printf("condition:\n");
        ast_print(node->as.while_stmt.condition, indent + 2);
        print_indent(indent + 1);
        printf("body:\n");
        ast_print(node->as.while_stmt.body, indent + 2);
        break;

    case NODE_UNLEASH_STMT:
        printf("\n");
        if (node->as.unleash.value)
            ast_print(node->as.unleash.value, indent + 1);
        break;

    case NODE_ENGRAVE_STMT:
        printf(" (%d args)\n", node->as.engrave.args.count);
        for (int i = 0; i < node->as.engrave.args.count; i++) {
            ast_print(node->as.engrave.args.items[i], indent + 1);
        }
        break;

    case NODE_SHIELD_STMT:
        printf("\n");
        print_indent(indent + 1);
        printf("try:\n");
        ast_print(node->as.shield.try_body, indent + 2);
        print_indent(indent + 1);
        printf("catch(%s):\n", node->as.shield.catch_var ? node->as.shield.catch_var : "");
        ast_print(node->as.shield.catch_body, indent + 2);
        break;

    case NODE_SHATTER_STMT:
        printf("\n");
        ast_print(node->as.shatter.expr, indent + 1);
        break;

    case NODE_FORGE_DECL:
        printf(" %s (%d params) -> ", node->as.forge.name, node->as.forge.params.count);
        print_type_annotation(&node->as.forge.return_type);
        printf("\n");
        for (int i = 0; i < node->as.forge.params.count; i++) {
            print_indent(indent + 1);
            printf("param: %s : ", node->as.forge.params.items[i].name);
            print_type_annotation(&node->as.forge.params.items[i].type);
            printf("\n");
        }
        if (node->as.forge.body) {
            print_indent(indent + 1);
            printf("body:\n");
            ast_print(node->as.forge.body, indent + 2);
        }
        break;

    case NODE_ENTITY_DECL:
        printf(" %s", node->as.entity.name);
        if (node->as.entity.parent) {
            printf(" extends %s", node->as.entity.parent);
        }
        printf("\n");
        for (int i = 0; i < node->as.entity.members.count; i++) {
            ast_print(node->as.entity.members.items[i], indent + 1);
        }
        break;

    case NODE_REALM_DECL:
        printf(" %s\n", node->as.realm.name);
        for (int i = 0; i < node->as.realm.declarations.count; i++) {
            ast_print(node->as.realm.declarations.items[i], indent + 1);
        }
        break;

    case NODE_SUMMON_DECL:
        printf(" %s", node->as.summon.module_path);
        if (node->as.summon.alias)
            printf(" as %s", node->as.summon.alias);
        printf("\n");
        break;

    case NODE_QUEST_DECL:
        printf("\n");
        ast_print(node->as.quest.body, indent + 1);
        break;

    case NODE_GPU_BLOCK:
        printf("\n");
        ast_print(node->as.gpu_block.body, indent + 1);
        break;

    case NODE_AI_BLOCK:
        printf("\n");
        ast_print(node->as.ai_block.body, indent + 1);
        break;

    case NODE_BLOCK_STMT:
        printf(" (%d stmts)\n", node->as.block.statements.count);
        for (int i = 0; i < node->as.block.statements.count; i++) {
            ast_print(node->as.block.statements.items[i], indent + 1);
        }
        break;

    case NODE_EXPR_STMT:
        printf("\n");
        ast_print(node->as.expr_stmt.expr, indent + 1);
        break;

    case NODE_SELF_EXPR:
        printf("\n");
        break;

    case NODE_SHATTER_CYCLE_STMT:
    case NODE_SKIP_STMT:
        printf("\n");
        break;

    default:
        printf(" (unhandled print)\n");
        break;
    }
}
