#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

enum TokenType {
	TokenInvalid, /// starts as 0
	TokenName, /// constant value of 1.
	TokenNum,

	/// our math operators.
	TokenPlus, TokenMinus, TokenStar, TokenSlash, TokenCarot,

	/// our delimiters.
	TokenLParen, TokenRParen, TokenLBracket, TokenRBracket,
	MaxTokens,
};

static char const *token_names[MaxTokens] = {
	[TokenInvalid]  = "Invalid",
	[TokenName]     = "Name",
	[TokenNum]      = "Number",
	[TokenPlus]     = "+",
	[TokenMinus]    = "-",
	[TokenStar]     = "*",
	[TokenSlash]    = "/",
	[TokenCarot]    = "^",
	[TokenLParen]   = "(",
	[TokenRParen]   = ")",
	[TokenLBracket] = "[",
	[TokenRBracket] = "]",
};

struct Token {
	double         value;
	size_t         num_chars;
	enum TokenType type;
	unsigned       start, end;
};

struct Lexer {
	struct Token curr_token;
	char        *input;
	size_t       input_len;
	size_t       pos;
};

bool lex_decimal(struct Lexer *lexer) {
	bool has_dot = false;
	size_t old_pos = lexer->pos;
	while( lexer->input[lexer->pos] != 0 && (isalnum(lexer->input[lexer->pos]) || lexer->input[lexer->pos]=='.') ) {
		switch( lexer->input[lexer->pos] ) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9': {
				lexer->pos++;
				break;
			}
			case '.': {
				if( has_dot ) {
					// error -- extra dot in decimal literal
					return false;
				}
				lexer->pos++;
				has_dot = true;
				break;
			}
			default: {
				lexer->pos++;
				// error -- invalid decimal literal
				return false;
			}
		}
	}
	lexer->curr_token.type = TokenNum;
	lexer->curr_token.value = strtod(&lexer->input[old_pos], NULL);
	lexer->curr_token.num_chars = lexer->pos - old_pos;
	lexer->curr_token.start = old_pos;
	lexer->curr_token.end = lexer->pos;
	return true;
}

bool lex_identifier(struct Lexer *lexer) {
	size_t old_pos = lexer->pos;
	while( lexer->input[lexer->pos] != 0 && isalpha(lexer->input[lexer->pos]) ) {
		lexer->pos++;
	}
	lexer->curr_token.type = TokenName;
	lexer->curr_token.num_chars = lexer->pos - old_pos;
	lexer->curr_token.start = old_pos;
	lexer->curr_token.end = lexer->pos;
	return true;
}

bool lexer_get_token(struct Lexer *lexer) {
	lexer->curr_token = (struct Token){0};
	while( lexer->input[lexer->pos] != 0 ) {
		char const c = lexer->input[lexer->pos];
		switch( c ) {
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '\a':
			case '\v':
				++lexer->pos;
				break;
			case '+':
				lexer->curr_token.type = TokenPlus;
				++lexer->pos;
				return true;
			case '-':
				lexer->curr_token.type = TokenMinus;
				++lexer->pos;
				return true;
			case '*':
				lexer->curr_token.type = TokenStar;
				++lexer->pos;
				return true;
			case '/':
				lexer->curr_token.type = TokenSlash;
				++lexer->pos;
				return true;
			case '^':
				lexer->curr_token.type = TokenCarot;
				++lexer->pos;
				return true;
			case '(':
				lexer->curr_token.type = TokenLParen;
				++lexer->pos;
				return true;
			case ')':
				lexer->curr_token.type = TokenRParen;
				++lexer->pos;
				return true;
			case '[':
				lexer->curr_token.type = TokenLBracket;
				++lexer->pos;
				return true;
			case ']':
				lexer->curr_token.type = TokenRBracket;
				++lexer->pos;
				return true;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
				return lex_decimal(lexer);
			default: {
				if( isalpha(c) ) {
					lex_identifier(lexer);
					return true;
				}
				// error -- invalid character .
				return false;
			}
		}
	}
	return false;
}

/// Placed ABOVE all the parse_* functions.
double parse_add(struct Lexer *lexer);
double parse_mul(struct Lexer *lexer);
double parse_pow(struct Lexer *lexer);
double parse_unary(struct Lexer *lexer);
double parse_term(struct Lexer *lexer);

/// Expr = AddExpr .
double parse_expr(struct Lexer *lexer) {
	return parse_add(lexer);
}

/// AddExpr = MulExpr *( ('+' | '-') MulExpr ) .
double parse_add(struct Lexer *lexer) {
	double result = parse_mul(lexer);
	struct Token const *curr_tok = &lexer->curr_token;
	while( curr_tok->type==TokenPlus || curr_tok->type==TokenMinus ) {
		enum TokenType const tt = curr_tok->type;
		lexer_get_token(lexer);
		switch( tt ) {
			case TokenPlus:
				result += parse_mul(lexer); break;
			case TokenMinus:
				result -= parse_mul(lexer); break;
		}
	}
	return result;
}

/// MulExpr = PowExpr *( ('*' | '/') PowExpr ) .
double parse_mul(struct Lexer *lexer) {
	double result = parse_pow(lexer);
	struct Token const *curr_tok = &lexer->curr_token;
	while( curr_tok->type == TokenStar || curr_tok->type==TokenSlash ) {
		enum TokenType const tt = curr_tok->type;
		lexer_get_token(lexer);
		switch( tt ) {
			case TokenStar:
				result *= parse_pow(lexer); break;
			case TokenSlash:
				result /= parse_pow(lexer); break;
		}
	}
	return result;
}

/// PowExpr = UnaryExpr *( '^' UnaryExpr ).
double parse_pow(struct Lexer *lexer) {
	double result = parse_unary(lexer);
	struct Token const *curr_tok = &lexer->curr_token;
	while( curr_tok->type == TokenCarot ) {
		lexer_get_token(lexer);
		result = pow(result, parse_unary(lexer));
	}
	return result;
}

/// UnaryExpr = *( '-' ) TermExpr .
double parse_unary(struct Lexer *lexer) {
	if( lexer->curr_token.type==TokenMinus ) {
		lexer_get_token(lexer);
		return -parse_unary(lexer);
	}
	return parse_term(lexer);
}

/// TermExpr = number | 'e' | 'pi' | '(' Expr ')' | '[' Expr ']' .
/// number = [0-9]+ [ '.' [0-9]+ ] | [ [0-9]+ ] '.' [0-9]+ .
double parse_term(struct Lexer *lexer) {
	/// copy current token.
	struct Token const token = lexer->curr_token;
	lexer_get_token(lexer);
	switch( token.type ) {
		case TokenNum: {
			return token.value;
		}
		case TokenName: {
			bool const has_parens = lexer->curr_token.type == TokenLParen || lexer->curr_token.type == TokenLBracket;
			char const *name = &lexer->input[token.start];
			if( !strncmp(name, "pi", token.num_chars) ) {
				return M_PI;
			} else if( !strncmp(name, "e", token.num_chars) ) {
				return M_E;
			} else if( !strncmp(name, "sin", token.num_chars) ) {
				return sin(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "ln", token.num_chars) ) {
				return log(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "cos", token.num_chars) ) {
				return cos(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "tan", token.num_chars) ) {
				return tan(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "arcsin", token.num_chars) ) {
				return asin(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "arccos", token.num_chars) ) {
				return acos(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "arctan", token.num_chars) ) {
				return atan(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "log", token.num_chars) ) {
				return log10(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "lb", token.num_chars) ) {
				return log2(has_parens? parse_term(lexer) : parse_pow(lexer));
			} else if( !strncmp(name, "myfunchere", token.num_chars) ) {
				double parameter = has_parens? parse_term(lexer) : parse_pow(lexer);
				/// do something with 'parameter'.
				return parameter;
			}
		}
		case TokenLParen: case TokenLBracket: {
			enum TokenType end_type = (token.type==TokenLParen)? TokenRParen : TokenRBracket;
			double result = parse_expr(lexer);
			if( lexer->curr_token.type==end_type ) {
				lexer_get_token(lexer);
				return result;
			}
			break;
		}
	}
	return INFINITY;
}


int main(void) {
	enum{ LINE_SIZE = 2000 };
	char line[LINE_SIZE + 1] = {0};
	for(;;) { /// infinite program loop.
		puts("please enter an equation or 'q' to quit.");
		if( fgets(line, LINE_SIZE, stdin)==NULL ) {
			puts("fgets:: bad input");
			break;
		} else if( line[0]=='q' || line[0]=='Q' ) {
			puts("calculator program exiting.");
			break;
		}
		size_t const equation_len = strlen(line);
		struct Lexer lexer = {
			.input = line,
			.input_len = equation_len,
		};
		line[equation_len-1] = 0; /// remove stupid newline at end of equation input.
		lexer_get_token(&lexer);
		printf("result of equation '%s' = %f\n", line, parse_expr(&lexer));
	}
}