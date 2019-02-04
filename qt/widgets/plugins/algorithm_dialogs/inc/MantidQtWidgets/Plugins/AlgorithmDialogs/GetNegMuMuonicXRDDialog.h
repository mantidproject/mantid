// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/PeriodicTableWidget.h"
#include <QDoubleSpinBox>
namespace MantidQt {

namespace CustomDialogs {
/**
This class gives a specialised dialog for the GetNegMuMuonicXRD algorithm
@author Matt King, ISIS Rutherford Appleton Laboratory
@date 11/08/2015
*/

class GetNegMuMuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Constructor
  GetNegMuMuonicXRDDialog(QWidget *parent = nullptr);

private:
  /// Periodic Table widget used for selection of elements property
  PeriodicTableWidget *m_periodicTable;
  /// QLineEdit used for input of y-position property
  QDoubleSpinBox *m_yPosition;
  /// QLineEdit used for input of GroupWorkspaceSpace
  QLineEdit *m_groupWorkspaceNameInput;
  // Check box for showing or hiding the Legend for PeriodicTableWidget
  QCheckBox *m_showLegendCheck;
  /** Enables a the buttons corresponding to the elements
  for which data exists in GetNegMuMuonicXRD.py
  */
  void enableElementsForGetNegMuMuonicXRD();

private slots:
  void parseInput() override;
  void showLegend();

protected:
  // create the initial layout
  void initLayout() override;
signals:
  /// signal emitted when validateDialogInput passes
  void validInput();
};
} // namespace CustomDialogs
} // namespace MantidQt
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_