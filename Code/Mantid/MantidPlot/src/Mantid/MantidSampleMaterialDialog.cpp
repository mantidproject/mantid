//----------------------------------
// Includes
//----------------------------------
#include "MantidSampleMaterialDialog.h"
#include "MantidUI.h"

#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QRadioButton>
#include <QFileInfo>
#include <QMessageBox>
#include <iostream>
#include <sstream>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

MantidSampleMaterialDialog::MantidSampleMaterialDialog(const QString & wsname, MantidUI* mtdUI, Qt::WFlags flags):
  QDialog(mtdUI->appWindow(), flags),
  m_wsname(wsname),
  m_mantidUI(mtdUI)
{
  m_uiForm.setupUi(this);
}

/**
* Initialize everything ub tge tree.
*/
void MantidSampleMaterialDialog::init()
{
}

