#include "../lexer.h"
#include <iostream>
#include <string>
using namespace std;

int main() {
    Lexer *lexer = new Lexer("test.txt");
    lexer->Tokenize();
    lexer->PrintTokenList();
    return 0;
}
