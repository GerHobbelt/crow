#pragma once

#include <boost/log/trivial.hpp>

namespace crow
{
namespace log
{

using BoostLogLevel = boost::log::trivial::severity_level;

bool doCrowLoggingForLevel(BoostLogLevel crowLogLevel);
BoostLogLevel getLogLevelForCrowLogLevel(BoostLogLevel crowLogLevel);

} // namespace log
} // namespace crow

#define CROW_LOG(level) \
  if(crow::log::doCrowLoggingForLevel(::boost::log::trivial::level)) \
    BOOST_LOG_STREAM_WITH_PARAMS( \
      ::boost::log::trivial::logger::get() \
      ,(::boost::log::keywords::severity = crow::log::getLogLevelForCrowLogLevel(::boost::log::trivial::level)) \
    ) << "Crow: "

#define CROW_LOG_CRITICAL CROW_LOG(fatal)
#define CROW_LOG_ERROR CROW_LOG(error)
#define CROW_LOG_WARNING CROW_LOG(warning)
#define CROW_LOG_INFO CROW_LOG(info)
#define CROW_LOG_DEBUG CROW_LOG(debug)
