// includes for workspace handling
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
// includes for interface functionality
#include "MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.h"
#include "MantidQtMantidWidgets/DisplayCurveFit.h"
#include "MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

// Add this class to the list of specialised dialogs in this namespace only if
// compiling in Debug mode
//#ifndef NDEBUG
DECLARE_SUBWINDOW(DisplayCurveFitTest)
//#endif

using curveType = MantidQt::MantidWidgets::DisplayCurveFit::curveType;
using dcRange = MantidQt::MantidWidgets::DisplayCurveFit::dcRange;

//           ++++++++++++++++++++++++++++
//           +++++  Public Members  +++++
//           ++++++++++++++++++++++++++++

/// Constructor
DisplayCurveFitTest::DisplayCurveFitTest(QWidget *parent)
    : UserSubWindow{parent} {}

DisplayCurveFitTest::~DisplayCurveFitTest() {}

/**
 * @brief Initialize the widgets defined within the form generated in
 * Qt-Designer. Also, defined the SIGNALS to SLOTS connections.
 */
void DisplayCurveFitTest::initLayout() {
  m_uiForm.setupUi(this);
  connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString &)), this,
          SLOT(loadSpectra(const QString &)));
}

//           +++++++++++++++++++++++++++
//           +++++  Private Slots  +++++
//           +++++++++++++++++++++++++++

/**
 * @brief The test proper that loads the fit curves to be
 * displayed and the two ranges.
 * @param workspaceName the name of the workspace containing
 * the data of the curves to be displayed.
 */
void DisplayCurveFitTest::loadSpectra(const QString &workspaceName) {
  auto workspace = Mantid::API::AnalysisDataService::Instance()
                       .retrieveWS<Mantid::API::MatrixWorkspace>(
                           workspaceName.toStdString());
  if (!workspace) {
    auto title = QString::fromStdString(this->name());
    auto error =
        QString::fromStdString("Workspace must be of type MatrixWorkspace");
    QMessageBox::warning(this, title, error);
    return;
  }
  if (workspace->getNumberHistograms() < 4) {
    auto title = QString::fromStdString(this->name());
    auto error = QString("Not enough number of histograms in the workspace");
    QMessageBox::warning(this, title, error);
    return;
  }
  m_uiForm.displayFit->addSpectrum(curveType::data, workspace, 0);
  auto curveRange = m_uiForm.displayFit->getCurveRange(curveType::data);
  static bool firstPass{TRUE};

  // Set up the range selector for the fit
  m_uiForm.displayFit->addRangeSelector(dcRange::fit);
  auto rangeSelectorFit = m_uiForm.displayFit->m_rangeSelector.at(dcRange::fit);
  if (firstPass || m_uiForm.updateRangeSelectors->isChecked()) {
    rangeSelectorFit->setRange(curveRange.first, curveRange.second);
    rangeSelectorFit->setMinimum(1.05 * curveRange.first);
    rangeSelectorFit->setMaximum(0.95 * curveRange.second);
  }

  // Set up the range evaluate range selector
  m_uiForm.displayFit->addRangeSelector(dcRange::evaluate);
  auto rangeSelectorEvaluate =
      m_uiForm.displayFit->m_rangeSelector.at(dcRange::evaluate);
  if (firstPass || m_uiForm.updateRangeSelectors->isChecked()) {
    rangeSelectorEvaluate->setRange(curveRange.first, curveRange.second);
    rangeSelectorEvaluate->setMinimum(curveRange.first);
    rangeSelectorEvaluate->setMaximum(curveRange.second);
  }

  m_uiForm.displayFit->addSpectrum(curveType::fit, workspace, 1);
  m_uiForm.displayFit->addSpectrum(curveType::residuals, workspace, 2);
  m_uiForm.displayFit->addSpectrum(curveType::guess, workspace, 3);

  m_uiForm.displayFit->addResidualsZeroline();
  firstPass = FALSE;
}

} // namespace MantidQt
} // namespace CustomInterfaces
} // namespace DynamicPDF
