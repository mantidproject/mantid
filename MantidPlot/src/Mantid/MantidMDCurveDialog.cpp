// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDCurveDialog.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDWorkspace.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::IMDWorkspace;
using Mantid::API::IMDWorkspace_sptr;

MantidMDCurveDialog::MantidMDCurveDialog(QWidget *parent, const QString &wsName) : QDialog(parent), m_wsName(wsName) {
  ui.setupUi(this);
  m_lineOptions = new LinePlotOptions(this);
  ui.mainLayout->insertWidget(0, m_lineOptions);

  // To set the right dimension labels
  IMDWorkspace_sptr ws =
      std::dynamic_pointer_cast<IMDWorkspace>(AnalysisDataService::Instance().retrieve(m_wsName.toStdString()));
  if (ws) {
    // Retrieve the original workspace. Prefer the "intermediate" one if
    // available.
    if (ws->hasOriginalWorkspace())
      ws = std::dynamic_pointer_cast<IMDWorkspace>(ws->getOriginalWorkspace(ws->numOriginalWorkspaces() - 1));
    if (ws)
      m_lineOptions->setOriginalWorkspace(ws);
  }

  // Connect the button slots
  QMetaObject::connectSlotsByName(this);
}

/// @return true if error bars are selected
bool MantidMDCurveDialog::showErrorBars() { return ui.chkErrorBars->isChecked(); }

MantidMDCurveDialog::~MantidMDCurveDialog() {}
void MantidMDCurveDialog::on_btnOK_clicked() {
  this->accept();
  this->close();
}

void MantidMDCurveDialog::on_btnCancel_clicked() {
  this->reject();
  this->close();
}
