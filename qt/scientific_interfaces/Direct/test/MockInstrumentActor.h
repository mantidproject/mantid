// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets;

class MockInstrumentActor : public IInstrumentActor {
public:
  MOCK_CONST_METHOD0(getInstrument, std::shared_ptr<const Mantid::Geometry::Instrument>());
  MOCK_CONST_METHOD0(getWorkspace, std::shared_ptr<const Mantid::API::MatrixWorkspace>());
  MOCK_CONST_METHOD0(componentInfo, const Mantid::Geometry::ComponentInfo &());
  MOCK_CONST_METHOD0(detectorInfo, const Mantid::Geometry::DetectorInfo &());

  MOCK_CONST_METHOD1(getWorkspaceIndex, std::size_t(std::size_t index));
  MOCK_CONST_METHOD3(getBinMinMaxIndex, void(std::size_t wi, std::size_t &imin, std::size_t &imax));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
