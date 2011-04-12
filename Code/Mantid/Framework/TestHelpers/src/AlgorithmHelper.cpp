#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/System.h"
#include <cstdarg>

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
    alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algorithmName, -1);
    alg->initialize();
    if (!alg->isInitialized())
      throw std::runtime_error(algorithmName + " was not initialized.");

    if (count % 2 == 1)
    {
      throw std::runtime_error("Must have an even number of parameter/value string arguments");
    }

    va_list Params;
    va_start(Params, count);
    for(int i = 0; i < count; i += 2 )
    {
      std::string paramName = va_arg(Params, const char *);
      std::string paramValue = va_arg(Params, const char *);
      alg->setPropertyValue(paramName, paramValue);
    }
    va_end(Params);

    alg->execute();
    return alg;
  }



} //namespace

