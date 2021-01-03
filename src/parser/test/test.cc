#include "../parser.h"
#include "../../lexer/lexer.h"
#include <iostream>
#include <string>
#include <memory>
using namespace std;

int main() {
    std::unique_ptr<Parser> parser = std::make_unique<Parser>("test.txt");
    parser->Scan();
    return 0;
}
