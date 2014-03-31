#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view)
    : m_view(view), m_data()
  {}

  void ALCBaselineModellingPresenter::initialize()
  {
    m_view->initialize();

    connect(m_view, SIGNAL(fit()), this, SLOT(fit()));
  }

  void ALCBaselineModellingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    m_data = data;
    m_view->displayData(data);
  }

  void ALCBaselineModellingPresenter::fit()
  {
    IFunction_sptr funcToFit =
        FunctionFactory::Instance().createInitialized( m_view->function()->asString() );

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", funcToFit);
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->execute();

    m_view->updateFunction(funcToFit);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
