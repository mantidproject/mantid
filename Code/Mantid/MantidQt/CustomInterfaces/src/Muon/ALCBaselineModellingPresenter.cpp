#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view,
                                                               MatrixWorkspace_const_sptr data)
    : m_view(view), m_data(data)
  {}
    
  ALCBaselineModellingPresenter::~ALCBaselineModellingPresenter()
  {
  }

  void ALCBaselineModellingPresenter::initialize()
  {
    connectView();

    m_view->displayData(m_data);
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

  void ALCBaselineModellingPresenter::connectView()
  {
    connect(m_view, SIGNAL(fit()), this, SLOT(fit()));
  }

} // namespace CustomInterfaces
} // namespace MantidQt
