#include "MantidMDCurveDialog.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

using Mantid::API::IMDWorkspace_sptr;
using Mantid::API::AnalysisDataService;
using Mantid::API::IMDWorkspace;

MantidMDCurveDialog::MantidMDCurveDialog(QWidget *parent, QString wsName)
    : QDialog(parent),
      m_wsName(wsName)
{
	ui.setupUi(this);
	m_lineOptions = new LinePlotOptions(this);

	// To set the right dimension labels
  IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(
    AnalysisDataService::Instance().retrieve( m_wsName.toStdString()) );
  if (ws)
    m_lineOptions->setOriginalWorkspace(ws);

}

/// @return true if error bars are selected
bool MantidMDCurveDialog::showErrorBars()
{
  return ui.chkErrorBars->isChecked();
}


MantidMDCurveDialog::~MantidMDCurveDialog()
{

}
void MantidMDCurveDialog::on_btnOK_toggled()
{
  this->accept();
}

void MantidMDCurveDialog::on_btnCancel_toggled()
{
  this->reject();
}
