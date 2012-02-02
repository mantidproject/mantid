#include "MantidMDCurveDialog.h"

MantidMDCurveDialog::MantidMDCurveDialog(QWidget *parent, QString wsName)
    : QWidget(parent),
      m_wsName(wsName)
{
	ui.setupUi(this);
	m_lineOptions = new LinePlotOptions(this);
}

MantidMDCurveDialog::~MantidMDCurveDialog()
{

}
