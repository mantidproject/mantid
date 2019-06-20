// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "DPDFBackgroundRemover.h"
// Mantid Headers from the same project
#include "DPDFDisplayControl.h"
#include "DPDFFitControl.h"
#include "DPDFFourierTransform.h"
#include "DPDFInputDataControl.h"
#include "SliceSelector.h"
// Mantid headers from other projects
#include "MantidKernel/UsageService.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
// 3rd party library headers
// System includes

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

BackgroundRemover::BackgroundRemover(QWidget *parent)
    : UserSubWindow{parent}, m_sliceSelector(), m_inputDataControl(),
      m_displayControl(), m_fitControl{nullptr}, m_fourierTransform{nullptr} {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Interface", "DynamicPDF->BackgroundRemover", false);
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
  m_fitControl = m_uiForm.fitControl;
  m_fourierTransform = m_uiForm.fourier;
  // Correct size for the vertical splitter
  QList<int> sizes;
  sizes.push_back(300);
  sizes.push_back(200);
  m_uiForm.splitterModelResiduals->setSizes(sizes);
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
      nullptr, QString("Dynamic PDF Background Remover"));
}

/**
 * @brief Spawn the SliceSelector widget to load a matrix workspace (or file)
 * containing they dynamic structure factor.
 */
void BackgroundRemover::summonSliceSelector() {
  if (!m_sliceSelector) {
    // Initialize all the components
    // Initialize the slice selector
    m_sliceSelector = std::make_unique<SliceSelector>(this);
    // Initialize the InputDataControl object
    m_inputDataControl = std::make_unique<InputDataControl>();
    // Initialize the DisplayControl object
    m_displayControl = std::make_unique<DisplayControl>(
        m_inputDataControl.get(), m_uiForm.displayModelFit);
    m_displayControl->init();
    // Initialize the FitControl object
    m_fitControl->setInputDataControl(m_inputDataControl.get());
    m_fitControl->setDisplayControl(m_displayControl.get());
    // Initialize the FourierTransform object
    m_fourierTransform->setInputDataControl(m_inputDataControl.get());
    m_fourierTransform->setFitControl(m_fitControl);
    // Establish SIGNAL/SLOT connections
    // user loaded a workspace in the SliceSelector
    // (use get() for required raw pointer)
    connect(m_sliceSelector.get(), SIGNAL(signalSlicesLoaded(QString)),
            m_inputDataControl.get(), SLOT(updateWorkspace(QString)));
    // user selected a slice for fitting in SliceSelector
    connect(m_sliceSelector.get(),
            SIGNAL(signalSliceForFittingSelected(size_t)),
            m_inputDataControl.get(), SLOT(updateSliceForFitting(size_t)));
    // slice for fitting updated
    connect(m_inputDataControl.get(), SIGNAL(signalSliceForFittingUpdated()),
            m_displayControl.get(), SLOT(updateSliceForFitting()));
    m_fitControl->setConnections();
    m_fourierTransform->setConnections();
    connect(m_uiForm.pbFourier, SIGNAL(clicked()), m_fourierTransform,
            SLOT(transform()));
    connect(m_uiForm.pbClearFourierPlot, SIGNAL(clicked()), m_fourierTransform,
            SLOT(clearFourierPlot()));
  }

  m_sliceSelector->show();
  m_sliceSelector->raise();          // raise on top
  m_sliceSelector->activateWindow(); // set as active window
}
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
