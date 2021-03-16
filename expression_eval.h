#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <memory>
#include <cassert>
#include <exception>
#include <map>

// custom (basic) Either implementation

template<class T, class U>
class Either {
    bool _isLeft;
    union {
	T left;
	U right;
    };
    template <class T_, class U_> friend Either<T_, U_> Left(T_ x);
    template <class T_, class U_> friend Either<T_, U_> Right(U_ x);
public:
    bool isLeft(){
	return _isLeft;
    }
    bool isRight(){
	return !_isLeft;
    }
    T unsafeGetLeft(){
	// unsafe if not in `isLeft` check!
	return left;
    }
    U unsafeGetRight(){
	// unsafe if not in `isLeft` check!
	return right;
    }
    T getLeft(){
	if(_isLeft){
	    return left;
	}
	else{
	    throw std::runtime_error(std::string("Cannot return `left` as Either is right!"));
	}
    }
    U getRight(){
	if(!_isLeft){
	    return right;
	}
	else{
	    throw std::runtime_error(std::string("Cannot return `right` as Either is left!"));
	}
    }
};

template <class T_, class U_> Either<T_, U_> Left(T_ x){
    Either<T_, U_> result;
    result._isLeft = true;
    result.left = x;
    return result;
}

template <class T_, class U_> Either<T_, U_> Right(U_ x){
    Either<T_, U_> result;
    result._isLeft = false;
    result.right = x;
    return result;
}

using namespace std;

enum TokenKind {
    tkInvalid,
    tkIdent, tkFloat,
    tkParensOpen, tkParensClose,
    tkMul, tkDiv, tkPlus, tkMinus,
    tkLess, tkGreater, tkLessEq, tkGreaterEq,
    tkEqual, tkUnequal,
    tkAnd, tkOr, tkNot,
    tkSqrt
};

static inline string toString(TokenKind tkKind){
    switch(tkKind) {
        case tkParensOpen : return "(";
        case tkParensClose : return ")";
        case tkMul : return "*";
        case tkDiv : return "/";
        case tkPlus : return "+";
        case tkMinus : return "-";
        case tkLess : return "<";
        case tkGreater : return ">";
        case tkLessEq : return "<=";
        case tkGreaterEq : return ">=";
        case tkAnd : return "&&";
        case tkOr : return "||";
        case tkUnequal : return "!=";
        case tkEqual : return "==";
        case tkNot : return "!";
        case tkSqrt : return "sqrt";
        case tkIdent: return "Ident";
        case tkFloat: return "Float";
        default: return "Invalid " + tkKind;
    }
}

typedef struct Token {
    Token(TokenKind tkKind): kind(tkKind), name(toString(tkKind)) {};
    Token(TokenKind tkKind, string name): kind(tkKind), name(name) {};
    Token operator=(const Token& tok) {return tok;};
    const TokenKind kind;
    const string name;
} Token;

static inline string toString(Token tok){
    string res = toString(tok.kind);
    switch(tok.kind){
        case tkIdent:
            res += ": " + tok.name;
            break;
        case tkFloat:
            res += ": " + tok.name;
            break;
	default: break;
    }
    return res;
}

static inline Token toIdentAndClear(string &s){
    if(s.length() > 0){
        Token tok = Token(tkIdent, s);
        s.clear();
        return tok;
    }
    else{
        return Token(tkInvalid);
    }
}

enum NodeKind {
    nkUnary, nkBinary, nkFloat, nkIdent,
    nkExpression // for root as well as parens
    // , nkCall (for sqrt, pow etc), ?
};

enum BinaryOpKind {
    boMul, boDiv, boPlus, boMinus,
    boLess, boGreater, boLessEq, boGreaterEq,
    boEqual, boUnequal,
    boAnd, boOr
};

enum UnaryOpKind {
    uoPlus, uoMinus, uoNot
};

class Node {
public:
    NodeKind kind;
    virtual UnaryOpKind GetUnaryOp() {return uoPlus;}; // wtf why have to return something
    virtual shared_ptr<Node> GetUnaryNode() {return nullptr;};
    virtual void SetUnaryNode(shared_ptr<Node> node) {};
    virtual BinaryOpKind GetBinaryOp() {return boPlus;};
    virtual shared_ptr<Node> GetLeft() {return nullptr;};
    virtual shared_ptr<Node> GetRight() {return nullptr;};
    virtual void SetLeft(shared_ptr<Node> node) {};
    virtual void SetRight(shared_ptr<Node> node) {};
    virtual double GetVal() {return 0.0;};
    virtual string GetIdent() {return "";};
    virtual shared_ptr<Node> GetExprNode() {return nullptr;};
};

class UnaryNode: public Node {
public:
    UnaryNode (UnaryOpKind op, shared_ptr<Node> n): op(op), n(n) {
        kind = nkUnary;
    };
    UnaryOpKind GetUnaryOp() override {return op;};
    shared_ptr<Node> GetUnaryNode() override {return n;};
    void SetUnaryNode(shared_ptr<Node> node) override {n = node;};
    UnaryOpKind op;
    shared_ptr<Node> n;
};

class BinaryNode: public Node {
public:
    BinaryNode(BinaryOpKind op, shared_ptr<Node> left, shared_ptr<Node>(right)):
        op(op), left(left), right(right) {
        kind = nkBinary;
    };
    BinaryOpKind GetBinaryOp() override {return op;};
    shared_ptr<Node> GetLeft() override {return left;};
    shared_ptr<Node> GetRight() override {return right;};
    void SetLeft(shared_ptr<Node> node) override {left = node;};
    void SetRight(shared_ptr<Node> node) override {right = node;};
    BinaryOpKind op;
    shared_ptr<Node> left;
    shared_ptr<Node> right;
};

class FloatNode: public Node {
public:
    FloatNode(double val): val(val) {
        kind = nkFloat;
    };
    double GetVal() override {return val;};
    double val;
};

class IdentNode: public Node {
public:
    IdentNode(string ident): ident(ident) {
        kind = nkIdent;
    };
    string GetIdent() override {return ident;};
    string ident;
};

class ExpressionNode: public Node {
public:
    shared_ptr<Node> GetExprNode() override {return node;};
    shared_ptr<Node> node;
};

using Expression = shared_ptr<Node>;

static inline BinaryOpKind toBinaryOpKind(TokenKind kind){
    switch(kind){
        case tkMul: return boMul;
        case tkDiv: return boDiv;
        case tkPlus: return boPlus;
        case tkMinus: return boMinus;
        case tkLess: return boLess;
        case tkGreater: return boGreater;
        case tkLessEq: return boLessEq;
        case tkGreaterEq: return boGreaterEq;
        case tkEqual: return boEqual;
        case tkUnequal: return boUnequal;
        case tkAnd: return boAnd;
        case tkOr: return boOr;
        default:
            cout << "Invalid binary op kind for token " << kind << endl;
            throw;
    }
}

static inline string toStr(BinaryOpKind op){
    switch(op){
        case boMul: return "*";
        case boDiv: return "/";
        case boPlus: return "+";
        case boMinus: return "-";
        case boGreater: return ">";
        case boGreaterEq: return ">=";
        case boLess: return "<";
        case boLessEq: return "<=";
        case boEqual: return "==";
        case boUnequal: return "!=";
        case boAnd: return "&&";
        case boOr: return "||";
        default: return "";
    }
}

static inline UnaryOpKind toUnaryOpKind(TokenKind kind){
    switch(kind){
        case tkPlus: return uoPlus;
        case tkMinus: return uoMinus;
        case tkNot: return uoNot;
        default:
            cout << "Invalid unary op kind for token " << kind << endl;
            throw;
    }
}

static inline string toStr(UnaryOpKind op){
    switch(op){
        case uoPlus: return "+";
        case uoMinus: return "-";
        case uoNot: return "!";
        default: return "";
    }
}

inline string astToStr(shared_ptr<Node> n){
    // recursively call this proc until we reach ident or float nodes.
    // NOTE: This proc assumes that the children of unary / binary are never NULL!
    if(n == NULL) return "";
    string res;
    switch(n->kind){
        case nkUnary:
            res += "(" + toStr(n->GetUnaryOp()) + " " + astToStr(n->GetUnaryNode()) + ")";
            break;
        case nkBinary:
            res += "(" + toStr(n->GetBinaryOp()) + " " + astToStr(n->GetLeft()) + " " + astToStr(n->GetRight()) + ")";
            break;
        case nkFloat:
            res += to_string(n->GetVal());
            break;
        case nkIdent:
            res += n->GetIdent();
            break;
        case nkExpression:
            res += "(" + astToStr(n->GetExprNode()) + ")";
            break;
    }
    return res;
}

template <class T>
static vector<T> sliceCopy(vector<T> v, int start, int end){
    // returns inclusive (copied) sliceCopy from start to end
    vector<T> result;
    if(start < 0) return result;
    int endIdx = min(end, (int)v.size() - 1);
    for(int i = start; i <= endIdx; i++){
        result.push_back(v[i]);
    }
    return result;
}

static inline shared_ptr<Node> binaryNode(BinaryOpKind op, shared_ptr<Node> left, shared_ptr<Node> right){
    return shared_ptr<Node>(new BinaryNode(op, left, right));
}

static inline shared_ptr<Node> binaryNode(BinaryOpKind op){
    return shared_ptr<Node>(new BinaryNode(op, nullptr, nullptr));
}

static inline shared_ptr<Node> unaryNode(UnaryOpKind op, shared_ptr<Node> n){
    return shared_ptr<Node>(new UnaryNode(op, n));
}

static inline shared_ptr<Node> unaryNode(UnaryOpKind op){
    return shared_ptr<Node>(new UnaryNode(op, nullptr));
}

static inline shared_ptr<Node> identOrFloatNode(Token tok){
    try{
        double val = stod(tok.name);
        return shared_ptr<Node>(new FloatNode(val));
    }
    catch (...) {
        // not a valid float, just an identifier
        return shared_ptr<Node>(new IdentNode(tok.name));
    }
}

static inline bool binaryOrUnary(vector<Token> tokens, int idx){
    if((idx > 1 && (tokens[idx-1].kind == tkFloat || tokens[idx-1].kind == tkIdent)) &&
       (((size_t)idx < tokens.size() - 1 && tokens[idx+1].kind == tkFloat) || tokens[idx+1].kind == tkIdent)){
        return true;
    }
    else{
        return false;
    }
}

inline shared_ptr<Node> parseNode(Token tok, vector<Token> tokens, int idx){
    bool inExp = false;
    shared_ptr<Node> result;
    int exprStart = 0;
    switch(tok.kind){
        case tkParensOpen:
            // start a ExpressionNode
            inExp = true;
            exprStart = idx;
            break;
        case tkParensClose:
            if(!inExp){
                cout << "Invalid input! No opening parens for closing parens found!" << endl;
                throw;
            }
            inExp = false;
            // parse everything in between into node
            result = parseNode(tok, sliceCopy(tokens, exprStart, idx), idx);
            break;
        case tkMul:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkDiv:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkLess:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkGreater:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkLessEq:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkGreaterEq:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkEqual:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkUnequal:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkAnd:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkOr:
            result = binaryNode(toBinaryOpKind(tok.kind));
            break;
        case tkNot:
            result = unaryNode(toUnaryOpKind(tok.kind));
            break;
        case tkPlus: {// possibly binary or unary
            bool isBinary = binaryOrUnary(tokens, idx);
            if(isBinary){
                result = binaryNode(toBinaryOpKind(tok.kind));
            }
            else{
                result = unaryNode(toUnaryOpKind(tok.kind));
            }
            break;
        }

        case tkMinus: {
            bool isBinary = binaryOrUnary(tokens, idx);
            if(isBinary){
                result = binaryNode(toBinaryOpKind(tok.kind));
            }
            else{
                result = unaryNode(toUnaryOpKind(tok.kind));
            }
            break;
        }
        case tkSqrt: // do nothing
            break;
        case tkIdent:
            result = identOrFloatNode(tokens[idx]);
            break;
        case tkFloat:
            result = identOrFloatNode(tokens[idx]);
            break;
        default:
            cout << "INVALID TOKEN: " << toString(tok) << endl;
            abort();
    }
    return result;
}

static inline int getPrecedence(TokenKind tkKind){
    switch(tkKind) {
        case tkParensOpen : return 10;
        case tkParensClose : return 10;
        case tkMul : return 9;
        case tkDiv : return 9;
        case tkPlus : return 8;
        case tkMinus : return 8;
        case tkLess : return 7;
        case tkGreater : return 7;
        case tkLessEq : return 7;
        case tkGreaterEq : return 7;
        case tkEqual : return 7;
        case tkUnequal : return 7;
        case tkAnd : return 6;
        case tkOr : return 6;
        case tkNot : return 6;
        case tkSqrt : return 5;
        case tkIdent: return 0;
        case tkFloat: return 0;
        default: return -1;
    }
}

static inline int getPrecedence(BinaryOpKind opKind){
    switch(opKind) {
        case boMul : return 9;
        case boDiv : return 9;
        case boPlus : return 8;
        case boMinus : return 8;
        case boLess : return 7;
        case boGreater : return 7;
        case boLessEq : return 7;
        case boGreaterEq : return 7;
        case boEqual : return 7;
        case boUnequal : return 7;
        case boAnd : return 6;
        case boOr : return 6;
        default: return -1;
    }
}

static inline int getPrecedence(UnaryOpKind opKind){
    switch(opKind) {
        case uoPlus : return 8;
        case uoMinus : return 8;
        case uoNot : return 6;
        default: return -1;
    }
}

static inline Token nextOpTok(vector<Token> tokens, int& idx){
    while((size_t)idx < tokens.size()){
        if(tokens[idx].kind > tkParensClose && tokens[idx].kind < tkSqrt){
            return tokens[idx];
        }
        else{
            idx++;
        }
    }
    return Token(tkInvalid);
}

static inline bool setLastRightNode(shared_ptr<Node> node, shared_ptr<Node> right){
    if(node->GetRight() != NULL){
        auto bval = setLastRightNode(node->GetRight(), right);
        if(bval){
            return true;
        }
    }
    node->SetRight(right);
    return true;
}

static inline bool setLastUnaryNode(shared_ptr<Node> node, shared_ptr<Node> right){
    if(node->kind != nkUnary){
        auto bval = setLastUnaryNode(node->GetRight(), right);
        if(bval){
            return true;
        }
    }
    node->SetUnaryNode(right);
    return true;
}

static inline void setNodeWithPrecedence(shared_ptr<Node> node, int precedence){
    while(node->kind == nkBinary &&
	  ((node->GetRight()->kind == nkBinary &&
	    getPrecedence(node->GetRight()->GetBinaryOp()) < precedence) ||
	   node->GetRight() == NULL)){
	node = node->GetRight();
    }
}

static inline void setNode(Expression& ast, shared_ptr<Node> toSet,
             int lastPrecedence, Token nextOp){
    if(toSet->kind == nkBinary){
        // have to find correct place to insert new node
	auto node = ast;
	setNodeWithPrecedence(node, getPrecedence(toSet->GetBinaryOp()));
	if(node->kind == nkBinary){
            assert(node->kind == nkBinary);
	    if(getPrecedence(node->GetBinaryOp()) < getPrecedence(toSet->GetBinaryOp())){
                auto tmp = node->GetRight();
                toSet->SetLeft(tmp);
                node->SetRight(toSet);
	    }
	    else{
		toSet->SetLeft(node);
		ast = toSet;
	    }
	}
	else{
	    toSet->SetLeft(ast);
	    ast = toSet;
	}
    }
    else{
        // traverse the result node until we find a binary node with an operator of
        // *higher* precedence than `toSet` or a NULL node
	auto node = ast;
	setNodeWithPrecedence(node, getPrecedence(toSet->GetBinaryOp()));
        if(node->GetRight() == NULL){
            node->SetRight(nullptr);
            toSet->SetLeft(node);
            node->SetRight(toSet);
        }
        else{
            auto tmp = node->GetRight();
            toSet->SetLeft(tmp);
            node->SetRight(toSet);
        }
    }
}

static inline Expression tokensToAst(vector<Token> tokens){
    // parses the given vector of tokens into an AST. Done by respecting operator precedence
    // TODO possibly have to sort by operator precedence
    ExpressionNode exp;
    Expression result = nullptr;
    shared_ptr<Node> n;
    int idx = 0;
    int lastPrecedence = 0;
    bool lastWasUnary = false;
    Token lastOp = Token(tkInvalid);
    while((size_t)idx < tokens.size()){
        // essentially have to skip until we find an operator based token and *not*
        // float / ident. Thene decide where to attach the resulting thing based
        // on precedence table
        int opIdx = idx;
        auto tok = tokens[idx];
        auto nextOp = nextOpTok(tokens, ref(opIdx));
        int precedence = getPrecedence(tok.kind);

        // order switch statement based on precedence.
        n = parseNode(tok, tokens, idx);
        //cout << "Current node " << astToStr(n) << endl;
        if(result == nullptr){
            // start by result being first node (e.g. pure string or float)
            result = n;
        }
        // check if we are looking at a unary/binary op, if so build tree
        if(nextOp.kind == tok.kind){
            switch(n->kind){
                case nkBinary:
                    // add last element as left child
                    setNode(ref(result), n, lastPrecedence, nextOp);
                    break;
                case nkUnary:
                    // single child comes after this token
                    lastWasUnary = true;
                    // TODO: fix for case result isn't binary!!
                    setLastRightNode(result, n);
                    break;
                // else nothing to do
		default: break;
            }
        }
        else{
            switch(result->kind){
                case nkBinary:
                    // add n to result right
                    // traverse and set **last** right node
                    setLastRightNode(result, n);
                    break;
                case nkUnary:
                    // add to result node
                    result = n;
		    break;
		default: break;
            }
            // append to last
            if(lastWasUnary){
                setLastUnaryNode(result, n);
            }
            lastWasUnary = false;
        }

        if(nextOp.kind == tok.kind){
            lastPrecedence = precedence;
            lastOp = nextOp;
        }
        idx++;
    }
    cout << "Resulting expression " << astToStr(result) << endl;
    return result;
}

static inline void addTokenIfValid(vector<Token>& v, Token tok){
    // adds the given token to `v` if `tok` is not `tkInvalid`
    if(tok.kind != tkInvalid){
        v.push_back(tok);
    }
}

inline Either<double, bool> negative(Either<double, bool> x){
    assert(x.isLeft());
    return Left<double, bool>(-x.unsafeGetLeft());
}

inline Either<double, bool> negateCmp(Either<double, bool> x){
    // NOTE: we do *not* support arbitrary casting of floats to bools!
    assert(x.isRight());
    return Left<double, bool>(!x.unsafeGetRight());
}

inline Either<double, bool> multiply(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Left<double, bool>(x.unsafeGetLeft() * y.unsafeGetLeft());
}

inline Either<double, bool> divide(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Left<double, bool>(x.unsafeGetLeft() / y.unsafeGetLeft());
}

inline Either<double, bool> plusCmp(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Left<double, bool>(x.unsafeGetLeft() + y.unsafeGetLeft());
}

inline Either<double, bool> minusCmp(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Left<double, bool>(x.unsafeGetLeft() - y.unsafeGetLeft());
}

inline Either<double, bool> lessCmp(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Right<double, bool>(x.unsafeGetLeft() < y.unsafeGetLeft());
}

inline Either<double, bool> greaterCmp(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Right<double, bool>(x.unsafeGetLeft() > y.unsafeGetLeft());
}

inline Either<double, bool> lessEq(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Right<double, bool>(x.unsafeGetLeft() <= y.unsafeGetLeft());
}

inline Either<double, bool> greaterEq(Either<double, bool> x, Either<double, bool> y){
    assert(x.isLeft());
    assert(y.isLeft());
    return Right<double, bool>(x.unsafeGetLeft() >= y.unsafeGetLeft());
}

inline Either<double, bool> equal(Either<double, bool> x, Either<double, bool> y){
    if(x.isLeft() && y.isLeft()){
	// TODO: make comparison float precision safe
	return Right<double, bool>(x.unsafeGetLeft() == y.unsafeGetLeft());
    }
    else if(x.isRight() && y.isRight()){
	return Right<double, bool>(x.unsafeGetRight() == y.unsafeGetRight());
    }
    else{
	throw domain_error("Cannot compare a float and a bool for equality!");
    }
}

inline Either<double, bool> unequal(Either<double, bool> x, Either<double, bool> y){
    if(x.isLeft() && y.isLeft()){
	return Right<double, bool>(x.unsafeGetLeft() != y.unsafeGetLeft());
    }
    else if(x.isRight() && y.isRight()){
	return Right<double, bool>(x.unsafeGetRight() != y.unsafeGetRight());
    }
    else{
	throw domain_error("Cannot compare a float and a bool for inequality!");
    }
}

inline Either<double, bool> andCmp(Either<double, bool> x, Either<double, bool> y){
    if(x.isRight() && y.isRight()){
	return Right<double, bool>(x.unsafeGetRight() && y.unsafeGetRight());
    }
    else{
	throw domain_error("Cannot compute `and` of float and bool or two floats!");
    }
}

inline Either<double, bool> orCmp(Either<double, bool> x, Either<double, bool> y){
    if(x.isRight() && y.isRight()){
	return Right<double, bool>(x.unsafeGetRight() || y.unsafeGetRight());
    }
    else{
	throw domain_error("Cannot compute `or` of float and bool or two floats!");
    }
}

inline Either<double, bool> evaluate(map<string, float> m, shared_ptr<Node> n){
    switch(n->kind){
	case nkBinary:
	    // recurse on both childern
	    switch(n->GetBinaryOp()){
		case boMul: return multiply(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boDiv: return divide(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boPlus: return plusCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boMinus: return minusCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boLess: return lessCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boGreater: return greaterCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boLessEq: return lessEq(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boGreaterEq: return greaterEq(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boEqual: return equal(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boUnequal: return unequal(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boAnd: return andCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		case boOr: return orCmp(evaluate(m, n->GetLeft()), evaluate(m, n->GetRight()));
		default:
		    cout << "Invalid binary op kind for token " << astToStr(n) << endl;
		    throw;
	    }
	case nkUnary:
	    // apply unary op
	    switch(n->GetUnaryOp()){
		case uoPlus: return evaluate(m, n->GetUnaryNode());
		case uoMinus: return negative(evaluate(m, n->GetUnaryNode()));
		case uoNot: return negateCmp(evaluate(m, n->GetUnaryNode()));
	    }
	case nkIdent:
	    // return value stored for ident
	    // TODO: check if element exists in map (/in REST)
	    return Left<double, bool>(m[n->GetIdent()]);
	case nkFloat:
	    return Left<double, bool>(n->GetVal());
	case nkExpression:
	    return evaluate(m, n->GetExprNode());
    }
    cout << "Invalid code branch in `evaluate`. Should never end up here! " << endl;
    throw;
}

inline Expression parseExpression(string s){
    int idx = 0;
    vector<Token> tokens;
    string curBuf;
    while((size_t)idx < s.length()){
        switch(s[idx]){
            // single char tokens
            case '(':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkParensOpen));
                break;
            case ')':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkParensClose));
                break;
            case '*':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkMul));
                break;
            case '/':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkDiv));
                break;
            case '+':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkPlus));
                break;
            case '-':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                addTokenIfValid(ref(tokens), Token(tkMinus));
                break;
            // possible multi char tokens, i.e. accumulate until next token?
            case '<':
                // peek next
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                if((size_t)(idx + 1) < s.length() && s[idx + 1] == '='){
                    addTokenIfValid(ref(tokens), Token(tkLessEq));
                    idx++;
                }
                else{
                    addTokenIfValid(ref(tokens), Token(tkLess));
                }
                break;
            case '>':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                if((size_t)(idx + 1) < s.length() && s[idx + 1] == '='){
                    addTokenIfValid(ref(tokens), Token(tkGreaterEq));
                    idx++;
                }
                else{
                    addTokenIfValid(ref(tokens), Token(tkGreater));
                }
                break;
            case '&':
		// TODO: replace by `and` and `or`
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                // assert after is another &
                if(!((size_t)(idx + 1) < s.length()) && s[idx+1] != '&'){
                    // raise a parsing error
                    cout << "Bad formula! " << idx << "  " << s.length() << endl;
                    throw;
                }
                addTokenIfValid(ref(tokens), Token(tkAnd));
                // skip the next element
                idx++;
                break;
            case '=':
                addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
                // assert after is another =
                if(!((size_t)(idx + 1) < s.length()) && s[idx+1] != '='){
                    // raise a parsing error
                    cout << "Bad formula! " << idx << "  " << s.length() << endl;
                    throw;
                }
                addTokenIfValid(ref(tokens), Token(tkEqual));
                // skip the next element
                idx++;
                break;
            case ' ':
                break;
            default:
                curBuf += s[idx];
        }
        idx++;
    }
    addTokenIfValid(ref(tokens), toIdentAndClear(ref(curBuf)));
    cout << "curBuf " << curBuf << endl;
    int i = 0;
    for(auto tok : tokens){
        cout << "i " << i << "  " << toString(tok) << endl;
        i++;
    }
    return tokensToAst(tokens);
}