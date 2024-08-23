// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <gmock/gmock.h>

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

#include <optional>
#include <string>

using namespace MantidQt::Widgets::MplCpp;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockExternalPlotter : public IExternalPlotter {
public:
  virtual ~MockExternalPlotter() = default;

  MOCK_METHOD3(plotSpectra,
               void(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars));
  MOCK_METHOD4(plotSpectra, void(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                                 std::optional<QHash<QString, QVariant>> const &kwargs));
  MOCK_METHOD3(plotCorrespondingSpectra,
               void(std::vector<std::string> const &workspaceNames, std::vector<int> const &workspaceIndices,
                    std::vector<bool> const &errorBars));
  MOCK_METHOD4(plotCorrespondingSpectra,
               void(std::vector<std::string> const &workspaceNames, std::vector<int> const &workspaceIndices,
                    std::vector<bool> const &errorBars,
                    std::vector<std::optional<QHash<QString, QVariant>>> const &kwargs));
  MOCK_METHOD3(plotBins, void(std::string const &workspaceName, std::string const &binIndices, bool errorBars));
  MOCK_METHOD1(showSliceViewer, void(std::string const &workspaceName));
  MOCK_METHOD1(plot3DSurface, void(std::string const &workspaceName));
  MOCK_METHOD3(plotTiled, void(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars));
  MOCK_METHOD1(plotContour, void(std::string const &workspaceName));
  MOCK_CONST_METHOD3(validate,
                     bool(std::string const &workspaceName, std::optional<std::string> const &workspaceIndices,
                          std::optional<MantidAxis> const &axisType));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
