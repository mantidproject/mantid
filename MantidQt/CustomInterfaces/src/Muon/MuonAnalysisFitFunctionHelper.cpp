#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitFunctionHelper.h"

using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionControl;

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param parent :: [input] Parent dialog (MuonAnalysis)
 * @param fitBrowser :: [input] Non-owning pointer to muon fit property browser
 * @param funcBrowser :: [input] Non-owning pointer to function browser
 */
MuonAnalysisFitFunctionHelper::MuonAnalysisFitFunctionHelper(
    QObject *parent, IMuonFitFunctionControl *fitBrowser,
    IFunctionBrowser *funcBrowser)
    : QObject(parent), m_fitBrowser(fitBrowser), m_funcBrowser(funcBrowser) {
  doConnect();
}

/**
 * Connect up signals and slots
 * Abstract base class is not a QObject, so attempt a cast.
 * (Its derived classes are QObjects).
 */
void MuonAnalysisFitFunctionHelper::doConnect() {
  if (const QObject *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser)) {
    connect(fitBrowser, SIGNAL(functionUpdateRequested()), this,
            SLOT(updateFunction()));
    connect(fitBrowser, SIGNAL(functionUpdateAndFitRequested(bool)), this,
            SLOT(updateFunctionAndFit(bool)));
  }
  if (const QObject *funcBrowser = dynamic_cast<QObject *>(m_funcBrowser)) {
    connect(funcBrowser, SIGNAL(functionStructureChanged()), this,
            SLOT(updateFunction()));
  }
}

/**
 * Queries function browser and updates function in fit property browser.
 */
void MuonAnalysisFitFunctionHelper::updateFunction() {
  const QString funcString = m_funcBrowser->getFunctionString();
  m_fitBrowser->setFunction(funcString);
}

/**
 * Called when a fit is requested.
 * Queries function browser and updates function in fit property browser.
 * Then calls fit or sequential fit as controlled by argument.
 * @param sequential :: [input] Whether a regular or sequential fit was
 * requested.
 */
void MuonAnalysisFitFunctionHelper::updateFunctionAndFit(bool sequential) {
  updateFunction();
  if (sequential) {
    m_fitBrowser->runSequentialFit();
  } else {
    m_fitBrowser->runFit();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
