// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_PlotAsymmetryByLogValueDialog.h"

#include <QSignalMapper>
#include <QString>

//---------------------------
// Qt Forward declarations
//---------------------------
class QVBoxLayout;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace MantidQt {
namespace CustomDialogs {

/**
    This class gives specialised dialog for the LoadRaw algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 24/02/2009
*/
class PlotAsymmetryByLogValueDialog : public MantidQt::API::AlgorithmDialog {

  Q_OBJECT

public:
  /// Constructor
  PlotAsymmetryByLogValueDialog(QWidget *parent = nullptr);
  /// Destructor
  ~PlotAsymmetryByLogValueDialog() override;

private:
  /** @name Virtual functions. */
  //@{
  /// Create the layout
  void initLayout() override;
  //@}

private slots:

  /// Opens a file dialog. Updates the QLineEdit provided when the dialog is
  /// closed.
  void openFileDialog(const QString &filePropName);
  void fillLogBox(const QString & /*unused*/);

  /// Show or hide Dead Time file widget depending on which Dead Time type is
  /// selected.
  void showHideDeadTimeFileWidget(int deadTimeTypeIndex);

private:
  // The form generated with Qt Designer
  Ui::PlotAsymmetryByLogValueDialog m_uiForm;

  /// Maps Browse buttons to file properties
  QSignalMapper *browseButtonMapper;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOM_DIALOGSLPLOTASYMMETRYBYLOGVALUEDIALOG_H
