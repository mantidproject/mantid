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
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::MantidWidgets;

class MockInstrumentActor : public IInstrumentActor {
public:
  MOCK_CONST_METHOD1(draw, void(bool picking));
  MOCK_CONST_METHOD3(getBoundingBox,
                     void(Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound, const bool excludeMonitors));
  MOCK_CONST_METHOD0(getInstrument, std::shared_ptr<const Mantid::Geometry::Instrument>());
  MOCK_CONST_METHOD0(getWorkspace, std::shared_ptr<const Mantid::API::MatrixWorkspace>());
  MOCK_CONST_METHOD0(componentInfo, const Mantid::Geometry::ComponentInfo &());
  MOCK_CONST_METHOD0(detectorInfo, const Mantid::Geometry::DetectorInfo &());
  MOCK_CONST_METHOD1(getColor, GLColor(size_t index));
  MOCK_CONST_METHOD0(minBinValue, double());
  MOCK_CONST_METHOD0(maxBinValue, double());
  MOCK_CONST_METHOD0(ndetectors, std::size_t());
  MOCK_CONST_METHOD1(getDetID, Mantid::detid_t(std::size_t pickID));
  MOCK_CONST_METHOD1(getDetPos, const Mantid::Kernel::V3D(std::size_t pickID));
  MOCK_CONST_METHOD1(getWorkspaceIndex, std::size_t(std::size_t index));
  MOCK_CONST_METHOD3(getBinMinMaxIndex, void(std::size_t wi, std::size_t &imin, std::size_t &imax));
  MOCK_CONST_METHOD1(getIntegratedCounts, double(std::size_t index));
  MOCK_CONST_METHOD0(components, const std::vector<size_t> &());
  MOCK_CONST_METHOD0(getInstrumentRenderer, const InstrumentRenderer &());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
