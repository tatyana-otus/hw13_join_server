#include <fstream>

#define BOOST_TEST_MODULE test_main

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_main_1)
BOOST_AUTO_TEST_CASE(send_cmd)
{
    std::system("rm -f out.txt in.txt");

    {
        std::string data = 
        "INTERSECTION\n"
        "SYMMETRIC_DIFFERENCE\n"
        "INSERT A 0 a\n"
        "INSERT A 0 b\n"
        "INSERT A 1 b\n"
        "INSERT A 2 c\n"
        "INSERT B 3 dd\n"
        "INSERT B 4 ee\n"
        "INSERT B 5 gg\n"
        "INTERSECTION\n"
        "SYMMETRIC_DIFFERENCE\n"
        "INSERT B 1 bb\n"
        "INSERT B 0 aa\n"
        "INTERSECTION\n"
        "TRUNCATE A\n"
        "INTERSECTION\n"
        "SYMMETRIC_DIFFERENCE\n"
        "TRUNCATE B\n"
        "INTERSECTION\n"
        "SYMMETRIC_DIFFERENCE\n";

        auto f = std::ofstream("in.txt");
        std::stringstream ss;
        ss << data;
        f << ss.rdbuf();
    }

    std::system("chmod 755 ../../tests/sh/send_cmds.sh");
    std::system("../../tests/sh/send_cmds.sh");

    {
        std::string data = 
        "OK\nOK\nOK\n"
        "ERR duplicate 0\n"
        "OK\nOK\nOK\nOK\nOK\nOK\n"
        "0,a,\n"
        "1,b,\n"
        "2,c,\n"
        "3,,dd\n"
        "4,,ee\n"
        "5,,gg\n"
        "OK\n"
        "OK\nOK\n"
        "0,a,aa\n"
        "1,b,bb\n"
        "OK\n"
        "OK\nOK\n"
        "0,,aa\n"
        "1,,bb\n"
        "3,,dd\n"
        "4,,ee\n"
        "5,,gg\n"
        "OK\n"
        "OK\nOK\nOK\n";
        auto f = std::ifstream("out.txt");
        std::stringstream ss;
        ss << f.rdbuf();
        BOOST_CHECK_EQUAL(ss.str(), data);
    }
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_suite_main_2)
BOOST_AUTO_TEST_CASE(send_invalid_cmd)
{
    std::system("rm -f out.txt in.txt");

    {
        std::string data = 
        "INTERSECTION A\n"
        "SYMMETRIC_DIFFERENCE B\n"
        "INSERT B 1 bb vv\n"
        "INSERT C 0 aa\n"
        "INSERT A XXX aa\n"
        "INSERT A 5XXX aa\n"
        "INSERT A 5+8 aa\n"
        "TRUNCATE A A\n"
        "TRUNCATE C\n"
        "qwqwqwq\n";

        auto f = std::ofstream("in.txt");
        std::stringstream ss;
        ss << data;
        f << ss.rdbuf();
    }

    std::system("chmod 755 ../../tests/sh/send_cmds.sh");
    std::system("../../tests/sh/send_cmds.sh");

    {
        std::string data = 
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n"
        "ERR wrong command\n";
        auto f = std::ifstream("out.txt");
        std::stringstream ss;
        ss << f.rdbuf();
        BOOST_CHECK_EQUAL(ss.str(), data);
    }
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_suite_main_3)
BOOST_AUTO_TEST_CASE(send_multy_cmd)
{
    std::system("rm -f intersec.txt intersec.count sym_diff.txt sym_diff.count");

    std::system("chmod 755 ../../tests/sh/gen_insert_A.sh");
    std::system("chmod 755 ../../tests/sh/gen_insert_B.sh");

    std::system("../join_server 9001 &");
    std::system("sleep 1");

    std::system("seq 1  1000 | xargs -n1 -P100 ../../tests/sh/gen_insert_A.sh > /dev/null");
    std::system("seq 201 800 | xargs -n1 -P100 ../../tests/sh/gen_insert_B.sh > /dev/null");

    std::system("echo \"INTERSECTION\" | nc -q 1 localhost 9001 > intersec.txt");
    std::system("echo \"SYMMETRIC_DIFFERENCE\" | nc localhost -q 1 9001 > sym_diff.txt");

    std::system("pkill join_server");

    std::system("grep -ocE '[0-9]+' intersec.txt > intersec.count");
    std::system("grep -ocE '[0-9]+' sym_diff.txt > sym_diff.count");

    {
        auto f = std::ifstream("intersec.count");
        std::stringstream ss;
        ss << f.rdbuf();
        BOOST_CHECK_EQUAL(ss.str(), "600\n");
    }

    {
        auto f = std::ifstream("sym_diff.count");
        std::stringstream ss;
        ss << f.rdbuf();
        BOOST_CHECK_EQUAL(ss.str(), "400\n");
    }
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_suite_main_4)
BOOST_AUTO_TEST_CASE(send_cmd_get_tables)
{
    std::system("chmod 755 ../../tests/sh/gen_insert_A.sh");
    std::system("chmod 755 ../../tests/sh/gen_insert_B.sh");
    std::system("chmod 755 ../../tests/sh/read_tables.sh");
    std::system("chmod 755 ../../tests/sh/test_send_cmd_read_table.sh");

    std::system("../../tests/sh/test_send_cmd_read_table.sh");

    std::ifstream f("OK");
    BOOST_CHECK_EQUAL( f.good(), true );
}

BOOST_AUTO_TEST_SUITE_END()