#include "session_storage.h"
#include "session.h"
#include "join_server.h"
#include "functions.h"


#define BOOST_TEST_MODULE test_main

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_main_1)


BOOST_AUTO_TEST_CASE(invalid_port)
{  
    BOOST_CHECK_THROW(process( "-1"    ), std::invalid_argument);
    BOOST_CHECK_THROW(process( "45as"  ), std::invalid_argument);
    BOOST_CHECK_THROW(process( "http"  ), std::invalid_argument);
    BOOST_CHECK_THROW(process( "65536" ), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

