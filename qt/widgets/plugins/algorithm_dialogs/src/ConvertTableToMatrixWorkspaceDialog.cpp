// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/ConvertTableToMatrixWorkspaceDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QUrl>

// Mantid
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/Property.h"

namespace MantidQt {
namespace CustomDialogs {
// Declare the dialog. Name must match the class name
DECLARE_DIALOG(ConvertTableToMatrixWorkspaceDialog)

/// Default constructor
ConvertTableToMatrixWorkspaceDialog::ConvertTableToMatrixWorkspaceDialog(
    QWidget *parent)
    : API::AlgorithmDialog(parent), m_form() {}

/**
 * When the input workspace changes the column name comboboxes have to
 * be updated.
 * @param qWSName :: The new table workspace name
 */
void ConvertTableToMatrixWorkspaceDialog::fillColumnNames(
    const QString &qWSName) {
  m_form.cbColumnX->clear();
  m_form.cbColumnY->clear();
  m_form.cbColumnE->clear();
  std::string wsName = qWSName.toStdString();
  if (wsName.empty())
    return;
  Mantid::API::ITableWorkspace_sptr tws =
      boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
  if (!tws)
    return;                             // just in case
  m_form.cbColumnE->insertItem(-1, ""); // the default value
  std::vector<std::string> columns = tws->getColumnNames();
  if (columns.empty())
    return;
  QString defaultXColumn;
  QString defaultYColumn;
  QString defaultEColumn;
  for (std::vector<std::string>::const_iterator column = columns.begin();
       column != columns.end(); ++column) {
    QString qName = QString::fromStdString(*column);
    m_form.cbColumnX->addItem(qName);
    m_form.cbColumnY->addItem(qName);
    m_form.cbColumnE->addItem(qName);
    Mantid::API::Column_sptr col = tws->getColumn(*column);
    if (col->getPlotType() == 1 && defaultXColumn.isEmpty()) // type X
    {
      defaultXColumn = qName;
    }
    if (col->getPlotType() == 2 && defaultYColumn.isEmpty()) // type Y
    {
      defaultYColumn = qName;
    }
    if (col->getPlotType() == 5 && defaultEColumn.isEmpty()) // type yErr
    {
      defaultEColumn = qName;
    }
  }
  // set initial guesses for column names
  if (!defaultXColumn.isEmpty()) {
    int i = m_form.cbColumnX->findText(defaultXColumn);
    if (i >= 0) {
      m_form.cbColumnX->setCurrentIndex(i);
    }
  }
  if (!defaultYColumn.isEmpty()) {
    int i = m_form.cbColumnY->findText(defaultYColumn);
    if (i >= 0) {
      m_form.cbColumnY->setCurrentIndex(i);
    }
  }
  if (!defaultEColumn.isEmpty()) {
    int i = m_form.cbColumnE->findText(defaultEColumn);
    if (i >= 0) {
      m_form.cbColumnE->setCurrentIndex(i);
    }
  }
}

//--------------------------------------------------------------------------
// Private methods (non-slot)
//---------------------------------------------------------------------------

/// Initialize the layout
void ConvertTableToMatrixWorkspaceDialog::initLayout() {
  m_form.setupUi(this);
  ((QVBoxLayout *)this->layout())->addLayout(createDefaultButtonLayout());
  tie(m_form.cbInputWorkspace, "InputWorkspace", m_form.gridLayout);
  tie(m_form.leOutputWorkspace, "OutputWorkspace", m_form.gridLayout);
  tie(m_form.cbColumnX, "ColumnX", m_form.gridLayout);
  tie(m_form.cbColumnY, "ColumnY", m_form.gridLayout);
  tie(m_form.cbColumnE, "ColumnE", m_form.gridLayout);

  QString presetInputWorkspace = this->getInputValue("InputWorkspace");
  fillAndSetComboBox("InputWorkspace", m_form.cbInputWorkspace);
  if (!presetInputWorkspace.isEmpty()) {
    int i = m_form.cbInputWorkspace->findText(presetInputWorkspace);
    if (i >= 0) {
      m_form.cbInputWorkspace->setCurrentIndex(i);
    }
  }
  connect(m_form.cbInputWorkspace, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(fillColumnNames(const QString &)));
  fillColumnNames(m_form.cbInputWorkspace->currentText());
}
} // namespace CustomDialogs
} // namespace MantidQt
