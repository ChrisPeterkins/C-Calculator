#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_TOKEN_LEN 256
#define MAX_EXPR_LEN 1024

typedef enum {
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_POWER,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_SIN,
    TOKEN_COS,
    TOKEN_TAN,
    TOKEN_SQRT,
    TOKEN_LOG,
    TOKEN_EXP,
    TOKEN_ABS,
    TOKEN_PI,
    TOKEN_E,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    double value;
    char text[MAX_TOKEN_LEN];
} Token;

typedef struct {
    const char *input;
    int position;
    Token current;
} Lexer;

void lexer_init(Lexer *lexer, const char *input) {
    lexer->input = input;
    lexer->position = 0;
    lexer->current.type = TOKEN_EOF;
}

void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->input[lexer->position] && isspace(lexer->input[lexer->position])) {
        lexer->position++;
    }
}

Token lexer_read_number(Lexer *lexer) {
    Token token;
    token.type = TOKEN_NUMBER;
    
    int start = lexer->position;
    int has_dot = 0;
    
    while (lexer->input[lexer->position]) {
        char c = lexer->input[lexer->position];
        if (isdigit(c)) {
            lexer->position++;
        } else if (c == '.' && !has_dot) {
            has_dot = 1;
            lexer->position++;
        } else {
            break;
        }
    }
    
    int len = lexer->position - start;
    if (len >= MAX_TOKEN_LEN) len = MAX_TOKEN_LEN - 1;
    strncpy(token.text, lexer->input + start, len);
    token.text[len] = '\0';
    token.value = atof(token.text);
    
    return token;
}

Token lexer_read_identifier(Lexer *lexer) {
    Token token;
    int start = lexer->position;
    
    while (lexer->input[lexer->position] && isalpha(lexer->input[lexer->position])) {
        lexer->position++;
    }
    
    int len = lexer->position - start;
    if (len >= MAX_TOKEN_LEN) len = MAX_TOKEN_LEN - 1;
    strncpy(token.text, lexer->input + start, len);
    token.text[len] = '\0';
    
    if (strcmp(token.text, "sin") == 0) token.type = TOKEN_SIN;
    else if (strcmp(token.text, "cos") == 0) token.type = TOKEN_COS;
    else if (strcmp(token.text, "tan") == 0) token.type = TOKEN_TAN;
    else if (strcmp(token.text, "sqrt") == 0) token.type = TOKEN_SQRT;
    else if (strcmp(token.text, "log") == 0) token.type = TOKEN_LOG;
    else if (strcmp(token.text, "exp") == 0) token.type = TOKEN_EXP;
    else if (strcmp(token.text, "abs") == 0) token.type = TOKEN_ABS;
    else if (strcmp(token.text, "pi") == 0) {
        token.type = TOKEN_PI;
        token.value = M_PI;
    }
    else if (strcmp(token.text, "e") == 0) {
        token.type = TOKEN_E;
        token.value = M_E;
    }
    else {
        token.type = TOKEN_ERROR;
        sprintf(token.text, "Unknown identifier: %s", token.text);
    }
    
    return token;
}

Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    Token token;
    token.value = 0;
    token.text[0] = '\0';
    
    if (!lexer->input[lexer->position]) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    char c = lexer->input[lexer->position];
    
    if (isdigit(c) || (c == '.' && isdigit(lexer->input[lexer->position + 1]))) {
        return lexer_read_number(lexer);
    }
    
    if (isalpha(c)) {
        return lexer_read_identifier(lexer);
    }
    
    lexer->position++;
    
    switch (c) {
        case '+':
            token.type = TOKEN_PLUS;
            strcpy(token.text, "+");
            break;
        case '-':
            token.type = TOKEN_MINUS;
            strcpy(token.text, "-");
            break;
        case '*':
            token.type = TOKEN_MULTIPLY;
            strcpy(token.text, "*");
            break;
        case '/':
            token.type = TOKEN_DIVIDE;
            strcpy(token.text, "/");
            break;
        case '%':
            token.type = TOKEN_MODULO;
            strcpy(token.text, "%");
            break;
        case '^':
            token.type = TOKEN_POWER;
            strcpy(token.text, "^");
            break;
        case '(':
            token.type = TOKEN_LPAREN;
            strcpy(token.text, "(");
            break;
        case ')':
            token.type = TOKEN_RPAREN;
            strcpy(token.text, ")");
            break;
        default:
            token.type = TOKEN_ERROR;
            sprintf(token.text, "Unexpected character: %c", c);
            break;
    }
    
    return token;
}

void lexer_advance(Lexer *lexer) {
    lexer->current = lexer_next_token(lexer);
}

typedef struct {
    Lexer *lexer;
    char error[256];
    int has_error;
} Parser;

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->has_error = 0;
    parser->error[0] = '\0';
    lexer_advance(lexer);
}

void parser_error(Parser *parser, const char *message) {
    parser->has_error = 1;
    strncpy(parser->error, message, 255);
    parser->error[255] = '\0';
}

double parse_expression(Parser *parser);
double parse_term(Parser *parser);
double parse_factor(Parser *parser);
double parse_power(Parser *parser);
double parse_unary(Parser *parser);
double parse_primary(Parser *parser);

double parse_expression(Parser *parser) {
    double left = parse_term(parser);
    
    while (!parser->has_error) {
        TokenType type = parser->lexer->current.type;
        if (type == TOKEN_PLUS) {
            lexer_advance(parser->lexer);
            double right = parse_term(parser);
            left = left + right;
        } else if (type == TOKEN_MINUS) {
            lexer_advance(parser->lexer);
            double right = parse_term(parser);
            left = left - right;
        } else {
            break;
        }
    }
    
    return left;
}

double parse_term(Parser *parser) {
    double left = parse_factor(parser);
    
    while (!parser->has_error) {
        TokenType type = parser->lexer->current.type;
        if (type == TOKEN_MULTIPLY) {
            lexer_advance(parser->lexer);
            double right = parse_factor(parser);
            left = left * right;
        } else if (type == TOKEN_DIVIDE) {
            lexer_advance(parser->lexer);
            double right = parse_factor(parser);
            if (right == 0) {
                parser_error(parser, "Division by zero");
                return 0;
            }
            left = left / right;
        } else if (type == TOKEN_MODULO) {
            lexer_advance(parser->lexer);
            double right = parse_factor(parser);
            if (right == 0) {
                parser_error(parser, "Modulo by zero");
                return 0;
            }
            left = fmod(left, right);
        } else {
            break;
        }
    }
    
    return left;
}

double parse_factor(Parser *parser) {
    return parse_power(parser);
}

double parse_power(Parser *parser) {
    double left = parse_unary(parser);
    
    if (!parser->has_error && parser->lexer->current.type == TOKEN_POWER) {
        lexer_advance(parser->lexer);
        double right = parse_power(parser);
        return pow(left, right);
    }
    
    return left;
}

double parse_unary(Parser *parser) {
    TokenType type = parser->lexer->current.type;
    
    if (type == TOKEN_MINUS) {
        lexer_advance(parser->lexer);
        return -parse_unary(parser);
    } else if (type == TOKEN_PLUS) {
        lexer_advance(parser->lexer);
        return parse_unary(parser);
    }
    
    if (type == TOKEN_SIN) {
        lexer_advance(parser->lexer);
        return sin(parse_primary(parser));
    } else if (type == TOKEN_COS) {
        lexer_advance(parser->lexer);
        return cos(parse_primary(parser));
    } else if (type == TOKEN_TAN) {
        lexer_advance(parser->lexer);
        return tan(parse_primary(parser));
    } else if (type == TOKEN_SQRT) {
        lexer_advance(parser->lexer);
        double value = parse_primary(parser);
        if (value < 0) {
            parser_error(parser, "Square root of negative number");
            return 0;
        }
        return sqrt(value);
    } else if (type == TOKEN_LOG) {
        lexer_advance(parser->lexer);
        double value = parse_primary(parser);
        if (value <= 0) {
            parser_error(parser, "Logarithm of non-positive number");
            return 0;
        }
        return log(value);
    } else if (type == TOKEN_EXP) {
        lexer_advance(parser->lexer);
        return exp(parse_primary(parser));
    } else if (type == TOKEN_ABS) {
        lexer_advance(parser->lexer);
        return fabs(parse_primary(parser));
    }
    
    return parse_primary(parser);
}

double parse_primary(Parser *parser) {
    Token token = parser->lexer->current;
    
    if (token.type == TOKEN_NUMBER) {
        lexer_advance(parser->lexer);
        return token.value;
    }
    
    if (token.type == TOKEN_PI) {
        lexer_advance(parser->lexer);
        return token.value;
    }
    
    if (token.type == TOKEN_E) {
        lexer_advance(parser->lexer);
        return token.value;
    }
    
    if (token.type == TOKEN_LPAREN) {
        lexer_advance(parser->lexer);
        double value = parse_expression(parser);
        
        if (parser->lexer->current.type != TOKEN_RPAREN) {
            parser_error(parser, "Expected closing parenthesis");
            return 0;
        }
        lexer_advance(parser->lexer);
        return value;
    }
    
    if (token.type == TOKEN_EOF) {
        parser_error(parser, "Unexpected end of expression");
    } else if (token.type == TOKEN_ERROR) {
        parser_error(parser, token.text);
    } else {
        char error_msg[256];
        sprintf(error_msg, "Unexpected token: %s", token.text);
        parser_error(parser, error_msg);
    }
    
    return 0;
}

double evaluate(const char *expression, int *error) {
    Lexer lexer;
    Parser parser;
    
    lexer_init(&lexer, expression);
    parser_init(&parser, &lexer);
    
    double result = parse_expression(&parser);
    
    if (!parser.has_error && parser.lexer->current.type != TOKEN_EOF) {
        parser_error(&parser, "Unexpected tokens after expression");
    }
    
    if (parser.has_error) {
        *error = 1;
        printf("Error: %s\n", parser.error);
        return 0;
    }
    
    *error = 0;
    return result;
}

void print_help() {
    printf("\n=== Calculator Help ===\n");
    printf("Basic Operations:\n");
    printf("  +  Addition\n");
    printf("  -  Subtraction\n");
    printf("  *  Multiplication\n");
    printf("  /  Division\n");
    printf("  %%  Modulo\n");
    printf("  ^  Power\n");
    printf("\nFunctions:\n");
    printf("  sin(x)   Sine\n");
    printf("  cos(x)   Cosine\n");
    printf("  tan(x)   Tangent\n");
    printf("  sqrt(x)  Square root\n");
    printf("  log(x)   Natural logarithm\n");
    printf("  exp(x)   Exponential (e^x)\n");
    printf("  abs(x)   Absolute value\n");
    printf("\nConstants:\n");
    printf("  pi       π (3.14159...)\n");
    printf("  e        Euler's number (2.71828...)\n");
    printf("\nCommands:\n");
    printf("  help     Show this help\n");
    printf("  quit     Exit calculator\n");
    printf("  exit     Exit calculator\n");
    printf("  clear    Clear screen\n");
    printf("\nExamples:\n");
    printf("  2 + 3 * 4\n");
    printf("  sin(pi/2)\n");
    printf("  sqrt(16) + log(e)\n");
    printf("  2^8\n");
    printf("  (5 + 3) * 2\n");
    printf("====================\n\n");
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

int main() {
    char input[MAX_EXPR_LEN];
    int error;
    
    printf("=== C Calculator ===\n");
    printf("Type 'help' for instructions or 'quit' to exit\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        
        if (strcmp(input, "help") == 0) {
            print_help();
            continue;
        }
        
        if (strcmp(input, "clear") == 0) {
            clear_screen();
            printf("=== C Calculator ===\n");
            printf("Type 'help' for instructions or 'quit' to exit\n\n");
            continue;
        }
        
        double result = evaluate(input, &error);
        
        if (!error) {
            printf("= %.10g\n", result);
        }
    }
    
    return 0;
}