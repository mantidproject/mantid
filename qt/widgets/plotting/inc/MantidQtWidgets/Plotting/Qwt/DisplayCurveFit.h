// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
#define MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/Qwt/RangeSelector.h"
#include "ui_DisplayCurveFit.h"

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
*/
class EXPORT_OPT_MANTIDQT_PLOTTING DisplayCurveFit : public API::MantidWidget {
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
  void setAxisRange(QPair<double, double> range,
                    AxisID axisID = AxisID::XBottom);
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
