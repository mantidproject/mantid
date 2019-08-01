// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTPLOTTER_H
#define MANTID_INDIRECTPLOTTER_H

#include "IndirectTab.h"

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/none_t.hpp>
#include <boost/optional.hpp>

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class PythonRunner;
#endif

enum MantidAxis { Spectrum, Bin };

/**
 @class IndirectPlotter
 IndirectPlotter is a class used for external plotting within Indirect
 */
class MANTIDQT_INDIRECT_DLL IndirectPlotter : public QObject {
  Q_OBJECT

public:
  IndirectPlotter(IndirectTab *parent = nullptr);
  virtual ~IndirectPlotter();

  virtual void plotSpectra(std::string const &workspaceName,
                           std::string const &workspaceIndices);
  virtual void plotBins(std::string const &workspaceName,
                        std::string const &binIndices);
  virtual void plotContour(std::string const &workspaceName);
  virtual void plotTiled(std::string const &workspaceName,
                         std::string const &workspaceIndices);

  bool
  validate(std::string const &workspaceName,
           boost::optional<std::string> const &workspaceIndices = boost::none,
           boost::optional<MantidAxis> const &axisType = boost::none) const;

private:
  bool
  validate(Mantid::API::MatrixWorkspace_const_sptr workspace,
           boost::optional<std::string> const &workspaceIndices = boost::none,
           boost::optional<MantidAxis> const &axisType = boost::none) const;
  bool validateSpectra(Mantid::API::MatrixWorkspace_const_sptr workspace,
                       std::string const &workspaceIndices) const;
  bool validateBins(Mantid::API::MatrixWorkspace_const_sptr workspace,
                    std::string const &binIndices) const;

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  void runPythonCode(std::string const &pythonCode);

  API::PythonRunner m_pythonRunner;
#endif
  IndirectTab *m_parentTab;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_INDIRECTPLOTTER_H */
