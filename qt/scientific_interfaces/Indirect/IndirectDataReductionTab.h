#ifndef MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_

#include "IndirectTab.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/IndirectInstrumentConfig.h"
#include "MantidQtWidgets/Common/PythonRunner.h"
#include "MantidQtWidgets/LegacyQwt/QwtWorkspaceSpectrumData.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include <MantidQtWidgets/Common/QtPropertyBrowser/QtIntPropertyManager>
#include <MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser>
#include <QDoubleValidator>
#include <QMap>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
class IndirectDataReduction;

/** IndirectDataReductionTab

  This class defines common functionality of tabs used in the Indirect Data
  Reduction interface.

  @author Samuel Jackson
  @date 13/08/2013

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IndirectDataReductionTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectDataReductionTab(IndirectDataReduction *idrUI,
                           QObject *parent = nullptr);
  ~IndirectDataReductionTab() override;

public slots:
  void runTab();

signals:
  /// Update the Run button on the IDR main window
  void updateRunButton(bool enabled = true, QString message = "Run",
                       QString tooltip = "");
  /// Emitted when the instrument setup is changed
  void newInstrumentConfiguration();

protected:
  Mantid::API::MatrixWorkspace_sptr
  loadInstrumentIfNotExist(std::string instrumentName,
                           std::string analyser = "",
                           std::string reflection = "");
  /// Function to get details about the instrumet from a given workspace
  QMap<QString, QString> getInstrumentDetails() const;
  std::map<std::string, double>
  getRangesFromInstrument(QString instName = "", QString analyser = "",
                          QString reflection = "");
  /// Get the instrument config widget
  MantidWidgets::IndirectInstrumentConfig *getInstrumentConfiguration() const;

private slots:
  void tabExecutionComplete(bool error);

private:
  IndirectDataReduction *m_idrUI;
  bool m_tabRunning;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTDATAREDUCTIONTAB_H_ */
