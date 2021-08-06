#pragma once

namespace crow {

#if defined(CROW_MAIN) && !defined(BUILD_MONOLITHIC)
  char VERSION[] = "master";
#else
  extern char VERSION[];
#endif
}
