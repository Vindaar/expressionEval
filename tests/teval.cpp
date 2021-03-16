#include "../expression_eval.h"
#include <cassert>

using namespace std;

std::map<std::string, float> m = { {"hitsAna_energy", 6000}, {"hitsAna_xy2Sigma", 0.2} };

Either<double, bool> runIt(string s){
    auto expr = parseExpression(s);
    cout << "Is " << astToStr(expr) << endl;
    return evaluate(m, expr);
}

void testIt(string s, double exp){
    auto res = runIt(s);
    assert(res.isLeft());
    assert(res.unsafeGetLeft() == exp);
}

void testIt(string s, bool exp){
    auto res = runIt(s);
    assert(res.isRight());
    assert(res.unsafeGetRight() == exp);
}

void failToParse(string s){
    try{
	auto res = parseExpression(s);
	throw;
    }
    catch (...){
	assert(true);
    }
}

void simple() {
    testIt("5", 5.0);
}

void simpleInfix(){
    testIt("5 + 2", 7.0);
    testIt("5 < 10", true);
    testIt("5 > 10", false);
    testIt("5 <= 10", true);
    testIt("5 >= 10", false);
    testIt("5 == 5", true);
    testIt("5 != 10", true);
    testIt("5 == 10", false);
    testIt("10 != 10", false);
}

void simpleInfixBool(){
    testIt("5 + 2 < 7", false);
    testIt("5 + 2 <= 7", true);
    testIt("5 + 2 > 7", false);
    testIt("5 + 2 >= 7", true);
    testIt("5 + 2 == 7", true);
    testIt("5 + 2 != 7", false);
    testIt("7 < 5 + 2", false);
    testIt("7 <= 5 + 2", true);
    testIt("7 > 5 + 2", false);
    testIt("7 >= 5 + 2", true);
}

void boolAndLogical(){
    testIt("5 + 2 < 7 && 4 < 8", false);
    testIt("5 + 2 <= 7 && 4 < 8", true);
    testIt("5 + 2 > 7 && 4 < 8", false);
    testIt("5 + 2 >= 7 && 4 < 8", true);
    testIt("5 + 2 == 7 && 4 < 8", true);
    testIt("5 + 2 != 7 && 4 < 8", false);
    testIt("7 < 5 + 2 && 4 < 8", false);
    testIt("7 <= 5 + 2 && 4 < 8", true);
    testIt("7 > 5 + 2 && 4 < 8", false);
    testIt("7 >= 5 + 2 && 4 < 8", true);
    testIt("2.1 < 2.2 && 1000 == 1000 && 6.4 >= 1.1 && 1234 != 2345", true);
    testIt("2.1 < 2.2 && 1000 == 1000 && 6.4 >= 1.1 && 1234 != 2345 || 5 < 2", true);
    testIt("2.1 < 2.2 && 1000 == 1000 && 6.4 >= 1.1 && 1234 != 2345 || 2 < 5", true);
    testIt("2.1 < 2.2 && 1000 == 1000 && 6.4 >= 1.1 && 1234 == 2345 || 2 < 5", true);
    testIt("2.1 < 2.2 && 1000 == 1000 && 6.4 >= 1.1 && 1234 == 2345 || 5 < 2", false);
}

void invalid(){
    // these should fail at the parsing stage already!
    failToParse("5 ! = 3");
    failToParse("5 >");
    failToParse("(5 > 3");
    // this currently works...
    // failToParse("7 < 5 < 3");
}

int main() {
    simple();
    simpleInfix();
    simpleInfixBool();
    boolAndLogical();
    invalid();
}
