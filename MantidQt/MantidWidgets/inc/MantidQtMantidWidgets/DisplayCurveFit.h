#ifndef MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
#define MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
// includes for interface development
#include "ui_DisplayCurveFit.h"
#include "WidgetDllOption.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtMantidWidgets/RangeSelector.h"
// includes for workspace handling
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace MantidWidgets {

// forward declarations
class PreviewPlot;

/** A widget to display fitting , guess, and residuals curves against data.
 *
 * Two PreviewPlot widgets take care of plotting the curves. One for residuals only, and
 * the other for fitting, guess, and data.
 * It is required that all the neccessary data to plot the curves are stored in one
 * or more workspaces. Subscriptions to the Analysis Data Service to receive notifications
 * regarding modification of the workspaces is implemented in the PreviewPlot widgets.

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS DisplayCurveFit
    : public API::MantidWidget {
  Q_OBJECT

public:
  /** only plot curves related to the fitting procedure */
  enum class curveType { data = 1, guess, fit, residuals };
  using curveTypes = std::vector<curveType>;

  /** Ranges on the X-axis */
  enum class fitRange {
    standard = 1, /** range over which the fitting procedure is carried out */
    extended      /** extended range over which the fit and residuals curves are
                     evaluated. Usually extends beyond the boundaries of the standard
                     range */
  };
  DisplayCurveFit(QWidget *parent = nullptr);
  virtual ~DisplayCurveFit();
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
  RangeSelector *
  addRangeSelector(const fitRange &aRange,
                   RangeSelector::SelectType type = RangeSelector::XMINMAX);

  static std::map<curveType, QString> const m_curveTypeToQString;
  static std::map<curveType, Qt::GlobalColor> const m_curveTypeToColor;
  static std::map<fitRange, QString> const m_fitRangeToQString;

  private:
    curveType nameToType(const QString &name) const;
    curveTypes namesToTypes(const QStringList &names) const;
    Ui::DisplayCurveFit m_uiForm;
    // map a curve type onto a PreviewPlot panel
    std::map<curveType, PreviewPlot *> m_plotPanel;
  };
}
}

#endif // MANTID_MANTIDWIDGETS_DISPLAYCURVEFIT_H_
