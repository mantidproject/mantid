#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view,
                                                               IALCBaselineModellingModel* model)
    : m_view(view), m_model(model)
  {
    // TODO: check that both view and model aren't NULL
  }

  void ALCBaselineModellingPresenter::initialize()
  {
    m_view->initialize();

    connect(m_view, SIGNAL(fitRequested()), SLOT(fit()));
    connect(m_view, SIGNAL(addSectionRequested()), SLOT(addSection()));
    connect(m_view, SIGNAL(removeSectionRequested(size_t)), SLOT(removeSection(size_t)));

    connect(m_view, SIGNAL(sectionModified(size_t, double, double)),
            SLOT(onSectionModified(size_t, double, double)));

    connect(m_view, SIGNAL(sectionSelectorModified(size_t,double,double)),
            SLOT(onSectionSelectorModified(size_t,double,double)));
  }

  /**
   * @param data :: Data we want to fit baseline for
   */
  void ALCBaselineModellingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    m_model->setData(data);

    assert(data->getNumberHistograms() == 1);

    boost::shared_ptr<QwtData> curveData = ALCHelper::curveDataFromWs(data,0);

    m_view->setDataCurve(*curveData);
  }

  /**
   * Perform a fit and updates the view accordingly
   */
  void ALCBaselineModellingPresenter::fit()
  {
    // TODO: catch exceptions
    m_model->fit(m_view->function(), m_view->sections());

    IFunction_const_sptr fittedFunc = m_model->fittedFunction();

    m_view->setFunction(fittedFunc);

    const std::vector<double>& xValues = m_model->data()->readX(0);
    m_view->setBaselineCurve(*(ALCHelper::curveDataFromFunction(fittedFunc, xValues)));

    MatrixWorkspace_const_sptr correctedData = m_model->correctedData();
    assert(correctedData->getNumberHistograms() == 1);

    m_view->setCorrectedCurve(*(ALCHelper::curveDataFromWs(correctedData, 0)));
  }

  /**
   * Adds new section in the view
   */
  void ALCBaselineModellingPresenter::addSection()
  {
    double initStart = m_model->data()->getXMin();
    double initEnd = m_model->data()->getXMax();

    auto sections = m_view->sections();
    sections.push_back(IALCBaselineModellingView::Section(initStart, initEnd));
    m_view->setSections(sections);
    m_view->setSectionSelectors(sections);
  }

  /**
   * @param index :: Index of the section to remove. Should be valid.
   */
  void ALCBaselineModellingPresenter::removeSection(size_t index)
  {
    auto sections = m_view->sections();
    assert(index < sections.size()); // The view should take care of that

    sections.erase(sections.begin() + index);

    m_view->setSections(sections);
    m_view->setSectionSelectors(sections);
  }

  void ALCBaselineModellingPresenter::onSectionModified(size_t index, double min, double max)
  {
    m_view->updateSectionSelector(index, min, max);
  }

  void ALCBaselineModellingPresenter::onSectionSelectorModified(size_t index, double min, double max)
  {
    m_view->updateSection(index, min, max);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
