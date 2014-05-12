#ifndef TEST_PYTHON_PARSER_H
#define TEST_PYTHON_PARSER_H

#include <kaztest/kaztest.h>
#include "../src/autocomplete/parsers/python.h"

class PythonParsingTests : public TestCase {
public:
    void test_parsing_classes() {
        unicode test_data = ""
"class A(object):\n"
"    i = 1\n"
"    j = []\n"
"    k = {}\n"
"    l = None\n"
"    def method(self):\n"
"        pass\n"
"\n"
"def function(self, *args, **kwargs):\n"
"    i = 1"
"    return i"
"class B(A): pass\n"
"class C(A, B): pass\n";

        delimit::parser::Python parser;

        auto tokens = parser.tokenize(test_data);

        assert_equal(tokens.at(0).str, "class");
    }
};

#endif // TEST_PYTHON_PARSER_H
