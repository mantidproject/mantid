// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/IJobManager.h"
#include "IPreviewModel.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"

#include <gmock/gmock.h>

#include <string>

using Mantid::API::MatrixWorkspace_sptr;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewModel : public IPreviewModel {
public:
  MOCK_METHOD(bool, loadWorkspaceFromAds, (std::string const &workspaceName), (override));
  MOCK_METHOD(void, loadAndPreprocessWorkspaceAsync, (std::string const &, IJobManager &), (override));
  MOCK_METHOD(MatrixWorkspace_sptr, getLoadedWs, (), (const, override));
  MOCK_METHOD(MatrixWorkspace_sptr, getSummedWs, (), (const, override));
  MOCK_METHOD(std::vector<Mantid::detid_t>, getSelectedBanks, (), (const, override));

  MOCK_METHOD(void, setSelectedBanks, (std::vector<Mantid::detid_t>), (override));
  MOCK_METHOD(void, setSelectedRegion, (Selection const &), (override));

  MOCK_METHOD(void, sumBanksAsync, (IJobManager &), (override));
  MOCK_METHOD(void, reduceAsync, (IJobManager &), (override));
  MOCK_METHOD(std::string, detIDsToString, (std::vector<Mantid::detid_t> const &), (const, override));
  MOCK_METHOD(void, exportSummedWsToAds, (), (const, override));
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
