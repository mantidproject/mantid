// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPreviewInstrumentDisplay.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"

#include <gmock/gmock.h>

#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MockPreviewInstrumentDisplay : public IPreviewInstrumentDisplay {
public:
  MOCK_METHOD(void, updateWorkspace, (Mantid::API::MatrixWorkspace_sptr &), (override));
  MOCK_METHOD(void, resetInstView, (), (override));
  MOCK_METHOD(void, plotInstView, (), (override));
  MOCK_METHOD(void, setInstViewZoomMode, (), (override));
  MOCK_METHOD(void, setInstViewEditMode, (), (override));
  MOCK_METHOD(void, setInstViewSelectRectMode, (), (override));
  MOCK_METHOD(std::vector<size_t>, getSelectedDetectors, (), (const, override));
  MOCK_METHOD(std::vector<Mantid::detid_t>, detIndicesToDetIDs, (std::vector<size_t> const &), (const, override));
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
