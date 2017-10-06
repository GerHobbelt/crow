#include "../amalgamate/crow_all.h"

namespace crow
{
namespace log
{

bool doCrowLoggingForLevel(BoostLogLevel)
{
  return true;
}
BoostLogLevel getLogLevelForCrowLogLevel(BoostLogLevel crowLogLevel)
{
  return crowLogLevel;
}

} // namespace log
} // namespace crow
