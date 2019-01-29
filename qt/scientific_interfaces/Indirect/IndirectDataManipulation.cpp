// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataManipulation.h"

#include "Elwin.h"
#include "IndirectMoments.h"
#include "IndirectSqw.h"
#include "IndirectSymmetrise.h"
#include "Iqt.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
DECLARE_SUBWINDOW(IndirectDataManipulation)

IndirectDataManipulation::IndirectDataManipulation(QWidget *parent)
    : UserSubWindow(parent) {
  m_uiForm.setupUi(this);

  // m_tabs.emplace(SYMMETRISE2, new IndirectSymmetrise(
  //                                m_uiForm.twIDMTabs->widget(SYMMETRISE2)));
  // m_tabs.emplace(SQW2, new IndirectSqw(m_uiForm.twIDMTabs->widget(SQW2)));
  // m_tabs.emplace(MOMENTS2,
  //               new IndirectMoments(m_uiForm.twIDMTabs->widget(MOMENTS2)));
  // m_tabs.emplace(ELWIN2, new Elwin(m_uiForm.twIDMTabs->widget(ELWIN2)));
  m_tabs.emplace(ELWIN, new Elwin(m_uiForm.twIDMTabs->widget(ELWIN)));
  m_tabs.emplace(IQT, new Iqt(m_uiForm.twIDMTabs->widget(IQT)));
}

void IndirectDataManipulation::initLayout() {
  // Set up all tabs
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->setupTab();
    connect(tab->second, SIGNAL(runAsPythonScript(QString const &, bool)), this,
            SIGNAL(runAsPythonScript(QString const &, bool)));
    connect(tab->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(QString const &)));
  }
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(handleHelp()));
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this,
          SLOT(handleExportToPython()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(handleManageDirectories()));
}

void IndirectDataManipulation::handleHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Indirect Data Manipulation"));
}

void IndirectDataManipulation::handleExportToPython() {
  std::size_t const currentTab = m_uiForm.twIDMTabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

void IndirectDataManipulation::handleManageDirectories() {
  auto dialog = new MantidQt::API::ManageUserDirectories(this);
  dialog->show();
  dialog->setFocus();
}

void IndirectDataManipulation::showMessageBox(QString const &message) {
  showInformationBox(message);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
