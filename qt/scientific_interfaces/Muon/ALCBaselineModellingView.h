#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_

#include "MantidKernel/System.h"

#include "DllConfig.h"
#include "IALCBaselineModellingView.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "ui_ALCBaselineModellingView.h"

#include <qwt_plot_curve.h>

namespace MantidQt {
namespace MantidWidgets {
class ErrorCurve;
}
} // namespace MantidQt

namespace MantidQt {
namespace CustomInterfaces {

/** ALCBaselineModellingView : Widget-based implementation of the ALC Baseline
  Modelling step
                               interface.

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTIDQT_MUONINTERFACE_DLL ALCBaselineModellingView
    : public IALCBaselineModellingView {
  Q_OBJECT

public:
  ALCBaselineModellingView(QWidget *widget);
  ~ALCBaselineModellingView() override;

  // -- IALCBaselineModellingView interface
  // --------------------------------------------------------
public:
  QString function() const override;
  SectionRow sectionRow(int row) const override;
  SectionSelector sectionSelector(int index) const override;
  int noOfSectionRows() const override;
  void emitFitRequested();

public slots:
  void initialize() override;
  void setDataCurve(const QwtData &data,
                    const std::vector<double> &errors) override;
  void setCorrectedCurve(const QwtData &data,
                         const std::vector<double> &errors) override;
  void setBaselineCurve(const QwtData &data) override;
  void setFunction(Mantid::API::IFunction_const_sptr func) override;
  void setNoOfSectionRows(int rows) override;
  void setSectionRow(int row, SectionRow values) override;
  void addSectionSelector(int index, SectionSelector values) override;
  void deleteSectionSelector(int index) override;
  void updateSectionSelector(int index, SectionSelector values) override;
  void displayError(const QString &message) override;
  void help() override;
  // -- End of IALCBaselineModellingView interface
  // -------------------------------------------------

private slots:
  /// Show context menu for sections table
  void sectionsContextMenu(const QPoint &widgetPoint);

private:
  /// Helper to set range selector values
  void setSelectorValues(MantidWidgets::RangeSelector *selector,
                         SectionSelector values);

  /// The widget used
  QWidget *const m_widget;

  /// UI form
  Ui::ALCBaselineModellingView m_ui;

  /// Plot curves
  QwtPlotCurve *m_dataCurve, *m_fitCurve, *m_correctedCurve;

  /// Error curves
  MantidQt::MantidWidgets::ErrorCurve *m_dataErrorCurve, *m_correctedErrorCurve;

  /// Range selectors
  std::map<int, MantidWidgets::RangeSelector *> m_rangeSelectors;

  QSignalMapper *m_selectorModifiedMapper;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_ */
