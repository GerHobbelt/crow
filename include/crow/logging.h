#pragma once

#include <boost/log/trivial.hpp>

#define CROW_LOG_CRITICAL BOOST_LOG_TRIVIAL(fatal) << "Crow: "
#define CROW_LOG_ERROR BOOST_LOG_TRIVIAL(error) << "Crow: "
#define CROW_LOG_WARNING BOOST_LOG_TRIVIAL(warning) << "Crow: "
#define CROW_LOG_INFO BOOST_LOG_TRIVIAL(info) << "Crow: "
#define CROW_LOG_DEBUG BOOST_LOG_TRIVIAL(debug) << "Crow: "
