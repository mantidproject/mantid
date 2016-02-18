// includes for workspace handling
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
// includes for interface functionality
#include "MantidQtCustomInterfaces/DynamicPDF/DisplayCurveFitTest.h"
#include "MantidQtMantidWidgets/DisplayCurveFit.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

// Add this class to the list of specialised dialogs in this namespace only if
// compiling in Debug mode
#ifndef NDEBUG
DECLARE_SUBWINDOW(DisplayCurveFitTest)
#endif

using curveType = MantidQt::MantidWidgets::DisplayCurveFit::curveType;

//           ++++++++++++++++++++++++++++
//           +++++  Public Members  +++++
//           ++++++++++++++++++++++++++++

/// Constructor
DisplayCurveFitTest::DisplayCurveFitTest(QWidget *parent) : UserSubWindow{parent} {
  std::cerr <<"UserSubWindow{parent} created\n";
}

DisplayCurveFitTest::~DisplayCurveFitTest() {}

void DisplayCurveFitTest::initLayout() {
  m_uiForm.setupUi(this);
  std::cerr<<"finished with m_uiForm.setupUi(this)\n";
  connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString &)), this,
          SLOT(loadSpectra(const QString &)));
}

//           +++++++++++++++++++++++++++
//           +++++  Private Slots  +++++
//           +++++++++++++++++++++++++++

void DisplayCurveFitTest::loadSpectra(const QString &workspaceName) {
  std::cerr<<"Entering DisplayCurveFitTest::loadSpectra()";
  std::cerr<<"workspaceName="<<workspaceName.toStdString()<<std::endl;
  auto workspace = Mantid::API::AnalysisDataService::Instance()
                       .retrieveWS<Mantid::API::MatrixWorkspace>(
                           workspaceName.toStdString());
  std::cerr<<"workspace instantiated. Number of histograms is "<<workspace->getNumberHistograms()<<"\n";
  if (workspace->getNumberHistograms() < 4) {
    throw std::out_of_range("Not enough number of histograms in the workspace");
  }
  std::cerr<<"Before entering DisplayCurveFit::addSpectrum\n";
  m_uiForm.displayFit->addSpectrum(curveType::data, workspace, 0);
  std::cerr<<"curveType::data added\n";
  m_uiForm.displayFit->addSpectrum(curveType::fit, workspace, 1);
  m_uiForm.displayFit->addSpectrum(curveType::residuals, workspace, 2);
  m_uiForm.displayFit->addSpectrum(curveType::guess, workspace, 3);
}

} // namespace MantidQt
} // namespace CustomInterfaces
} // namespace DynamicPDF
