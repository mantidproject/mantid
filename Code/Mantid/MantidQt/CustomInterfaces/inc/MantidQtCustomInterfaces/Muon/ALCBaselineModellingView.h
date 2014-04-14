#ifndef MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_

#include "MantidKernel/System.h"

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Muon/IALCBaselineModellingView.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include "ui_ALCBaselineModellingView.h"

#include <qwt_plot_curve.h>

namespace MantidQt
{
namespace CustomInterfaces
{

  using namespace MantidWidgets;

  /** ALCBaselineModellingView : Widget-based implementation of the ALC Baseline Modelling step
                                 interface.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class MANTIDQT_CUSTOMINTERFACES_DLL ALCBaselineModellingView : public IALCBaselineModellingView
  {
    Q_OBJECT

  public:
    ALCBaselineModellingView(QWidget* widget);

    void initialize();

    /// @see IALCBaselineModellingView::function
    IFunction_const_sptr function() const;

    /// @see IALCBaselineModellingView::sections
    std::vector<Section> sections() const;

  public slots:
    /// @see IALCBaselineModellingView::setDataCurve
    void setDataCurve(const QwtData &data);

    /// @see IALCBaselineModellingView::setCorrectedCurve
    void setCorrectedCurve(const QwtData &data);

    /// @see IALCBaselineModellingView::setBaselineCurve
    void setBaselineCurve(const QwtData &data);

    /// @see IALCBaselineModellingView::updateFunction
    void setFunction(IFunction_const_sptr func);

    /// @see IALCBaselineModellingView::setSections
    void setSections(const std::vector<Section>& sections);

    /// @see IALCBaselineModellingView::updateSection
    void updateSection(size_t index, double min, double max);

    /// @see IALCBaselineModellingView::setSectionSelectors
    void setSectionSelectors(const std::vector<SectionSelector> &selectors);

    /// @see IALCBaselineModellingView::updateSectionSelector
    void updateSectionSelector(size_t index, double min, double max);

  private slots:
    /// Show context menu for sections table
    void sectionsContextMenu(const QPoint& widgetPoint);

    /// Called when one of range selectors is modified
    void onRangeSelectorChanged(double min, double max);

    /// Called when one of sections is edited in the table
    void onSectionsTableChanged(int row, int col);

    /// Set section row values in sections table
    void setSectionRow(int row, Section section);

    /// Parse section from one of sections table rows
    Section parseSectionRow(int row) const;

    /// Requests to remove section at specified row
    void requestSectionRemoval(int row);

  private:
    /// Index of section start column in sections table
    static const int SECTION_START_COL = 0;
    /// Index of section end column in sections table
    static const int SECTION_END_COL = 1;

    /// The widget used
    QWidget* const m_widget;

    /// UI form
    Ui::ALCBaselineModellingView m_ui;

    /// Plot curves
    QwtPlotCurve *m_dataCurve, *m_fitCurve, *m_correctedCurve;

    /// Range selectors for sections
    std::vector<RangeSelector*> m_rangeSelectors;
  };


} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTIDQT_CUSTOMINTERFACES_ALCBASELINEMODELLINGVIEW_H_ */
