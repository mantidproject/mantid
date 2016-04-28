#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPER_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtMantidWidgets/IFunctionBrowser.h"
#include "MantidQtMantidWidgets/IMuonFitFunctionControl.h"
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {

/** MuonAnalysisFitFunctionHelper : Updates fit browser from function widget

  Handles interaction between FunctionBrowser widget and fit property browser.
  Implemented as a QObject to handle signals and slots.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL MuonAnalysisFitFunctionHelper : QObject {
  Q_OBJECT
public:
  /// Constructor
  MuonAnalysisFitFunctionHelper(
      QObject *parent,
      MantidQt::MantidWidgets::IMuonFitFunctionControl *fitBrowser,
      MantidQt::MantidWidgets::IFunctionBrowser *funcBrowser);
public slots:
  /// Update function and pass to fit property browser
  void updateFunction();
  /// Update function and pass to fit property browser, then fit
  void updateFunctionAndFit(bool sequential);
  /// When fit finished, update parameters in function browser
  void handleFitFinished(const QString &wsName);
  /// When parameter edited in function browser, update in fit property browser
  void handleParameterEdited(const QString &funcIndex,
                             const QString &paramName);
  /// When "Clear model" selected, clear function browser
  void handleModelCleared();

private:
  /// Connect signals and slots
  void doConnect();
  /// Non-owning pointer to muon fit property browser
  MantidQt::MantidWidgets::IMuonFitFunctionControl *m_fitBrowser;
  /// Non-owning pointer to function browser widget
  MantidQt::MantidWidgets::IFunctionBrowser *m_funcBrowser;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONHELPER_H_ */