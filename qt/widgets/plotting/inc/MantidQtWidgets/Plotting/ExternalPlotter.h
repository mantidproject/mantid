// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <optional>

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

enum MantidAxis { Spectrum, Bin, Both };

class EXPORT_OPT_MANTIDQT_PLOTTING IExternalPlotter {
public:
  virtual ~IExternalPlotter() = default;

  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars) = 0;
  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                           std::optional<QHash<QString, QVariant>> const &kwargs) = 0;
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices,
                                        std::vector<bool> const &errorBars) = 0;
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars,
                                        std::vector<std::optional<QHash<QString, QVariant>>> const &kwargs) = 0;
  virtual void plotBins(std::string const &workspaceName, std::string const &binIndices, bool errorBars) = 0;
  virtual void plotContour(std::string const &workspaceName) = 0;
  virtual void plotTiled(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars) = 0;
  virtual void plot3DSurface(std::string const &workspaceName) = 0;
  virtual void showSliceViewer(std::string const &workspaceName) = 0;
  virtual bool validate(std::string const &workspaceName, std::optional<std::string> const &workspaceIndices,
                        std::optional<MantidAxis> const &axisType) const = 0;
};

class EXPORT_OPT_MANTIDQT_PLOTTING ExternalPlotter final : public IExternalPlotter {

public:
  void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars) override;
  void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                   std::optional<QHash<QString, QVariant>> const &kwargs) override;
  void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars) override;
  void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                std::vector<int> const &workspaceIndices, std::vector<bool> const &errorBars,
                                std::vector<std::optional<QHash<QString, QVariant>>> const &kwargs) override;
  void plotBins(std::string const &workspaceName, std::string const &binIndices, bool errorBars) override;
  void plotContour(std::string const &workspaceName) override;
  void plotTiled(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars) override;
  void plot3DSurface(std::string const &workspaceName) override;
  void showSliceViewer(std::string const &workspaceName) override;
  bool validate(std::string const &workspaceName, std::optional<std::string> const &workspaceIndices = std::nullopt,
                std::optional<MantidAxis> const &axisType = std::nullopt) const override;

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
