// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

enum MantidAxis { Spectrum, Bin };

/**
 @class ExternalPlotter
 ExternalPlotter is a class used for external plotting within Indirect
 */
class MANTIDQT_MUONINTERFACE_DLL ExternalPlotter {

public:
  ExternalPlotter();
  virtual ~ExternalPlotter();

  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                           boost::optional<QHash<QString, QVariant>> kwargs = boost::none);
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices, bool errorBars);
  virtual void plotBins(std::string const &workspaceName, std::string const &binIndices, bool errorBars);
  virtual void plotContour(std::string const &workspaceName);
  virtual void plotTiled(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars);

  bool validate(std::string const &workspaceName, boost::optional<std::string> const &workspaceIndices = boost::none,
                boost::optional<MantidAxis> const &axisType = boost::none) const;

private:
  bool validate(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                boost::optional<std::string> const &workspaceIndices = boost::none,
                boost::optional<MantidAxis> const &axisType = boost::none) const;
  bool validateSpectra(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                       std::string const &workspaceIndices) const;
  bool validateBins(const Mantid::API::MatrixWorkspace_const_sptr &workspace, std::string const &binIndices) const;
};

} // namespace CustomInterfaces
} // namespace MantidQt
