#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionViewQtGUI.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

#include <boost/lexical_cast.hpp>

#include <QCheckBox>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>

namespace MantidQt {
namespace CustomInterfaces {

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(EnggDiffractionViewQtGUI)

/**
 * Default constructor.
 *
 * @param parent Parent window (most likely the Mantid main app window).
 */
EnggDiffractionViewQtGUI::EnggDiffractionViewQtGUI(QWidget *parent)
    : UserSubWindow(parent), IEnggDiffractionView(), m_presenter(NULL) {}

EnggDiffractionViewQtGUI::~EnggDiffractionViewQtGUI() {}

void EnggDiffractionViewQtGUI::initLayout() {
  // setup container ui
  m_ui.setupUi(this);
  // add tab contents and set up their ui's
  QWidget *wCalib = new QWidget(m_ui.tabMain);
  m_uiTabCalib.setupUi(wCalib);
  m_ui.tabMain->addTab(wCalib, QString("Calibration"));
  QWidget *wSettings = new QWidget(m_ui.tabMain);
  m_uiTabSettings.setupUi(wSettings);
  m_ui.tabMain->addTab(wSettings, QString("Setup"));

  readSettings();

  // basic UI setup
  doSetupTabCalib();
  doSetupTabSettings();

  // presenter that knows how to handle a IEnggDiffractionView should take care
  // of all the logic
  // note that the view needs to know the concrete presenter
  m_presenter.reset(new EnggDiffractionPresenter(this));

  // it will know what compute resources and tools we have available:
  // This view doesn't even know the names of compute resources, etc.
  m_presenter->notify(IEnggDiffractionPresenter::Start);
}

void EnggDiffractionViewQtGUI::doSetupTabCalib() {}

void EnggDiffractionViewQtGUI::doSetupTabSettings() {}

void EnggDiffractionViewQtGUI::readSettings() {}

void EnggDiffractionViewQtGUI::saveSettings() const {}

void EnggDiffractionViewQtGUI::userWarning(const std::string &err,
                                           const std::string &description) {
  QMessageBox::warning(this, QString::fromStdString(err),
                       QString::fromStdString(description), QMessageBox::Ok,
                       QMessageBox::Ok);
}

void EnggDiffractionViewQtGUI::userError(const std::string &err,
                                         const std::string &description) {
  QMessageBox::critical(this, QString::fromStdString(err),
                        QString::fromStdString(description), QMessageBox::Ok,
                        QMessageBox::Ok);
}

std::string EnggDiffractionViewQtGUI::getRBNumber() const {
  return "not available";
}

void EnggDiffractionViewQtGUI::loadCalibrationClicked() {}

void EnggDiffractionViewQtGUI::closeEvent(QCloseEvent *event) {
  int answer = QMessageBox::AcceptRole;

  QMessageBox msgBox;
  msgBox.setWindowTitle("Close the engineering diffraction interface");
  // with something like this, we'd have layout issues:
  // msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  // msgBox.setDefaultButton(QMessageBox::Yes);
  msgBox.setIconPixmap(QPixmap(":/win/unknown.png"));
  QCheckBox confirmCheckBox("Always ask for confirmation", &msgBox);
  confirmCheckBox.setCheckState(Qt::Checked);
  msgBox.layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
  msgBox.layout()->addWidget(&confirmCheckBox);
  QPushButton *bYes = msgBox.addButton("Yes", QMessageBox::YesRole);
  bYes->setIcon(style()->standardIcon(QStyle::SP_DialogYesButton));
  QPushButton *bNo = msgBox.addButton("No", QMessageBox::NoRole);
  bNo->setIcon(style()->standardIcon(QStyle::SP_DialogNoButton));
  msgBox.setDefaultButton(bNo);
  msgBox.setText("You are about to close this interface");
  msgBox.setInformativeText("Are you sure?");
  answer = msgBox.exec();

  if (answer == QMessageBox::AcceptRole) {
    m_presenter->notify(IEnggDiffractionPresenter::ShutDown);
    event->accept();
  } else {
    event->ignore();
  }
}

void EnggDiffractionViewQtGUI::openHelpWin() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Engineering_Diffraction"));
}

} // namespace CustomInterfaces
} // namespace MantidQt
