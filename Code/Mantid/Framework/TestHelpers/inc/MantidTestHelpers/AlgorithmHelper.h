#ifndef MANTID_TESTHELPERS_ALGORITHMHELPER_H_
#define MANTID_TESTHELPERS_ALGORITHMHELPER_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidTestHelpers/DLLExport.h"



/** AlgorithmHelper : Namespace containing methods for more easily creating algorithms
 * and running them, for tests.
 *
 * @author Janik Zikovsky
 * @date 2011-04-12 10:50:12.977084
 */
namespace AlgorithmHelper
{
DLL_TESTHELPERS Mantid::API::Algorithm_sptr runAlgorithm(std::string algorithmName, int count, ...);

};


#endif  /* MANTID_TESTHELPERS_ALGORITHMHELPER_H_ */
