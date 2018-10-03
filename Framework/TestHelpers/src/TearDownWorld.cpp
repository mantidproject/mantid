// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidTestHelpers/TearDownWorld.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/PropertyManagerDataService.h"

// On MSVC all workspaces must be deleted by the time main() exits as the
// Workspace destruction can call to an OpenMP loop which is not allowed
// on MSVC after main() exits.
// See Workspace2D::~Workspace2D()

namespace {
/// Define single ClearAlgorithmManager object
ClearAlgorithmManager clearAlgManager;
/// Definition of ClearADS object;
ClearADS clearADS;
/// Definition of single ClearPropertyManagerDataService object
ClearPropertyManagerDataService clearPropSvc;
} // namespace

//-----------------------------------------------------------------------------
// ClearAlgorithmManager
//-----------------------------------------------------------------------------
/// @return True to indicate success of the tear down process
bool ClearAlgorithmManager::tearDownWorld() {
  Mantid::API::AlgorithmManager::Instance().clear();
  return true;
}

//-----------------------------------------------------------------------------
// ClearADS
//-----------------------------------------------------------------------------

/// @return True to indicate success of the tear down process
bool ClearADS::tearDownWorld() {
  Mantid::API::AnalysisDataService::Instance().clear();
  return true;
}

//-----------------------------------------------------------------------------
// ClearPropertyManagerDataService
//-----------------------------------------------------------------------------

/// @return True to indicate success of the tear down process
bool ClearPropertyManagerDataService::tearDownWorld() {
  Mantid::Kernel::PropertyManagerDataService::Instance().clear();
  return true;
}
