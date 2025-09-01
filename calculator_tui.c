#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>

#define MAX_TOKEN_LEN 256
#define MAX_EXPR_LEN 1024
#define MAX_HISTORY 10

typedef enum {
    TOKEN_NUMBER, TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULTIPLY, TOKEN_DIVIDE,
    TOKEN_MODULO, TOKEN_POWER, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_SIN, TOKEN_COS, TOKEN_TAN, TOKEN_SQRT, TOKEN_LOG, TOKEN_EXP,
    TOKEN_ABS, TOKEN_PI, TOKEN_E, TOKEN_EOF, TOKEN_ERROR
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

typedef struct {
    Lexer *lexer;
    char error[256];
    int has_error;
} Parser;

typedef struct {
    char expressions[MAX_HISTORY][MAX_EXPR_LEN];
    double results[MAX_HISTORY];
    int errors[MAX_HISTORY];
    int count;
    int current;
} History;

History history = {0};
char current_expr[MAX_EXPR_LEN] = "";
int cursor_pos = 0;

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
        sprintf(token.text, "Unknown: %s", token.text);
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
        case '+': token.type = TOKEN_PLUS; strcpy(token.text, "+"); break;
        case '-': token.type = TOKEN_MINUS; strcpy(token.text, "-"); break;
        case '*': token.type = TOKEN_MULTIPLY; strcpy(token.text, "*"); break;
        case '/': token.type = TOKEN_DIVIDE; strcpy(token.text, "/"); break;
        case '%': token.type = TOKEN_MODULO; strcpy(token.text, "%"); break;
        case '^': token.type = TOKEN_POWER; strcpy(token.text, "^"); break;
        case '(': token.type = TOKEN_LPAREN; strcpy(token.text, "("); break;
        case ')': token.type = TOKEN_RPAREN; strcpy(token.text, ")"); break;
        default:
            token.type = TOKEN_ERROR;
            sprintf(token.text, "Unknown: %c", c);
            break;
    }
    return token;
}

void lexer_advance(Lexer *lexer) {
    lexer->current = lexer_next_token(lexer);
}

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
            left = left + parse_term(parser);
        } else if (type == TOKEN_MINUS) {
            lexer_advance(parser->lexer);
            left = left - parse_term(parser);
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
            left = left * parse_factor(parser);
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
        return pow(left, parse_power(parser));
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
            parser_error(parser, "Sqrt of negative");
            return 0;
        }
        return sqrt(value);
    } else if (type == TOKEN_LOG) {
        lexer_advance(parser->lexer);
        double value = parse_primary(parser);
        if (value <= 0) {
            parser_error(parser, "Log of non-positive");
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
    
    if (token.type == TOKEN_NUMBER || token.type == TOKEN_PI || token.type == TOKEN_E) {
        lexer_advance(parser->lexer);
        return token.value;
    }
    
    if (token.type == TOKEN_LPAREN) {
        lexer_advance(parser->lexer);
        double value = parse_expression(parser);
        if (parser->lexer->current.type != TOKEN_RPAREN) {
            parser_error(parser, "Missing )");
            return 0;
        }
        lexer_advance(parser->lexer);
        return value;
    }
    
    if (token.type == TOKEN_EOF) {
        parser_error(parser, "Unexpected end");
    } else if (token.type == TOKEN_ERROR) {
        parser_error(parser, token.text);
    } else {
        char error_msg[256];
        sprintf(error_msg, "Unexpected: %s", token.text);
        parser_error(parser, error_msg);
    }
    return 0;
}

double evaluate(const char *expression, char *error_msg) {
    Lexer lexer;
    Parser parser;
    
    lexer_init(&lexer, expression);
    parser_init(&parser, &lexer);
    
    double result = parse_expression(&parser);
    
    if (!parser.has_error && parser.lexer->current.type != TOKEN_EOF) {
        parser_error(&parser, "Extra tokens");
    }
    
    if (parser.has_error) {
        strcpy(error_msg, parser.error);
        return 0;
    }
    
    error_msg[0] = '\0';
    return result;
}

void clear_screen() {
    printf("\033[2J\033[H");
}

void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

void set_color(int color) {
    printf("\033[%dm", color);
}

void reset_color() {
    printf("\033[0m");
}

void draw_border(int width, int height) {
    move_cursor(1, 1);
    printf("┌");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┐");
    
    for (int i = 2; i < height; i++) {
        move_cursor(i, 1);
        printf("│");
        move_cursor(i, width);
        printf("│");
    }
    
    move_cursor(height, 1);
    printf("└");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┘");
}

void draw_ui() {
    clear_screen();
    draw_border(80, 24);
    
    move_cursor(2, 30);
    set_color(36);
    printf("═══ CALCULATOR ═══");
    reset_color();
    
    move_cursor(4, 3);
    printf("Expression: %s", current_expr);
    move_cursor(4, 15 + cursor_pos);
    
    move_cursor(6, 3);
    printf("────────────────────────────────────────────────────────────────────────");
    
    move_cursor(8, 3);
    set_color(33);
    printf("History:");
    reset_color();
    
    int start = history.count > 8 ? history.count - 8 : 0;
    for (int i = start; i < history.count; i++) {
        move_cursor(9 + i - start, 5);
        if (history.errors[i]) {
            set_color(31);
            printf("%s = Error", history.expressions[i]);
        } else {
            set_color(32);
            printf("%s = %.10g", history.expressions[i], history.results[i]);
        }
        reset_color();
    }
    
    move_cursor(19, 3);
    printf("────────────────────────────────────────────────────────────────────────");
    
    move_cursor(21, 3);
    set_color(35);
    printf("Commands:");
    reset_color();
    printf(" [Enter] Calculate  [Up/Down] History  [Esc] Clear  [Ctrl+D] Quit");
    
    move_cursor(22, 3);
    set_color(35);
    printf("Functions:");
    reset_color();
    printf(" sin() cos() tan() sqrt() log() exp() abs() pi e");
    
    move_cursor(4, 15 + cursor_pos);
    fflush(stdout);
}

void add_to_history(const char *expr, double result, int error) {
    if (history.count < MAX_HISTORY) {
        strcpy(history.expressions[history.count], expr);
        history.results[history.count] = result;
        history.errors[history.count] = error;
        history.count++;
    } else {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history.expressions[i], history.expressions[i + 1]);
            history.results[i] = history.results[i + 1];
            history.errors[i] = history.errors[i + 1];
        }
        strcpy(history.expressions[MAX_HISTORY - 1], expr);
        history.results[MAX_HISTORY - 1] = result;
        history.errors[MAX_HISTORY - 1] = error;
    }
    history.current = history.count;
}

void set_raw_mode(struct termios *orig) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, orig);
    raw = *orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    struct termios orig_termios;
    set_raw_mode(&orig_termios);
    
    draw_ui();
    
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 4) {
            break;
        } else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 0) {
                current_expr[0] = '\0';
                cursor_pos = 0;
            } else if (seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                    if (seq[1] == 'A') {
                        if (history.current > 0) {
                            history.current--;
                            strcpy(current_expr, history.expressions[history.current]);
                            cursor_pos = strlen(current_expr);
                        }
                    } else if (seq[1] == 'B') {
                        if (history.current < history.count - 1) {
                            history.current++;
                            strcpy(current_expr, history.expressions[history.current]);
                            cursor_pos = strlen(current_expr);
                        } else if (history.current == history.count - 1) {
                            history.current = history.count;
                            current_expr[0] = '\0';
                            cursor_pos = 0;
                        }
                    } else if (seq[1] == 'C') {
                        if (cursor_pos < strlen(current_expr)) cursor_pos++;
                    } else if (seq[1] == 'D') {
                        if (cursor_pos > 0) cursor_pos--;
                    }
                }
            }
        } else if (c == '\n' || c == '\r') {
            if (strlen(current_expr) > 0) {
                char error_msg[256];
                double result = evaluate(current_expr, error_msg);
                int error = (error_msg[0] != '\0');
                add_to_history(current_expr, result, error);
                current_expr[0] = '\0';
                cursor_pos = 0;
            }
        } else if (c == 127 || c == 8) {
            if (cursor_pos > 0) {
                memmove(&current_expr[cursor_pos - 1], &current_expr[cursor_pos], 
                        strlen(current_expr) - cursor_pos + 1);
                cursor_pos--;
            }
        } else if (c >= 32 && c < 127) {
            if (strlen(current_expr) < MAX_EXPR_LEN - 1) {
                memmove(&current_expr[cursor_pos + 1], &current_expr[cursor_pos], 
                        strlen(current_expr) - cursor_pos + 1);
                current_expr[cursor_pos] = c;
                cursor_pos++;
            }
        }
        
        draw_ui();
    }
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    clear_screen();
    move_cursor(1, 1);
    printf("Goodbye!\n");
    
    return 0;
}