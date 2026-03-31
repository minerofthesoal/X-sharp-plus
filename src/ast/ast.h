/*
 * X# (Xsharp) AST - Abstract Syntax Tree Node Definitions
 * =========================================================
 */

#ifndef XSHARP_AST_H
#define XSHARP_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== AST Node Types ===== */
typedef enum {
    /* --- Program --- */
    NODE_PROGRAM,

    /* --- Expressions --- */
    NODE_LITERAL_INT,
    NODE_LITERAL_FLOAT,
    NODE_LITERAL_STRING,
    NODE_LITERAL_BOOL,
    NODE_LITERAL_NULL,
    NODE_LITERAL_RUNE,
    NODE_IDENTIFIER,
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
    NODE_CALL_EXPR,
    NODE_MEMBER_ACCESS,
    NODE_INDEX_EXPR,
    NODE_ASSIGNMENT,
    NODE_COMPOUND_ASSIGN,
    NODE_PIPE_EXPR,
    NODE_SPELL_EXPR,      /* lambda / spell */
    NODE_CONJURE_EXPR,    /* new / conjure */
    NODE_ARSENAL_LITERAL, /* array literal */
    NODE_INTERPOLATED_STRING,
    NODE_CAST_EXPR,    /* as casting */
    NODE_RANGE_EXPR,   /* .. range */
    NODE_SPREAD_EXPR,  /* ... spread */
    NODE_TERNARY_EXPR, /* ? : */
    NODE_SELF_EXPR,    /* self */
    NODE_PREFIX_INC,   /* ++x */
    NODE_PREFIX_DEC,   /* --x */
    NODE_POSTFIX_INC,  /* x++ */
    NODE_POSTFIX_DEC,  /* x-- */

    /* --- Statements --- */
    NODE_EXPR_STMT,
    NODE_BLOCK_STMT,
    NODE_VAR_DECL,
    NODE_CONST_DECL,
    NODE_ORACLE_STMT,   /* if */
    NODE_CYCLE_STMT,    /* for */
    NODE_CYCLE_IN_STMT, /* for-in */
    NODE_WHILE_STMT,
    NODE_UNLEASH_STMT,       /* return */
    NODE_SHATTER_CYCLE_STMT, /* break */
    NODE_SKIP_STMT,          /* continue */
    NODE_ENGRAVE_STMT,       /* print */
    NODE_SHIELD_STMT,        /* try/catch */
    NODE_SHATTER_STMT,       /* throw */

    /* --- Declarations --- */
    NODE_FORGE_DECL,  /* function */
    NODE_ENTITY_DECL, /* class */
    NODE_REALM_DECL,  /* namespace */
    NODE_SUMMON_DECL, /* import */
    NODE_QUEST_DECL,  /* main */

    /* --- Special Blocks --- */
    NODE_GPU_BLOCK,
    NODE_AI_BLOCK,

    NODE_TYPE_COUNT
} AstNodeType;

/* ===== Forward declarations ===== */
typedef struct AstNode AstNode;

/* ===== Array of AstNode pointers ===== */
typedef struct {
    AstNode** items;
    int count;
    int capacity;
} AstNodeList;

/* ===== Type annotation ===== */
typedef struct {
    char* name;         /* "blade", "spark", "scroll", etc. */
    bool is_arsenal;    /* arsenal<T> */
    char* element_type; /* for arsenal<blade> this would be "blade" */
} TypeAnnotation;

/* ===== Parameter ===== */
typedef struct {
    char* name;
    TypeAnnotation type;
    AstNode* default_value; /* optional */
} Parameter;

typedef struct {
    Parameter* items;
    int count;
    int capacity;
} ParameterList;

/* ===== AST Node Structure ===== */
struct AstNode {
    AstNodeType type;
    int line;
    int column;

    union {
        /* NODE_LITERAL_INT */
        struct {
            int64_t value;
        } literal_int;

        /* NODE_LITERAL_FLOAT */
        struct {
            double value;
        } literal_float;

        /* NODE_LITERAL_STRING */
        struct {
            char* value;
        } literal_string;

        /* NODE_LITERAL_BOOL */
        struct {
            bool value;
        } literal_bool;

        /* NODE_LITERAL_RUNE */
        struct {
            char value;
        } literal_rune;

        /* NODE_IDENTIFIER */
        struct {
            char* name;
        } identifier;

        /* NODE_BINARY_EXPR */
        struct {
            AstNode* left;
            AstNode* right;
            int op; /* TokenType of operator */
        } binary;

        /* NODE_UNARY_EXPR */
        struct {
            AstNode* operand;
            int op;
        } unary;

        /* NODE_CALL_EXPR */
        struct {
            AstNode* callee;
            AstNodeList args;
        } call;

        /* NODE_MEMBER_ACCESS */
        struct {
            AstNode* object;
            char* member;
        } member_access;

        /* NODE_INDEX_EXPR */
        struct {
            AstNode* object;
            AstNode* index;
        } index_expr;

        /* NODE_ASSIGNMENT */
        struct {
            AstNode* target;
            AstNode* value;
        } assignment;

        /* NODE_COMPOUND_ASSIGN */
        struct {
            AstNode* target;
            AstNode* value;
            int op; /* += -= *= etc. */
        } compound_assign;

        /* NODE_PIPE_EXPR */
        struct {
            AstNode* left;
            AstNode* right;
        } pipe;

        /* NODE_SPELL_EXPR (lambda) */
        struct {
            ParameterList params;
            TypeAnnotation return_type;
            AstNode* body;
        } spell;

        /* NODE_CONJURE_EXPR (new) */
        struct {
            char* entity_name;
            AstNodeList args;
        } conjure;

        /* NODE_ARSENAL_LITERAL */
        struct {
            AstNodeList elements;
        } arsenal_literal;

        /* NODE_INTERPOLATED_STRING */
        struct {
            AstNodeList parts; /* alternating string segments and expressions */
        } interp_string;

        /* NODE_CAST_EXPR */
        struct {
            AstNode* expr;
            TypeAnnotation target_type;
        } cast;

        /* NODE_RANGE_EXPR */
        struct {
            AstNode* start;
            AstNode* end;
        } range;

        /* NODE_SPREAD_EXPR */
        struct {
            AstNode* expr;
        } spread;

        /* NODE_TERNARY_EXPR */
        struct {
            AstNode* condition;
            AstNode* then_expr;
            AstNode* else_expr;
        } ternary;

        /* NODE_PREFIX_INC, NODE_PREFIX_DEC, NODE_POSTFIX_INC, NODE_POSTFIX_DEC */
        struct {
            AstNode* operand;
        } inc_dec;

        /* NODE_EXPR_STMT */
        struct {
            AstNode* expr;
        } expr_stmt;

        /* NODE_BLOCK_STMT */
        struct {
            AstNodeList statements;
        } block;

        /* NODE_VAR_DECL, NODE_CONST_DECL */
        struct {
            char* name;
            TypeAnnotation type;
            AstNode* initializer; /* may be NULL */
        } var_decl;

        /* NODE_ORACLE_STMT (if/else) */
        struct {
            AstNode* condition;
            AstNode* then_branch;
            AstNode* else_branch; /* may be NULL */
        } oracle;

        /* NODE_CYCLE_STMT (for) */
        struct {
            AstNode* init;
            AstNode* condition;
            AstNode* update;
            AstNode* body;
        } cycle;

        /* NODE_CYCLE_IN_STMT (for-in) */
        struct {
            char* var_name;
            AstNode* iterable;
            AstNode* body;
        } cycle_in;

        /* NODE_WHILE_STMT */
        struct {
            AstNode* condition;
            AstNode* body;
        } while_stmt;

        /* NODE_UNLEASH_STMT (return) */
        struct {
            AstNode* value; /* may be NULL */
        } unleash;

        /* NODE_ENGRAVE_STMT (print) */
        struct {
            AstNodeList args;
        } engrave;

        /* NODE_SHIELD_STMT (try/catch) */
        struct {
            AstNode* try_body;
            char* catch_var;
            AstNode* catch_body;
        } shield;

        /* NODE_SHATTER_STMT (throw) */
        struct {
            AstNode* expr;
        } shatter;

        /* NODE_FORGE_DECL (function) */
        struct {
            char* name;
            ParameterList params;
            TypeAnnotation return_type;
            AstNode* body;
        } forge;

        /* NODE_ENTITY_DECL (class) */
        struct {
            char* name;
            char* parent;        /* NULL if no inheritance */
            AstNodeList members; /* forge decls, var decls */
        } entity;

        /* NODE_REALM_DECL (namespace) */
        struct {
            char* name;
            AstNodeList declarations;
        } realm;

        /* NODE_SUMMON_DECL (import) */
        struct {
            char* module_path;
            char* alias; /* NULL if no alias */
        } summon;

        /* NODE_QUEST_DECL (main) */
        struct {
            AstNode* body;
        } quest;

        /* NODE_GPU_BLOCK */
        struct {
            AstNode* body;
        } gpu_block;

        /* NODE_AI_BLOCK */
        struct {
            AstNode* body;
        } ai_block;

        /* NODE_PROGRAM */
        struct {
            AstNodeList declarations;
        } program;
    } as;
};

/* ===== AstNodeList operations ===== */
void ast_node_list_init(AstNodeList* list);
void ast_node_list_push(AstNodeList* list, AstNode* node);
void ast_node_list_free(AstNodeList* list);

/* ===== ParameterList operations ===== */
void param_list_init(ParameterList* list);
void param_list_push(ParameterList* list, Parameter param);
void param_list_free(ParameterList* list);

/* ===== Node constructors ===== */
AstNode* ast_new_node(AstNodeType type, int line, int col);

/* ===== Free entire AST ===== */
void ast_free(AstNode* node);

/* ===== Debugging: print AST tree ===== */
void ast_print(const AstNode* node, int indent);

/* ===== Node type name ===== */
const char* ast_node_type_name(AstNodeType type);

#endif /* XSHARP_AST_H */
