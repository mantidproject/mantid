// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"

//---------------------------
// Qt Forward declarations
//---------------------------
class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QCheckBox;

namespace MantidQt {
namespace CustomDialogs {

/**
    This class gives specialised dialog for the LoadDAE algorithm.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 14/07/2010
*/
class LoadDAEDialog : public MantidQt::API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Constructor
  LoadDAEDialog(QWidget *parent = nullptr);
  /// Destruktor
  ~LoadDAEDialog() override;

protected:
  /// This does the work and must be overridden in each deriving class
  void initLayout() override;

private:
  /* GUI components */
  QLineEdit *lineHost;
  QLineEdit *lineName;
  QLineEdit *minSpLineEdit;
  QLineEdit *maxSpLineEdit;
  QLineEdit *listSpLineEdit;
  QLineEdit *updateLineEdit;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOM_DIALOGSLOADDAEDIALOG_H */
