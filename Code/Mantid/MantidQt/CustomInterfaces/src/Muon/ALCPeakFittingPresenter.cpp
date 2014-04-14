#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

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

    connect(m_view, SIGNAL(fitRequested()), this, SLOT(fit()));
  }

  void ALCPeakFittingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    assert(data->getNumberHistograms() == 1);

    m_data = data;
    m_view->setDataCurve(*(ALCHelper::curveDataFromWs(data, 0)));
  }

  void ALCPeakFittingPresenter::fit()
  {
    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", m_view->function());
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->execute();

    IFunction_sptr fittedFunc = fit->getProperty("Function");
    m_view->setFunction(fittedFunc->asString());

    const Mantid::MantidVec& x = m_data->readX(0);
    m_view->setFittedCurve(*(ALCHelper::curveDataFromFunction(fittedFunc, x)));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
