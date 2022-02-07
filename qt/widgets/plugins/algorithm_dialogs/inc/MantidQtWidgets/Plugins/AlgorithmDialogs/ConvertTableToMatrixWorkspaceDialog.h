// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "ui_ConvertTableToMatrixWorkspaceDialog.h"

//------------------------------------------------------------------------------
// Qt Forward declarations
//------------------------------------------------------------------------------
class QVBoxLayout;

namespace MantidQt {
//------------------------------------------------------------------------------
// Mantid Forward declarations
//------------------------------------------------------------------------------
namespace MantidWidgets {
class FileFinderWidget;
}

namespace CustomDialogs {

/**
  This class gives specialised dialog for the ConvertTableToMatrixWorkspace
  algorithm.

  @author Roman Tolchenov, Tessella plc
  @date 26/01/2012
*/
class ConvertTableToMatrixWorkspaceDialog : public API::AlgorithmDialog {
  Q_OBJECT

public:
  /// Default constructor
  ConvertTableToMatrixWorkspaceDialog(QWidget *parent = nullptr);

private slots:
  /// Update the column name widgets
  void fillColumnNames(const QString & /*qWSName*/);

private:
  /// Initialize the layout
  void initLayout() override;

private:
  /// Form
  Ui::ConvertTableToMatrixWorkspaceDialog m_form;
};
} // namespace CustomDialogs
} // namespace MantidQt
