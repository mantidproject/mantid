#include "MantidQtCustomInterfaces/Poldi/QtPoldiView.h"
#include "MantidQtCustomInterfaces/Poldi/PoldiGenericDataProcessorPresenterFactory.h"
#include "MantidQtCustomInterfaces/Poldi/PoldiPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h"
#include <boost/shared_ptr.hpp>
#include <qmessagebox.h>

namespace {
Mantid::Kernel::Logger g_log("QtPoldiView");
}

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(QtPoldiView)
}
}

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

/// Constructor
QtPoldiView::QtPoldiView(QWidget *parent)
    : UserSubWindow(parent), IPoldiView(), m_presenter() {}

/**
 * Set up the dialog layout.
 */
void QtPoldiView::initLayout() {
  m_uiForm.setupUi(this);

  // Create a presenter
  PoldiGenericDataProcessorPresenterFactory factory;
  boost::shared_ptr<MantidQt::MantidWidgets::DataProcessorPresenter> presenter =
      factory.create();

  // Create the DataProcessor widget
  MantidQt::MantidWidgets::QDataProcessorWidget *dataProcessor =
      new MantidQt::MantidWidgets::QDataProcessorWidget(presenter, this);

  // Add the DataProcessor widget to the layout
  m_uiForm.verticalLayout->addWidget(dataProcessor);

  // Load a demo table
  connect(m_uiForm.actionDemo, SIGNAL(triggered()), this,
          SLOT(loadDemoTriggered()));

  // Create the presenter
  m_presenter = Mantid::Kernel::make_unique<PoldiPresenter>(this, presenter);
}

/**
* Load a demo table.
*/
void QtPoldiView::loadDemoTriggered() {

  m_presenter->notify(IPoldiPresenter::LoadDemoFlag);
}

/** Print information
* @param prompt : [input] The prompt to appear on the dialog
* @param title : [input] The text for the title bar of the dialog
*/
void QtPoldiView::giveUserInfo(std::string prompt, std::string title) {
  QMessageBox::information(this, QString(title.c_str()),
                           QString(prompt.c_str()), QMessageBox::Ok,
                           QMessageBox::Ok);
}
