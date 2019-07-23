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

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class PythonRunner;
#endif

/**
 @class IndirectPlotter
 IndirectPlotter is a class used for external plotting within Indirect
 */
class MANTIDQT_INDIRECT_DLL IndirectPlotter : public QObject {
  Q_OBJECT

public:
  IndirectPlotter(IndirectTab *parent);

  void plotSpectra(std::string const &workspaceName,
                   std::string const &workspaceIndices);
  void plotBins(std::string const &workspaceName,
                std::string const &binIndices);
  void plotContour(std::string const &workspaceName);
  void plotTiled(std::string const &workspaceName,
                 std::string const &workspaceIndices);

private:
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  void runPythonCode(std::string const &pythonCode);

  API::PythonRunner m_pythonRunner;
#endif
  IndirectTab *m_parentTab;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_INDIRECTPLOTTER_H */
