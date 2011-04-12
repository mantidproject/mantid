#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"
#include <cstdarg>
#include <cxxtest/TestSuite.h>

namespace AlgorithmHelper
{

  /** Run any algorithm with a variable number of parameters
   *
   * @param algorithmName
   * @param count :: number of arguments given.
   * @param variable number of extra arguments, all string.
   * @return
   */
  Mantid::API::Algorithm_sptr runAlgorithm(std::string algorithmName, int count, ...)
  {
    // Create the algorithm
    Mantid::API::Algorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING( alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algorithmName, -1) );
    TS_ASSERT_THROWS_NOTHING( alg->initialize() )
    TS_ASSERT( alg->isInitialized() )

    if (count % 2 == 1)
    {
      TSM_ASSERT("Must have an even number of parameter/value string arguments", false);
      return alg;
    }

    va_list Params;
    va_start(Params, count);
    for(int i = 0; i < count; i += 2 )
    {
      std::string paramName = va_arg(Params, const char *);
      std::string paramValue = va_arg(Params, const char *);
      TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue(paramName, paramValue) );
    }
    va_end(Params);

    TS_ASSERT_THROWS_NOTHING( alg->execute() );
    return alg;
  }



} //namespace

