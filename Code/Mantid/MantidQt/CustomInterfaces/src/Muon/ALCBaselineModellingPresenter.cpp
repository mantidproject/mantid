#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view,
                                                               IALCBaselineModellingModel* model)
    : m_view(view), m_model(model)
  {}

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

    connect(m_model, SIGNAL(dataChanged()), SLOT(updateDataCurve()));
    connect(m_model, SIGNAL(correctedDataChanged()), SLOT(updateCorrectedCurve()));
    connect(m_model, SIGNAL(fittedFunctionChanged()), SLOT(updateFunction()));
    connect(m_model, SIGNAL(fittedFunctionChanged()), SLOT(updateBaselineCurve()));
  }

  /**
   * Perform a fit and updates the view accordingly
   */
  void ALCBaselineModellingPresenter::fit()
  {
    // TODO: catch exceptions
    m_model->fit(m_view->function(), m_view->sections());
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

  void ALCBaselineModellingPresenter::updateDataCurve()
  {
    m_view->setDataCurve(*(ALCHelper::curveDataFromWs(m_model->data(), 0)));
  }

  void ALCBaselineModellingPresenter::updateCorrectedCurve()
  {
    m_view->setCorrectedCurve(*(ALCHelper::curveDataFromWs(m_model->correctedData(), 0)));
  }

  void ALCBaselineModellingPresenter::updateBaselineCurve()
  {
    IFunction_const_sptr fittedFunc = m_model->fittedFunction();
    const std::vector<double>& xValues = m_model->data()->readX(0);
    m_view->setBaselineCurve(*(ALCHelper::curveDataFromFunction(fittedFunc, xValues)));
  }

  void ALCBaselineModellingPresenter::updateFunction()
  {
    m_view->setFunction(m_model->fittedFunction());
  }

} // namespace CustomInterfaces
} // namespace MantidQt
