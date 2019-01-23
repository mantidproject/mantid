// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MODECONTROLWIDGET_H_
#define MODECONTROLWIDGET_H_

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"
#include "ui_ModeControlWidget.h"

#include <QString>
#include <QWidget>
#include <map>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class controls the current view for the main level program.

  @author Michael Reuter
  @date 24/05/2011
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS ModeControlWidget
    : public QWidget {
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget of the mode control widget
   */
  ModeControlWidget(QWidget *parent = nullptr);
  /// Default destructor.
  ~ModeControlWidget() override;

  /// Enumeration for the view types
  enum Views { STANDARD, THREESLICE, MULTISLICE, SPLATTERPLOT };

public slots:
  /// Enable/disable a specific view button.
  void enableViewButton(ModeControlWidget::Views mode, bool state);

  /**
   * Enable/disable all view buttons, except standard.
   * @param state whether or not to enable the buttons
   * @param initialView The initial view.
   */
  void enableViewButtons(ModeControlWidget::Views initialView, bool state);

  /// Expose the standard view button.
  void setToStandardView();

  /// Switch to a selected view.
  void setToSelectedView(ModeControlWidget::Views view);

  /// Convert a string into an enum
  ModeControlWidget::Views getViewFromString(const QString &view);

signals:
  /**
   * Function to make the main program window switch to a given view.
   * @param v the type of view to switch to
   */
  void executeSwitchViews(ModeControlWidget::Views v);

protected slots:
  /**
   * Execute switch to multislice view, disable multislice button and
   * enable other view buttons.
   */
  void onMultiSliceViewButtonClicked();
  /**
   * Execute switch to splatter plot view, disable splatter plot
   * button and enable other views.
   */
  void onSplatterPlotViewButtonClicked();
  /**
   * Execute switch to standard view, disable standard button and
   * enable other view buttons.
   */
  void onStandardViewButtonClicked();
  /**
   * Execute switch to threeslice view, disable threeslice button and
   * enable other view buttons.
   */
  void onThreeSliceViewButtonClicked();

private:
  Ui::ModeControlWidgetClass ui; ///< The mode control widget's UI form
  std::map<QString, Views> mapFromStringToView; //< Holds the mapping from the a
  // string to an associated enum
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // MODECONTROLWIDGET_H_
