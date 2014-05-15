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
"        self.m = 'A'\n"
"        self.n = True\n"
"        self.o = set()\n"
"        self.p = dict({1: 1})\n"
"        self.q = 1.0002343\n"
"        self.r = 1000000000L\n"
"        pass\n"
"\n"
"def function(self, *args, **kwargs):\n"
"    i = 1"
"    return i"
"class B(A): pass\n"
"class C(A, B): pass\n"
"\"\"\" Multiline\n"
" ... \n"
"string \"\"\"\n";

        delimit::parser::Python parser;

        auto tokens = parser.tokenize(test_data);

        assert_equal(tokens.at(0).str, "class");

        auto scopes_and_success = parser.parse(test_data, "test.testing");
        auto scopes = scopes_and_success.first;

        for(auto scope: scopes) {
            std::cout << scope->path() << " = " << _u(",").join(scope->inherited_paths()) << std::endl;
        }

        assert_equal("A", scopes.at(0)->path());
        assert_equal("object", scopes.at(0)->inherited_paths().at(0));
        assert_equal(1, scopes.at(0)->start_line);
        assert_equal(15, scopes.at(0)->end_line);
        assert_equal(0, scopes.at(0)->start_col);
        assert_equal(0, scopes.at(0)->end_col);

        assert_equal("A.i", scopes.at(1)->path());
        assert_equal(2, scopes.at(1)->start_line);
        assert_equal(15, scopes.at(1)->end_line);
        assert_equal(6, scopes.at(1)->start_col);
        assert_equal(0, scopes.at(1)->end_col);

        assert_equal("int", scopes.at(1)->inherited_paths().at(0));
        assert_equal("A.j", scopes.at(2)->path());
        assert_equal("list", scopes.at(2)->inherited_paths().at(0));
        assert_equal("A.k", scopes.at(3)->path());
        assert_equal("dict", scopes.at(3)->inherited_paths().at(0));
        assert_equal("A.l", scopes.at(4)->path());
        assert_equal("None", scopes.at(4)->inherited_paths().at(0));
        assert_equal("A.method", scopes.at(5)->path());
        assert_equal("instancemethod", scopes.at(5)->inherited_paths().at(0));
        assert_equal("A.method.self", scopes.at(6)->path());
        assert_equal("A", scopes.at(6)->inherited_paths().at(0));
    }
};

#endif // TEST_PYTHON_PARSER_H
