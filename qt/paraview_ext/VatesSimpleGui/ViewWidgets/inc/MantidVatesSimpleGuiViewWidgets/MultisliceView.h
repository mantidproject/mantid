#ifndef MULTISLICEVIEW_H_
#define MULTISLICEVIEW_H_

#include "MantidKernel/VMD.h"
#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include "ui_MultisliceView.h"

#include <QPointer>
#include <QWidget>

class pqMultiSliceView;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

class RebinnedSourcesManager;
/**
 *
  This class uses the MultiSliceView created by Kitware based on our
  specifications.

  @date 24/05/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS MultiSliceView
    : public ViewBase {
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget of the multislice view widget
   * @param rebinnedSourcesManager Pointer to a RebinnedSourcesManager
   * @param createRenderProxy :: Whether to create a render proxy for this view
   */
  MultiSliceView(QWidget *parent = nullptr,
                 RebinnedSourcesManager *rebinnedSourcesManager = nullptr,
                 bool createRenderProxy = true);
  /// Default constructor.
  ~MultiSliceView() override;

  /// ViewBase::closeSubWindows
  void closeSubWindows() override;
  /**
   * ViewBase::destroyView
   */
  void destroyView() override;
  /**
   * ViewBase::getView
   */
  pqRenderView *getView() override;
  /**
   * ViewBase::render
   */
  void render() override;
  /**
   * ViewBase::renderAll
   */
  void renderAll() override;
  /// ViewBase::resetCamera()
  void resetCamera() override;
  /**
   * ViewBase::resetDisplay()
   */
  void resetDisplay() override;

  /// @see ViewBase::setView
  void setView(pqRenderView *view) override;
  /// @see ViewBase::getViewType
  ModeControlWidget::Views getViewType() override;

protected slots:
  /// Determine if slice is to be shown in SliceViewer.
  void checkSliceClicked(int axisIndex, double sliceOffsetOnAxis, int button,
                         int modifier);
  /// Launch SliceViewer with the specified cut.
  void showCutInSliceViewer(int axisIndex, double sliceOffsetOnAxis);

  //// changes the slice point in VATES.
  void changedSlicePoint(Mantid::Kernel::VMD selectedPoint);

  void setSlicePosition();

  void checkState(const QString &input);

private:
  Q_DISABLE_COPY(MultiSliceView)
  void editSlicePosition(int axisIndex, double sliceOffsetOnAxis);
  /// Determine if the data can support the SliceViewer being shown.
  void checkSliceViewCompat();
  /// Create the current data representation.
  void setupData();

  QPointer<pqMultiSliceView> m_mainView; ///< The main view class
  Ui::MultiSliceViewClass m_ui;          ///< The view's UI form

  QMenu *m_contextMenu;
  QLineEdit *m_edit;
  int m_axisIndex;
  double m_sliceOffsetOnAxis;
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // MULTISLICEVIEW_H_
