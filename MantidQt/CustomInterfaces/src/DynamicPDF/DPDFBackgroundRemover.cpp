// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFBackgroundRemover.h"
// Mantid Headers from the same project
#include "MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h"
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFInputDataControl.h"
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFDisplayControl.h"

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
  m_inputDataControl(),
  m_displayControl() {

}

/**
 * @brief explicit default destructor to prevent compile
 * errors for member attributes defined with unique_ptr
 */
BackgroundRemover::~BackgroundRemover() = default;

/**
 * @brief Initialize the form, and the signals/slots connections
 */
void BackgroundRemover::initLayout() {
  // initialize the components of the form
  m_uiForm.setupUi(this);
  // user wants to load new slices
  connect(m_uiForm.pushButtonSummonSliceSelector, SIGNAL(clicked()), this,
          SLOT(summonSliceSelector()));
  // user wants help
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
      NULL, QString("DPDFBackgroundRemover"));
}

/**
 * @brief Spawn the SliceSelector widget to load a matrix workspace (or file)
 * containing they dynamic structure factor.
 */
void BackgroundRemover::summonSliceSelector(){
  if(!m_sliceSelector){
    // Initialize all the components
    // Initialize the slice selector
    m_sliceSelector = Mantid::Kernel::make_unique<SliceSelector>(this);
    // Initialize the InputDataControl object
    m_inputDataControl = Mantid::Kernel::make_unique<InputDataControl>();
    // Initialize the DisplayControl object
    m_displayControl = Mantid::Kernel::make_unique<DisplayControl>
      (m_inputDataControl.get(), m_uiForm.displayModelFit);
    m_displayControl->init();

    // Establish SIGNAL/SLOT connections
    // user loaded a workspace in the SliceSelector (use get() for required raw pointer)
    connect(m_sliceSelector.get(), SIGNAL(signalSlicesLoaded(QString)),
      m_inputDataControl.get(), SLOT(updateWorkspace(QString)));
    // user selected a slice for fitting in SliceSelector (get() for required raw pointer)
    connect(m_sliceSelector.get(), SIGNAL(signalSliceForFittingSelected(size_t)),
      m_inputDataControl.get(), SLOT(updateSliceForFitting(size_t)));
    // user selected a slice for fitting in SliceSelector
    connect(m_inputDataControl.get(), SIGNAL(signalSliceForFittingUpdated()),
      m_displayControl.get(), SLOT(updateSliceForFitting()));
  }

  m_sliceSelector->show();
  m_sliceSelector->raise(); // raise on top
  m_sliceSelector->activateWindow(); // set as active window
}




}
}
}
