#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define MAX_TOKEN_LEN 256
#define MAX_EXPR_LEN 1024
#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 50
#define DISPLAY_HEIGHT 60
#define PADDING 5

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
    char label[16];
    int x, y;
    void (*action)(void);
} Button;

typedef struct {
    Display *display;
    Window window;
    GC gc;
    XFontStruct *font;
    char display_text[MAX_EXPR_LEN];
    char result_text[MAX_EXPR_LEN];
    int display_pos;
    Button *buttons;
    int button_count;
} Calculator;

Calculator calc;

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

double evaluate(const char *expression, int *error) {
    Lexer lexer;
    Parser parser;
    
    lexer_init(&lexer, expression);
    parser_init(&parser, &lexer);
    
    double result = parse_expression(&parser);
    
    if (!parser.has_error && parser.lexer->current.type != TOKEN_EOF) {
        parser_error(&parser, "Extra tokens");
    }
    
    if (parser.has_error) {
        *error = 1;
        strcpy(calc.result_text, parser.error);
        return 0;
    }
    
    *error = 0;
    return result;
}

void append_text(const char *text) {
    int len = strlen(text);
    if (calc.display_pos + len < MAX_EXPR_LEN - 1) {
        strcat(calc.display_text, text);
        calc.display_pos += len;
    }
}

void clear_display() {
    calc.display_text[0] = '\0';
    calc.result_text[0] = '\0';
    calc.display_pos = 0;
}

void backspace() {
    if (calc.display_pos > 0) {
        calc.display_text[--calc.display_pos] = '\0';
        calc.result_text[0] = '\0';
    }
}

void calculate() {
    if (strlen(calc.display_text) == 0) return;
    
    int error;
    double result = evaluate(calc.display_text, &error);
    
    if (!error) {
        sprintf(calc.result_text, "= %.10g", result);
    }
}

void btn_0() { append_text("0"); }
void btn_1() { append_text("1"); }
void btn_2() { append_text("2"); }
void btn_3() { append_text("3"); }
void btn_4() { append_text("4"); }
void btn_5() { append_text("5"); }
void btn_6() { append_text("6"); }
void btn_7() { append_text("7"); }
void btn_8() { append_text("8"); }
void btn_9() { append_text("9"); }
void btn_dot() { append_text("."); }
void btn_plus() { append_text("+"); }
void btn_minus() { append_text("-"); }
void btn_mult() { append_text("*"); }
void btn_div() { append_text("/"); }
void btn_pow() { append_text("^"); }
void btn_lparen() { append_text("("); }
void btn_rparen() { append_text(")"); }
void btn_sin() { append_text("sin("); }
void btn_cos() { append_text("cos("); }
void btn_sqrt() { append_text("sqrt("); }
void btn_pi() { append_text("pi"); }
void btn_clear() { clear_display(); }
void btn_back() { backspace(); }
void btn_equals() { calculate(); }

void init_buttons() {
    Button button_layout[] = {
        {"C", 0, 0, btn_clear}, {"(", 1, 0, btn_lparen}, {")", 2, 0, btn_rparen}, {"<-", 3, 0, btn_back}, {"^", 4, 0, btn_pow},
        {"7", 0, 1, btn_7}, {"8", 1, 1, btn_8}, {"9", 2, 1, btn_9}, {"/", 3, 1, btn_div}, {"sin", 4, 1, btn_sin},
        {"4", 0, 2, btn_4}, {"5", 1, 2, btn_5}, {"6", 2, 2, btn_6}, {"*", 3, 2, btn_mult}, {"cos", 4, 2, btn_cos},
        {"1", 0, 3, btn_1}, {"2", 1, 3, btn_2}, {"3", 2, 3, btn_3}, {"-", 3, 3, btn_minus}, {"sqrt", 4, 3, btn_sqrt},
        {"0", 0, 4, btn_0}, {".", 1, 4, btn_dot}, {"=", 2, 4, btn_equals}, {"+", 3, 4, btn_plus}, {"pi", 4, 4, btn_pi}
    };
    
    calc.button_count = sizeof(button_layout) / sizeof(Button);
    calc.buttons = malloc(calc.button_count * sizeof(Button));
    memcpy(calc.buttons, button_layout, calc.button_count * sizeof(Button));
}

void draw_button(Button *btn) {
    int x = PADDING + btn->x * (BUTTON_WIDTH + PADDING);
    int y = DISPLAY_HEIGHT + PADDING * 2 + btn->y * (BUTTON_HEIGHT + PADDING);
    
    XDrawRectangle(calc.display, calc.window, calc.gc, x, y, BUTTON_WIDTH, BUTTON_HEIGHT);
    
    int text_width = XTextWidth(calc.font, btn->label, strlen(btn->label));
    int text_x = x + (BUTTON_WIDTH - text_width) / 2;
    int text_y = y + BUTTON_HEIGHT / 2 + calc.font->ascent / 2;
    
    XDrawString(calc.display, calc.window, calc.gc, text_x, text_y, 
                btn->label, strlen(btn->label));
}

void draw_display() {
    XClearWindow(calc.display, calc.window);
    
    XDrawRectangle(calc.display, calc.window, calc.gc, 
                   PADDING, PADDING, 5 * BUTTON_WIDTH + 4 * PADDING, DISPLAY_HEIGHT);
    
    XDrawString(calc.display, calc.window, calc.gc, 
                PADDING * 2, PADDING + 20, 
                calc.display_text, strlen(calc.display_text));
    
    if (strlen(calc.result_text) > 0) {
        XDrawString(calc.display, calc.window, calc.gc, 
                    PADDING * 2, PADDING + 45, 
                    calc.result_text, strlen(calc.result_text));
    }
    
    for (int i = 0; i < calc.button_count; i++) {
        draw_button(&calc.buttons[i]);
    }
}

Button* find_button(int x, int y) {
    for (int i = 0; i < calc.button_count; i++) {
        Button *btn = &calc.buttons[i];
        int btn_x = PADDING + btn->x * (BUTTON_WIDTH + PADDING);
        int btn_y = DISPLAY_HEIGHT + PADDING * 2 + btn->y * (BUTTON_HEIGHT + PADDING);
        
        if (x >= btn_x && x <= btn_x + BUTTON_WIDTH &&
            y >= btn_y && y <= btn_y + BUTTON_HEIGHT) {
            return btn;
        }
    }
    return NULL;
}

int main() {
    calc.display = XOpenDisplay(NULL);
    if (!calc.display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    
    int screen = DefaultScreen(calc.display);
    Window root = RootWindow(calc.display, screen);
    
    int width = 5 * BUTTON_WIDTH + 6 * PADDING;
    int height = DISPLAY_HEIGHT + 5 * BUTTON_HEIGHT + 8 * PADDING;
    
    calc.window = XCreateSimpleWindow(calc.display, root, 100, 100, width, height, 1,
                                      BlackPixel(calc.display, screen),
                                      WhitePixel(calc.display, screen));
    
    XSelectInput(calc.display, calc.window, 
                 ExposureMask | ButtonPressMask | KeyPressMask);
    
    calc.gc = XCreateGC(calc.display, calc.window, 0, NULL);
    calc.font = XLoadQueryFont(calc.display, "fixed");
    if (calc.font) {
        XSetFont(calc.display, calc.gc, calc.font->fid);
    }
    
    XStoreName(calc.display, calc.window, "Calculator");
    XMapWindow(calc.display, calc.window);
    
    clear_display();
    init_buttons();
    
    XEvent event;
    while (1) {
        XNextEvent(calc.display, &event);
        
        if (event.type == Expose) {
            draw_display();
        } else if (event.type == ButtonPress) {
            Button *btn = find_button(event.xbutton.x, event.xbutton.y);
            if (btn && btn->action) {
                btn->action();
                draw_display();
            }
        } else if (event.type == KeyPress) {
            KeySym key = XLookupKeysym(&event.xkey, 0);
            if (key == XK_Return) {
                calculate();
                draw_display();
            } else if (key == XK_Escape) {
                clear_display();
                draw_display();
            } else if (key == XK_BackSpace) {
                backspace();
                draw_display();
            } else if (key == XK_q) {
                break;
            }
        }
    }
    
    free(calc.buttons);
    XFreeGC(calc.display, calc.gc);
    XDestroyWindow(calc.display, calc.window);
    XCloseDisplay(calc.display);
    
    return 0;
}