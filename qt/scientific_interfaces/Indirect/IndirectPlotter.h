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

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

enum MantidAxis { Spectrum, Bin };

/**
 @class IndirectPlotter
 IndirectPlotter is a class used for external plotting within Indirect
 */
class MANTIDQT_INDIRECT_DLL IndirectPlotter : public QObject {
  Q_OBJECT

public:
  IndirectPlotter();
  virtual ~IndirectPlotter();

  virtual void plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices);
  virtual void plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                        std::vector<int> const &workspaceIndices);
  virtual void plotBins(std::string const &workspaceName, std::string const &binIndices);
  virtual void plotContour(std::string const &workspaceName);
  virtual void plotTiled(std::string const &workspaceName, std::string const &workspaceIndices);

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
