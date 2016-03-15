// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFBackgroundRemover.h"
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidKernel/make_unique.h"
#include "MantidQtAPI/HelpWindow.h"
// 3rd party library headers
// System #includes

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(BackgroundRemover)

/*              **********************
 *              **  Public Methods  **
 *              **********************/

BackgroundRemover::BackgroundRemover(QWidget *parent) :
  UserSubWindow{parent},
  m_sliceSelector(),
  m_displayModelFit{nullptr} {

}

/**
 * @brief Initialize the form, and the signals/slots connections
 */
void BackgroundRemover::initLayout() {
  // initialize the components of the form
  m_uiForm.setupUi(this);
  // initialize the handy pointers to the components of the form
  m_displayModelFit = m_uiForm.displayModelFit;
  // signals/slots connections for this form
  connect(m_uiForm.pushButtonSummonSliceSelector, SIGNAL(clicked()), this,
          SLOT(summonSliceSelector()));
  connect(m_uiForm.pushButtonHelp, SIGNAL(clicked()), this, SLOT(showHelp()));

}

/*              *********************
 *              **  Private Slots  **
 *              *********************/

/**
 * @brief Opens the Qt help page for the interface
 */
void BackgroundRemover::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("DynamicPDFBackgroundRemover"));
}

/**
 * @brief Spawn the SliceSelector widget to load a matrix workspace (or file)
 * containing they dynamic structure factor.
 */
void BackgroundRemover::summonSliceSelector(){
  if(!m_sliceSelector){
    m_sliceSelector = Mantid::Kernel::make_unique<SliceSelector>(this);
  }
  else{
    m_sliceSelector->activateWindow(); // set as active window
  }
}




}
}
}
