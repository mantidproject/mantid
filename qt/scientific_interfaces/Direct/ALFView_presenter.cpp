// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_presenter.h"
#include "ALFView_model.h"
#include "ALFView_view.h"

#include "MantidAPI/FileFinder.h"

#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <math.h>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

using namespace Mantid;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;

DECLARE_SUBWINDOW(ALFView)

/// static logger
Mantid::Kernel::Logger g_log("ALFView");

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_view(nullptr),m_currentRun(0) {
  Direct::loadEmptyInstrument();
}

void ALFView::initLayout() {
  m_view = new ALFView_view(this);
  this->setCentralWidget(m_view);
  connect(m_view, SIGNAL(newRun()), this, SLOT(loadRunNumber()));
  connect(m_view, SIGNAL(browsedToRun(std::string)), this,
          SLOT(loadBrowsedFile(const std::string)));
}

void ALFView::loadAndAnalysis(const std::string &run) {
  int runNumber = Direct::loadData(run);
  auto bools = Direct::isDataValid();
  if (bools.first) {
    Direct::rename();
    m_currentRun = runNumber;
  } else {
    Direct::remove();
  }
  // if the displayed run number is out of sinc
  int das = m_view->getRunNumber();
  if (m_view->getRunNumber() != m_currentRun) {
    m_view->setRunQuietly(QString::number(m_currentRun));
  }
  if (bools.first && !bools.second) {
    Direct::transformData();
  }
}

void ALFView::loadRunNumber() {
  int newRun = m_view->getRunNumber();
  const int currentRunInADS = Direct::currentRun();


  if (currentRunInADS == newRun) {
    return;
  }
  const std::string runNumber = "ALF" + std::to_string(newRun);
  std::string filePath;
  // check its a valid run number
  try {
    filePath = Mantid::API::FileFinder::Instance().findRuns(runNumber)[0];
  } catch (...) {
    m_view->setRunQuietly(QString::number(m_currentRun));
	// if file has been deleted we should replace it
    if (currentRunInADS == -999) {
      loadAndAnalysis("ALF" + std::to_string(m_currentRun));
    }
    return;
  }
  loadAndAnalysis(runNumber);
}

void ALFView::loadBrowsedFile(const std::string fileName) {
  Direct::loadData(fileName);
  loadAndAnalysis(fileName);
}

} // namespace CustomInterfaces
} // namespace MantidQt