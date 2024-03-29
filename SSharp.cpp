#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <memory>

using namespace std;

struct Token
{
	uint64_t value;
	string strValue;
	enum Type
	{
		Prefix,		/* ~ */

		Mult,		/* * */
		Div,
		Mod,

		Plus,		/* + */
		Minus,

		Less,		/* < */
		Greater,	/* > */	
		Equality,	/* == */	
		Inequality,	/* != */

		And,		/* && */
		Or,			/* || */

		LeftParen,	/* ( */
		RightParen,	/* ) */
		LeftBrace,	/* { */
		RightBrace,	/* } */
		Comma,		/* , */
		Semicolon,	/* ; */

		If,			/* if */

		Number,		/* uint64_t */
		Variable,	/* var */
		Assign,		/* = */

		Error		/* error */
	} type;

	Token(uint64_t val, Type t) : value(val), type(t) {}
	Token(const string & val) : strValue(val), type(Type::Variable) {}

};

bool isLetter(const string & str) {
	for (auto &&c : str) 
		if ((c < 'a' || c > 'z'))
			return false;

	return true;
}

bool isNumber(const string & str) {
	for (auto &&c : str) {
		if (c < '0' || c > '9')
			return false;
	}
	return true;
}


void check(string& s, vector<Token>& tokens)
{
	if (s.empty()) {
		return;
	}

#define _tok(expr, res) else if (s == expr) { tokens.emplace_back(0, res); }
	_tok("=", Token::Assign)
	_tok("==", Token::Equality)
	_tok("!=", Token::Inequality)
	_tok("&&", Token::And)
	_tok("||", Token::Or)
	_tok("if", Token::If)

#undef _tok
	else if (isLetter(s)) {
			tokens.emplace_back(s);		/* var */
	}
	else {
		stringstream ss(s);
		uint64_t i;
		ss >> i;
		if (ss.fail())
		{
			tokens.clear();
			tokens.emplace_back(0, Token::Error);
		}
		tokens.emplace_back(i, Token::Number);
	}
	s.clear();
}

vector<Token> Tokenize(istream& in)
{
	char c;
	string s;
	vector<Token> tokens;
	while (in.get(c))
	{
		if (isspace(c))
		{
			check(s, tokens);
		}
		else if (c == '=' || c == '!' || c == '&' || c == '|')
		{
			if (s[0] != '=' && s[0] != '!' && s[0] != '&' && s[0] != '|')
				check(s, tokens);
			s.push_back(c);
			if (s == "==" || s == "!=" || s == "&&" || s == "||")
				check(s, tokens);
		}

#define tok(expr, res) else if (c == expr) { check(s, tokens); tokens.emplace_back(0, res); }

							tok('+', Token::Plus)
							tok('-', Token::Minus)
							tok('*', Token::Mult)
							tok('/', Token::Div)
							tok('%', Token::Mod)
							tok('>', Token::Greater)
							tok('<', Token::Less)
							tok('~', Token::Prefix)
							tok('(', Token::LeftParen)
							tok(')', Token::RightParen)
							tok(';', Token::Semicolon)
							tok(',', Token::Comma)
							tok('{', Token::LeftBrace)
							tok('}', Token::RightBrace)
		

#undef tok
else if (!isLetter(s) && !isNumber(s))
		{
			check(s, tokens);
			s.push_back(c);
		}
else
		{
			s.push_back(c);
		}
	}
	check(s, tokens);
	return tokens;

}


static stringstream out;

using Scope = map<string, uint64_t>;

// Expression
class Expression
{
public:
	virtual void eval(Scope & s) = 0;
	virtual ~Expression() {}
};

// Number
class NumExpr : public Expression
{
private:
	uint64_t value;
public:
	explicit NumExpr(uint64_t value) :
		value(value)
	{}
	virtual void eval(Scope & s) {
		out << value;
	}
};

// Zkraceni kodu (binarni operatory)
#define Binary_operator_expr(name, oper)								\
class name : public Expression {										\
	unique_ptr<Expression> l, r;										\
public:																	\
	name(unique_ptr<Expression> &&l, unique_ptr<Expression> &&r) :		\
		l(move(l)), r(move(r))											\
	{}																	\
	virtual void eval(Scope &s) {										\
		l->eval(s);														\
		out << oper;													\
		r->eval(s);														\
	}																	\
};
Binary_operator_expr(PlusExpr, "+")
Binary_operator_expr(MinusExpr, "-")
Binary_operator_expr(MultExpr, "*")
Binary_operator_expr(DivExpr, "/")
Binary_operator_expr(ModExpr, "%")
Binary_operator_expr(LessExpr, "<")
Binary_operator_expr(GreaterExpr, ">")
Binary_operator_expr(EqualityExpr, "==")
Binary_operator_expr(InequalityExpr, "!=")
Binary_operator_expr(AndExpr, "&&")
Binary_operator_expr(OrExpr, "||")
#undef Binary_operator_expr

// Prefix (~)
class PrefixExpr : public Expression
{
private:
	unique_ptr<Expression> p;
public:
	explicit PrefixExpr(unique_ptr<Expression> &&p) :
		p(move(p))
	{}
	virtual void eval(Scope & s) {
		out << "!";
		p->eval(s);
	}
};

// Variable
class VarExpr : public Expression
{
private:
	string id;
public:
	explicit VarExpr(const string & id) :
		id(id)
	{}
	virtual void eval(Scope & s) {
		if (s.find(id) == s.end()) throw invalid_argument("Error");
		out << id;
	}
	string Identifier() const {
		return id;
	}
};

// Assign
class AssignExpr : public Expression
{
	string a;
	unique_ptr<Expression> r;
public:
	AssignExpr(const string &a, unique_ptr<Expression> &&r) :
		a(a), r(move(r))
	{}
	virtual void eval(Scope& s) {
		if (s.find(a) == s.end()) {
			out << "uint64_t ";
			s[a] = 0;
		}
		if (s[a] == 1 || s[a] == 2) throw invalid_argument("Error");
		out << a + "=";
		r->eval(s);
	}
};

// Program
class ProgExpr : public Expression
{
	unique_ptr<Expression> p, r;
public:
	explicit ProgExpr(unique_ptr<Expression> &&p, unique_ptr<Expression> &&r) :
		p(move(p)), r(move(r))
	{}
	virtual void eval(Scope & s) {
		p->eval(s);
		out << ";\n";
		r->eval(s);
	}
};

// Return
class ReturnExpr : public Expression
{
private:
	unique_ptr<Expression> r;
public:
	explicit ReturnExpr(unique_ptr<Expression> &&r) :
		r(move(r))
	{}
	virtual void eval(Scope & s) {
		out << "return ";
		r->eval(s);
		out << ";";
	}
};


// Declaration of function
class FuncDeclExpr : public Expression
{
public:
	string label;
	shared_ptr<Expression> name;
	map<string, unique_ptr<Expression>> params;
	unique_ptr<Expression> body;
	string str;
	Scope scope;
	virtual void eval(Scope & s) {
		if (label == "main")
			out << "int ";
		else out << "uint64_t ";
		out << label;
		out << "(";
		for (auto const& param : params) {
			str += "uint64_t " + param.first + ",";
			s[param.first] = 2;
		}
		if (str != "") str = str.substr(0, str.size() - 1);
		out << str;
		out << ")";
		body->eval(s);
	}
};

// Function call
class FuncCallExpr : public Expression
{
public:
	shared_ptr<Expression> l;
	vector<unique_ptr<Expression>> params;
	FuncCallExpr(shared_ptr<Expression>& l, vector<unique_ptr<Expression>> &r) :
		l(move(l)), params(move(r))
	{}
	virtual void eval(Scope& s) {
		l->eval(s);
		out << "(";
		if (params.size() > 0) {
			for (size_t p = 0; p < params.size()-1; ++p) {
				params[p]->eval(s);
				out << ",";
			}
			params[params.size() - 1]->eval(s);
		}
		
		out << ")";
	}
};

// if condition
class ConditionExpr : public Expression
{
public:
	unique_ptr<Expression> cond, if_expr, else_expr;
	ConditionExpr(unique_ptr<Expression> &&cond, unique_ptr<Expression>&& if_expr, unique_ptr<Expression>&& else_expr) :
		cond(move(cond)), if_expr(move(if_expr)), else_expr(move(else_expr))
	{}
	virtual void eval(Scope & s) {
		out << "if";
		out << "(";
		cond->eval(s);
		out << ")";
		if_expr->eval(s);
		else_expr->eval(s);
	}
};

// short if condition
class CondShortExpr : public Expression
{
public:
	unique_ptr<Expression> cond, if_expr, else_expr;
	CondShortExpr(unique_ptr<Expression> &&cond, unique_ptr<Expression>&& if_expr, unique_ptr<Expression>&& else_expr) :cond(move(cond)), if_expr(move(if_expr)), else_expr(move(else_expr)) {}
	virtual void eval(Scope & s) {
		out << "(";
		cond->eval(s);
		out << "?";
		if_expr->eval(s);
		out << ":";
		else_expr->eval(s);
		out << ")";
	}
};

// Body
class BodyExpr : public Expression
{
private:
	unique_ptr<Expression> b;
public:
	Scope s;
	explicit BodyExpr(unique_ptr<Expression> &&b) :
		b(move(b))
	{}
	virtual void eval(Scope & s) {
			out << "{";
			b->eval(s);
			out << "}\n";

	}
};

using Funcs = map<string, unique_ptr<FuncDeclExpr>>;
using LabelFunc = vector<string>;

unique_ptr<Expression> parse_mult_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_add_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_stmt_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_bool_expression_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_prog_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_braces_exp(Funcs & f, const vector<Token> & tokens, size_t& p);
unique_ptr<Expression> parse_func_call_exp(Funcs & f, const vector<Token> & tokens, size_t& p);

// Number
unique_ptr<Expression> parse_number(const vector<Token> & tokens, size_t& p) {
	if (p < tokens.size() && tokens[p].type == Token::Number) {
		return make_unique<NumExpr>(tokens[p++].value);
	}
	return unique_ptr<Expression>();
}

// Variable
unique_ptr<Expression> parse_variable(const vector<Token> & tokens, size_t& p) {
	if (p < tokens.size() && tokens[p].type == Token::Variable) {
		return make_unique<VarExpr>(tokens[p++].strValue);
	}
	return unique_ptr<Expression>();
}

// { P }
unique_ptr<Expression> parse_braces_exp(Funcs & f, const vector<Token> & tokens, size_t& p) {
	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::LeftBrace) {
		++p;
		auto prog = parse_prog_exp(f, tokens, p);
		if (p < tokens.size() && tokens[p].type == Token::RightBrace) {
			++p;
			return make_unique<BodyExpr>(move(prog));
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}

// { S;P }
unique_ptr<Expression> parse_stprog_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	size_t tmp = p;
	unique_ptr<Expression> st = parse_stmt_exp(f, tokens, p);
	size_t tmp_st = p;
	if (st) {
		if (p < tokens.size() && tokens[p].type == Token::Semicolon) {
			++p;
			unique_ptr<Expression> r = parse_prog_exp(f, tokens, p);
			if (r) return make_unique<ProgExpr>(move(st), move(r));
		}
	}
	p = tmp;
	unique_ptr<Expression> r = parse_add_exp(f, tokens, p);
	if (r) {
		return make_unique<ReturnExpr>(move(r));
	}
	if (st)
	{
		p = tmp_st;
		return st;
	}
	p = tmp;
	return unique_ptr<Expression>();
}

unique_ptr<Expression> parse_prog_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	//S;P 
	unique_ptr<Expression> ptr = parse_stprog_exp(f, tokens, p);
	if (ptr)
		return ptr;

	return ptr;
}

// M | M + A | M - A 
unique_ptr<Expression> parse_add_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	unique_ptr<Expression> m = parse_mult_exp(f, tokens, p);
	if (!m) return m;

	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::Plus)
	{
		++p;
		unique_ptr<Expression> a = parse_add_exp(f, tokens, p);
		if (a) return make_unique<PlusExpr>(move(m), move(a));
	}
	else if (p < tokens.size() && tokens[p].type == Token::Minus)
	{
		++p;
		unique_ptr<Expression> a = parse_add_exp(f, tokens, p);
		if (a) return make_unique<MinusExpr>(move(m), move(a));
	}
	p = tmp;
	return m;

}


// ( Bool_Expr ) ? P : P
unique_ptr<Expression> parse_cond_short_exp(Funcs & f, const vector<Token> & tokens, size_t& p) {
	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::If) {
		++p;
		if (p < tokens.size() && tokens[p].type == Token::LeftParen) {
			++p;
			auto bool_expr = parse_bool_expression_exp(f, tokens, p);
			if (bool_expr) {
				if (p < tokens.size() && tokens[p].type == Token::RightParen) {
					++p;
					if (p < tokens.size() && tokens[p].type == Token::LeftBrace) {
						++p;
						auto if_expr = parse_add_exp(f, tokens, p);
						if (p < tokens.size() && tokens[p].type == Token::RightBrace) {
							++p;
							if (p < tokens.size() && tokens[p].type == Token::LeftBrace) {
								++p;
								auto else_expr = parse_add_exp(f, tokens, p);
								if (p < tokens.size() && tokens[p].type == Token::RightBrace) {
									++p;
									if (if_expr && else_expr)
										return make_unique<CondShortExpr>(move(bool_expr), move(if_expr), move(else_expr));
								}
							}
						}
					}
				}
			}
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}

unique_ptr<Expression> parse_basic_exp(Funcs & f, const vector<Token> & tokens, size_t& p) {

	// Number
	unique_ptr<Expression> ptr = parse_number(tokens, p);
	if (ptr)
		return ptr;

	// Function call
	ptr = parse_func_call_exp(f, tokens, p);
	if (ptr) return ptr;

	// ( Bool_Expr ) ? P : P
	ptr = parse_cond_short_exp(f, tokens, p);
	if (ptr) return ptr;

	// Variable
	ptr = parse_variable(tokens, p);
	if (ptr)
		return ptr;

	// { P }
	ptr = parse_braces_exp(f, tokens, p);
	if (ptr)
		return ptr;

	size_t tmp = p;

	// ( A )
	if (p < tokens.size() && tokens[p].type == Token::LeftParen)
	{
		++p;
		unique_ptr<Expression> ptr = parse_add_exp(f, tokens, p);
		if (ptr && tokens[p].type == Token::RightParen)
		{
			++p;
			return ptr;
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}

// B | B * M | B \ M | B % M
unique_ptr<Expression> parse_mult_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	unique_ptr<Expression> b = parse_basic_exp(f, tokens, p);
	if (!b) return b;

	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::Mult)
	{
		++p;
		unique_ptr<Expression> m = parse_mult_exp(f, tokens, p);
		if (m) return make_unique<MultExpr>(move(b), move(m));
	}
	else if (p < tokens.size() && tokens[p].type == Token::Div)
	{
		++p;
		unique_ptr<Expression> m = parse_mult_exp(f, tokens, p);
		if (m) return make_unique<DivExpr>(move(b), move(m));
	}
	else if (p < tokens.size() && tokens[p].type == Token::Mod)
	{
		++p;
		unique_ptr<Expression> m = parse_mult_exp(f, tokens, p);
		if (m) return make_unique<ModExpr>(move(b), move(m));
	}
	p = tmp;
	return b;
}

// if ( Bool_Expr ) { P } { P }
unique_ptr<Expression> parse_condition_exp(Funcs & f, const vector<Token> & tokens, size_t& p) {
	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::If) {
		++p;
		if (p < tokens.size() && tokens[p].type == Token::LeftParen) {
			++p;
			auto bool_expr = parse_bool_expression_exp(f, tokens, p);
			if (bool_expr) {
				if (p < tokens.size() && tokens[p].type == Token::RightParen) {
					++p;
					auto if_expr = parse_braces_exp(f, tokens, p);
					auto else_expr = parse_braces_exp(f, tokens, p);
					if (if_expr && else_expr)
						return make_unique<ConditionExpr>(move(bool_expr), move(if_expr), move(else_expr));
				}
			}
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}

// Var := A
unique_ptr<Expression> parse_assign_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	size_t tmp = p;
	unique_ptr<Expression> a = parse_variable(tokens, p);
	if (a) {
		if (p < tokens.size() && tokens[p].type == Token::Assign) {
			++p;
			unique_ptr<Expression> r = parse_add_exp(f, tokens, p);
			if (r) return make_unique<AssignExpr>(dynamic_cast<VarExpr*>(a.get())->Identifier(), move(r));
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}

// ~ Bool_expr
unique_ptr<Expression> parse_prefix_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::Prefix)
	{
		++p;
		unique_ptr<Expression> r = parse_bool_expression_exp(f, tokens, p);
		if (r) return make_unique<PrefixExpr>(move(r));
	}
	p = tmp;
	return unique_ptr<Expression>();
}

// ~B_Expr | B | B == B_Expr | B != B_Expr | B > B_Expr | B < B_Expr | B && B_Expr | B || B_Expr
unique_ptr<Expression> parse_bool_expression_exp(Funcs & f, const vector<Token> & tokens, size_t& p) {

	unique_ptr<Expression> ptr = parse_prefix_exp(f, tokens, p);
	if (ptr) return ptr;

	unique_ptr<Expression> b = parse_basic_exp(f, tokens, p);
	if (!b) return b;

	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::Equality) {
		++p;
		unique_ptr<Expression> e = parse_bool_expression_exp(f, tokens, p);
		if (e) return make_unique<EqualityExpr>(move(b), move(e));
	}
#define parse_bin(token_type,ptr_type)											\
	else if (p < tokens.size() && tokens[p].type == token_type){				\
			++p;																\
			unique_ptr<Expression> r = parse_bool_expression_exp(f, tokens, p);	\
			if (r) return make_unique<ptr_type>(move(b), move(r));				\
	}																			
	parse_bin(Token::Inequality, InequalityExpr)
		parse_bin(Token::Greater, GreaterExpr)
		parse_bin(Token::Less, LessExpr)
		parse_bin(Token::And, AndExpr)
		parse_bin(Token::Or, OrExpr)
#undef parse_bin
		p = tmp;
	return b;
}


unique_ptr<Expression> parse_stmt_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	//Var := A
	unique_ptr<Expression> ptr = parse_assign_exp(f, tokens, p);
	if (ptr)
		return ptr;

	// Function call
	ptr = parse_func_call_exp(f, tokens, p);
	if (ptr)
		return ptr;

	//if Bool_Expr { P } { P }
	ptr = parse_condition_exp(f, tokens, p);
	if (ptr)
		return ptr;

	//{ P }
	ptr = parse_braces_exp(f, tokens, p);

	if (ptr)
	{ }
		return ptr;

	return ptr;
}

// Function call label ( p1, ..., pn )
unique_ptr<Expression> parse_func_call_exp(Funcs & f, const vector<Token> & tokens, size_t& p)
{
	size_t tmp = p;
	if (p < tokens.size() && tokens[p].type == Token::Variable) {
		unique_ptr<Expression> v = parse_variable(tokens, p);
		if (v) {
			string f_name = dynamic_cast<VarExpr*>(v.get())->Identifier();
			if (f.find(f_name) != f.end()) {
				if (p < tokens.size() && tokens[p].type == Token::LeftParen) {
					++p;
					vector<unique_ptr<Expression>> params;
					while (tokens[p].type != Token::RightParen) {
						auto param = parse_add_exp(f, tokens, p);
						if (param)
							params.push_back(move(param));
						if (p < tokens.size() && tokens[p].type != Token::Comma)
							break;
						else ++p;
					}
					if (p < tokens.size() && tokens[p].type == Token::RightParen) {
						++p;
						if (params.size() == f[f_name]->params.size()) {
							auto l = f[f_name]->name;
							return make_unique<FuncCallExpr>(l, params);
						}
					}
				}
			}
		}
	}
	p = tmp;
	return unique_ptr<Expression>();
}


// label p1, ..., pn { P }
string parse_f(Funcs & f, const vector<Token> & tokens, size_t& p) {
	size_t tmp = p;
	string f_label;
	if (p < tokens.size() && tokens[p].type == Token::Variable) {
		auto f_name = parse_variable(tokens, p);
		f_label = dynamic_cast<VarExpr*>(f_name.get())->Identifier();
		if (f.find(f_label) == f.end()) {
			f[f_label] = make_unique<FuncDeclExpr>();
			if (f_name) {
				f[f_label]->name = move(f_name);
				f[f_label]->label = f_label;
				while (p < tokens.size() && tokens[p].type != Token::LeftBrace) {
					auto param = parse_variable(tokens, p);
					if (param) {
						string p_name = dynamic_cast<VarExpr*>(param.get())->Identifier();
						f[f_label]->params[p_name] = move(param);
					}
				}
				unique_ptr<Expression> prog = parse_braces_exp(f, tokens, p);
				if (prog) {
					f[f_label]->body = move(prog);
					return f_label;
				}
			}
		}
	}
	if (f_label != "") {
		f.erase(f_label);
	}
	p = tmp;
	return "";
}



// fns
void main_program(Funcs & f, const vector<Token> & tokens, size_t& p, bool bool_write = false) {
	f["read"] = make_unique<FuncDeclExpr>();
	f["read"]->name = make_shared<VarExpr>("read");
	f["read"]->label = "read";
	f["write"] = make_unique<FuncDeclExpr>();
	f["write"]->label = "cout";
	f["write"]->params["par"] = unique_ptr<Expression>();
	f["write"]->name = make_shared<VarExpr>("write");
	LabelFunc order;

	while (p < tokens.size()) {
		string name = parse_f(f, tokens, p);
		if (name == "")
			throw invalid_argument("Error. Main missing");
		else
			order.push_back(name);

	}
	if (!f["main"]) throw invalid_argument("Error. Main missng");

	out << "#include <iostream>" << endl;
	out << "#include <cstdint>" << endl;
	out << "#include <stdint.h>" << endl;
	out << "uint64_t read() {uint64_t x; scanf_s(\"%lu\", &x); return x;}" << endl;
	for (auto &&tk : tokens) {
		if (tk.strValue == "write" && bool_write == false) {
			out << "int64_t write(uint64_t x) {printf_s(\"%lu\\n\", x); return 0;}" << endl;
			bool_write = true;
		}
	}

	for (auto i = 0; i < order.size(); ++i) {
		for (auto const& x : f) {
			f[order[i]]->scope[x.first] = 1;
		}
		f[order[i]]->eval(f[order[i]]->scope);
	}
}

int main()
{
	try {
		auto t = Tokenize(cin);
		if (t[0].type == Token::Error)
		{
			cout << "Error, fail while tokenizing" << endl;
			return -1;
		}
		size_t nul = 0;
		Funcs f;
		main_program(f, t, nul);
		cout << out.str();
	}
	catch (...) {
		cout << "error";
	}
}

