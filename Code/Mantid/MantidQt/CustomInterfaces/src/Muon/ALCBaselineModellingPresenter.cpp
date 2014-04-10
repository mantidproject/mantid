#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"

using namespace Mantid::API;

namespace MantidQt
{
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
  }

  /**
   * @param data :: Data we want to fit baseline for
   */
  void ALCBaselineModellingPresenter::setData(MatrixWorkspace_const_sptr data)
  {
    m_model->setData(data);

    assert(data->getNumberHistograms() == 1);

    boost::shared_ptr<QwtData> curveData = curveDataFromWs(data,0);

    m_view->setDataCurve(*curveData);
  }

  /**
   * Perform a fit and updates the view accordingly
   */
  void ALCBaselineModellingPresenter::fit()
  {
    std::vector<IALCBaselineModellingModel::Section> sections;

    for (int i = 0; i < m_view->sectionCount(); ++i)
    {
      sections.push_back(m_view->section(i));
    }

    // TODO: catch exceptions
    m_model->fit(m_view->function(), sections);

    IFunction_const_sptr fittedFunc = m_model->fittedFunction();

    m_view->setFunction(fittedFunc);

    const std::vector<double>& xValues = m_model->data()->readX(0);
    m_view->setBaselineCurve(*(curveDataFromFunction(fittedFunc, xValues)));

    MatrixWorkspace_const_sptr correctedData = m_model->correctedData();
    assert(correctedData->getNumberHistograms() == 1);

    m_view->setCorrectedCurve(*(curveDataFromWs(correctedData, 0)));
  }

  /**
   * Adds new section in the view
   */
  void ALCBaselineModellingPresenter::addSection()
  {
    m_view->addSection(IALCBaselineModellingModel::Section(0,0));
  }

  /**
   * Creates QwtData using X and Y values from the workspace spectra.
   * @param ws :: Workspace with X and Y values to use
   * @param wsIndex :: Workspace index to use
   * @return Pointer to created QwtData
   */
  boost::shared_ptr<QwtData> ALCBaselineModellingPresenter::curveDataFromWs(
      MatrixWorkspace_const_sptr ws, size_t wsIndex)
  {
    const double* x = &ws->readX(wsIndex)[0];
    const double* y = &ws->readY(wsIndex)[0];
    size_t size = ws->blocksize();

    return boost::make_shared<QwtArrayData>(x,y,size);
  }

  /**
   * Creates QwtData with Y values produced by the function for specified X values.
   * @param func :: Function to use
   * @param xValues :: X values which we want Y values for. QwtData will have those as well.
   * @return Pointer to create QwtData
   */
  boost::shared_ptr<QwtData> ALCBaselineModellingPresenter::curveDataFromFunction(
      IFunction_const_sptr func, const std::vector<double>& xValues)
  {
    FunctionDomain1DVector domain(xValues);
    FunctionValues values(domain);

    func->function(domain, values);
    assert(values.size() != 0);

    size_t size = xValues.size();

    return boost::make_shared<QwtArrayData>(&xValues[0], values.getPointerToCalculated(0), size);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
