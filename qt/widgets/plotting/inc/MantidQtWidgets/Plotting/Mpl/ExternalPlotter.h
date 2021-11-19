// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <boost/none_t.hpp>
#include <optional>

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

enum MantidAxis { Spectrum, Bin };

/**
 @class ExternalPlotter
 ExternalPlotter is a class used for external plotting within Indirect
 */
class EXPORT_OPT_MANTIDQT_PLOTTING ExternalPlotter {

public:
  ExternalPlotter();
  virtual ~ExternalPlotter();

  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars);
  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                           std::optional<QHash<QString, QVariant>> const &kwargs);
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars);
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars,
                                        std::vector<std::optional<QHash<QString, QVariant>>> const &kwargs);
  virtual void plotBins(std::string const &workspaceName, std::string const &binIndices, bool errorBars);
  virtual void plotContour(std::string const &workspaceName);
  virtual void plotTiled(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars);

  bool validate(std::string const &workspaceName, std::optional<std::string> const &workspaceIndices = std::nullopt,
                std::optional<MantidAxis> const &axisType = std::nullopt) const;

private:
  bool validate(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                std::optional<std::string> const &workspaceIndices = std::nullopt,
                std::optional<MantidAxis> const &axisType = std::nullopt) const;
  bool validateSpectra(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                       std::string const &workspaceIndices) const;
  bool validateBins(const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &binIndices) const;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
