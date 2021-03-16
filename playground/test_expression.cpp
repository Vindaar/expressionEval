#include "../expression_eval.h"

int main() {
    std::string s = "hitsAna_energy > 5000 + 100 && hitsAna_xy2Sigma < 0.5";
    auto ast = parseExpression(s);
    std::cout << "Final AST: " << astToStr(ast) << std::endl;

    std::map<std::string, float> m = { {"hitsAna_energy", 6000}, {"hitsAna_xy2Sigma", 0.2} };
    std::cout << "Eval = " << evaluate(m, ast).getRight() << std::endl;
}
