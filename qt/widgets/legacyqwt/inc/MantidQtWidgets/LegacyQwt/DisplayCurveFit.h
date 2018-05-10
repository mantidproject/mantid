#ifndef MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
#define MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
// includes for interface development
#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"
#include "ui_DisplayCurveFit.h"
// includes for workspace handling
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace MantidWidgets {

// forward declarations
class PreviewPlot;
class RangeSelector;

/** A widget to display the results of a curve fitting. Its main features:
 * - An upper panel to plot the data curve, the evaluation of the model with
 *   current model parameters (guess curve), and the evaluation of the
 *   model with optimized parameters (fit curve).
 * - A lower panel to plot the residuals curve, the difference between
 *   the data curve  and the fit curve.
 * - A range limited by two vertical lines over which the fit should
 *   be carried out (fit-range).
 * - A range over which the model is evaluated (evaluate-range). Sometimes
 *   one may wish to evaluate the model over a range slightly bigger than
 *   the range over which the fit is carried out.
 *
 * All curves to be plotted need to be stored in workspaces. The
 * AnalysisDataService notifies DisplayCurveFit of changes of these
 * workspaces.

  @date 2016-02-11

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_LEGACYQWT DisplayCurveFit : public API::MantidWidget {
  Q_OBJECT

public:
  /** only plot curves related to the fitting procedure */
  enum class curveType { data = 1, guess, fit, residuals };
  using curveTypes = std::vector<curveType>;

  /** Ranges on the X-axis */
  enum class dcRange {
    fit = 1, ///< range over which the fitting procedure is carried out.
    evaluate ///< range over which the fit and residuals curves are evaluated.
    /// Usually extends beyond the boundaries of the fit range.
  };
  DisplayCurveFit(QWidget *parent = nullptr);
  ~DisplayCurveFit() override;
  void setAxisRange(QPair<double, double> range, int axisID = QwtPlot::xBottom);
  curveTypes
  getCurvesForWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace);
  QPair<double, double> getCurveRange(const curveType &atype);
  QPair<double, double>
  getCurveRange(const Mantid::API::MatrixWorkspace_sptr workspace);
  void addSpectrum(const curveType &aType,
                   const Mantid::API::MatrixWorkspace_sptr workspace,
                   const size_t specIndex = 0);
  void removeSpectrum(const curveType &aType);
  bool hasCurve(const curveType &aType);
  void
  addRangeSelector(const dcRange &adcRange,
                   RangeSelector::SelectType type = RangeSelector::XMINMAX);
  void addResidualsZeroline();
  static std::map<curveType, QString> const m_curveTypeToQString;
  static std::map<curveType, Qt::GlobalColor> const m_curveTypeToColor;
  static std::map<dcRange, QString> const m_dcRangeToQString;

  /// Pointers to the RangeSelector objects.
  std::map<dcRange, RangeSelector *> m_rangeSelector;

private:
  curveType nameToType(const QString &name) const;
  curveTypes namesToTypes(const QStringList &names) const;
  /// Object holding the widgets defined in the form created in Qt-designer
  Ui::DisplayCurveFit m_uiForm;
  // maps a curve type onto one of the two PreviewPlot panels
  std::map<curveType, PreviewPlot *> m_plotPanel;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
