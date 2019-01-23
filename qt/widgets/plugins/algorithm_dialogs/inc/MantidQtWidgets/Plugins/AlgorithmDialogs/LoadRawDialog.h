// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGSLOADRAWDIALOG_H

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"

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
class LoadRawDialog : public MantidQt::API::AlgorithmDialog {

  Q_OBJECT

public:
  /// Constructor
  LoadRawDialog(QWidget *parent = nullptr);
  /// Destructor
  ~LoadRawDialog() override;

private:
  /** @name Virtual functions. */
  //@{
  /// Create the layout
  void initLayout() override;
  //@}

private slots:

  /// A slot for the browse button clicked signal
  void browseClicked();

private:
  /// The line inputs
  QLineEdit *m_pathBox, *m_wsBox;
};
} // namespace CustomDialogs
} // namespace MantidQt

#endif // MANTIDQT_CUSTOMDIALOGS_LOADRAWDIALOG_H
