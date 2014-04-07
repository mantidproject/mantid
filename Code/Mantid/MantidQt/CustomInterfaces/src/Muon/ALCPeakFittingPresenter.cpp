#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCPeakFittingPresenter::ALCPeakFittingPresenter(IALCPeakFittingView* view)
    : m_view(view), m_data()
  {}

  void ALCPeakFittingPresenter::initialize()
  {
    m_view->initialize();

    connect(m_view, SIGNAL(fit()), this, SLOT(fit()));
  }

  void ALCPeakFittingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    m_data = data;
    m_view->setData(data);
  }

  void ALCPeakFittingPresenter::fit()
  {
    IFunction_sptr funcToFit =
        FunctionFactory::Instance().createInitialized( m_view->peaks()[0]->asString() );

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", funcToFit);
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->execute();

    auto fittedPeak = boost::dynamic_pointer_cast<IPeakFunction>(funcToFit);
    assert(fittedPeak);

    IALCPeakFittingView::ListOfPeaks fittedPeaks;
    fittedPeaks.push_back(fittedPeak);

    m_view->setPeaks(fittedPeaks);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
