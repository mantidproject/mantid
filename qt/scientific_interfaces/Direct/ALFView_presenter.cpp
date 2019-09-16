// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_view.h"
#include "ALFView_model.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"
#include "MantidAPI/FileFinder.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <math.h>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::Kernel;
using namespace Mantid::API;

DECLARE_SUBWINDOW(ALFView)

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;

using Mantid::API::Workspace_sptr;

/// static logger
Mantid::Kernel::Logger g_log("ALFTest");

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_view(nullptr)
{
	// set up an empty ALF workspace
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create("LoadEmptyInstrument");
  alg->initialize();
  alg->setProperty("OutputWorkspace", "ALF");
  alg->setProperty("InstrumentName", "ALF");
  alg->execute();
}

void ALFView::initLayout() { m_view = new ALFView_view(this);
  this->setCentralWidget(m_view);
  connect(m_view, SIGNAL(newRun()), this, SLOT(loadRunNumber()));
  connect(m_view, SIGNAL(browsedToRun(const::std::string &)), this, SLOT(loadBrowsedFile(const std::string &)));
}

void ALFView::loadRunNumber() { 
	const std::string runNumber = "ALF" + std::to_string(m_view->getRunNumber()); 
	std::string filePath;
	// add memory of last good run and check alf instrument
	try {
          filePath = Mantid::API::FileFinder::Instance().findRuns(runNumber)[0];
        } catch (...) {
          return;
		}

	Direct::loadData(runNumber);
}

void ALFView::loadBrowsedFile(const std::string &fileName) {
  Direct::loadData(fileName);
  
  }

} // namespace CustomInterfaces
} // namespace MantidQt