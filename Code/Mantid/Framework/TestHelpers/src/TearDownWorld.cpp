//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidTestHelpers/TearDownWorld.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

namespace
{
  /// Define single ClearAlgorithmManager object
  ClearAlgorithmManager clearAlgManager;
  /// Definition of ClearADS object;
  ClearADS clearADS;
}

//-----------------------------------------------------------------------------
// ClearAlgorithmManager
//-----------------------------------------------------------------------------
/// @return True to indicate success of the tear down process
bool ClearAlgorithmManager::tearDownWorld()
{
  Mantid::API::AlgorithmManager::Instance().clear();
  return true;
}


//-----------------------------------------------------------------------------
// ClearADS
//-----------------------------------------------------------------------------

/// @return True to indicate success of the tear down process
bool ClearADS::tearDownWorld()
{
  Mantid::API::AnalysisDataService::Instance().clear();
  return true;
}
