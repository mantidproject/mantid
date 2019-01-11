// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
#define MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_

//---------------------------
// Includes
//--------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_LOQScriptInputDialog.h"

namespace MantidQt {
namespace CustomDialogs {

/**
    This class gives specialised dialog for the LOQ input algorithm.

    @author Martyn Gigg, Tessella Support Services plc
    @date 05/03/2009
*/
class LOQScriptInputDialog : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  LOQScriptInputDialog(QWidget *parent = nullptr);

private:
  /// Initialize the layout
  void initLayout() override;

  /// Get the input out of the dialog
  void parseInput() override;

private slots:

  /// browse clicked method
  void browseClicked();

private:
  // The form generated with Qt Designer
  Ui::LOQScriptInputDialog m_uiForm;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_LOQSCRIPTINPUTDIALOG_H_
