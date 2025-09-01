#import <Cocoa/Cocoa.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 256
#define MAX_EXPR_LEN 1024

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
    double left = parse_power(parser);
    
    // Check for implicit multiplication patterns
    // Examples: 2pi, 2sin(x), 2(3+4), (2)(3)
    while (!parser->has_error) {
        TokenType next = parser->lexer->current.type;
        
        // Number or closing paren followed by: constant, function, or opening paren
        if (next == TOKEN_PI || next == TOKEN_E || 
            next == TOKEN_LPAREN ||
            next == TOKEN_SIN || next == TOKEN_COS || next == TOKEN_TAN ||
            next == TOKEN_SQRT || next == TOKEN_LOG || next == TOKEN_EXP ||
            next == TOKEN_ABS || next == TOKEN_NUMBER) {
            
            // Implicitly multiply by the next factor
            double right = parse_power(parser);
            left = left * right;
        } else {
            break;
        }
    }
    
    return left;
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

@interface CalculatorView : NSView
@property (nonatomic, strong) NSTextField *display;
@property (nonatomic, strong) NSTextField *result;
@property (nonatomic, strong) NSMutableString *expression;
@end

@implementation CalculatorView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.expression = [NSMutableString string];
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    self.display = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 320, 280, 30)];
    [self.display setEditable:NO];
    [self.display setBezeled:YES];
    [self.display setBackgroundColor:[NSColor controlBackgroundColor]];
    [self.display setFont:[NSFont systemFontOfSize:16]];
    [self addSubview:self.display];
    
    self.result = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 285, 280, 25)];
    [self.result setEditable:NO];
    [self.result setBezeled:NO];
    [self.result setDrawsBackground:NO];
    [self.result setTextColor:[NSColor systemBlueColor]];
    [self.result setFont:[NSFont systemFontOfSize:14]];
    [self addSubview:self.result];
    
    NSArray *buttons = @[
        @[@"C", @"(", @")", @"⌫", @"^"],
        @[@"7", @"8", @"9", @"/", @"sin"],
        @[@"4", @"5", @"6", @"*", @"cos"],
        @[@"1", @"2", @"3", @"-", @"√"],
        @[@"0", @".", @"=", @"+", @"π"]
    ];
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            NSButton *button = [[NSButton alloc] initWithFrame:NSMakeRect(10 + col * 56, 220 - row * 50, 54, 48)];
            [button setTitle:buttons[row][col]];
            [button setTarget:self];
            [button setAction:@selector(buttonPressed:)];
            [button setBezelStyle:NSBezelStyleRegularSquare];
            [self addSubview:button];
        }
    }
}

- (void)buttonPressed:(NSButton *)sender {
    NSString *title = [sender title];
    
    if ([title isEqualToString:@"C"]) {
        [self.expression setString:@""];
        [self.result setStringValue:@""];
    } else if ([title isEqualToString:@"⌫"]) {
        if ([self.expression length] > 0) {
            [self.expression deleteCharactersInRange:NSMakeRange([self.expression length] - 1, 1)];
        }
    } else if ([title isEqualToString:@"="]) {
        [self calculate];
    } else if ([title isEqualToString:@"sin"]) {
        [self.expression appendString:@"sin("];
    } else if ([title isEqualToString:@"cos"]) {
        [self.expression appendString:@"cos("];
    } else if ([title isEqualToString:@"√"]) {
        [self.expression appendString:@"sqrt("];
    } else if ([title isEqualToString:@"π"]) {
        [self.expression appendString:@"pi"];
    } else {
        [self.expression appendString:title];
    }
    
    [self.display setStringValue:self.expression];
}

- (void)calculate {
    char error_msg[256];
    double result = evaluate([self.expression UTF8String], error_msg);
    
    if (error_msg[0] == '\0') {
        [self.result setStringValue:[NSString stringWithFormat:@"= %.10g", result]];
        [self.result setTextColor:[NSColor systemGreenColor]];
    } else {
        [self.result setStringValue:[NSString stringWithUTF8String:error_msg]];
        [self.result setTextColor:[NSColor systemRedColor]];
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    NSString *chars = [event characters];
    unichar c = [chars length] > 0 ? [chars characterAtIndex:0] : 0;
    
    if (c == '\r' || c == '\n') {
        [self calculate];
    } else if (c == 27) {
        [self.expression setString:@""];
        [self.result setStringValue:@""];
        [self.display setStringValue:@""];
    } else if (c == 127 || c == 8) {
        if ([self.expression length] > 0) {
            [self.expression deleteCharactersInRange:NSMakeRange([self.expression length] - 1, 1)];
            [self.display setStringValue:self.expression];
        }
    } else if ((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' || 
               c == '*' || c == '/' || c == '(' || c == ')' || c == '^') {
        [self.expression appendFormat:@"%c", c];
        [self.display setStringValue:self.expression];
    }
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(100, 100, 300, 360);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                               styleMask:NSWindowStyleMaskTitled |
                                                        NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskMiniaturizable
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    
    [self.window setTitle:@"Calculator"];
    
    CalculatorView *calcView = [[CalculatorView alloc] initWithFrame:NSMakeRect(0, 0, 300, 360)];
    [self.window setContentView:calcView];
    
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:calcView];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}