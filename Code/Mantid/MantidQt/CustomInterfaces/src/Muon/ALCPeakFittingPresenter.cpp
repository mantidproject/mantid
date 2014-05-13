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

    connect(m_view, SIGNAL(fitRequested()), SLOT(fit()));
    connect(m_view, SIGNAL(currentFunctionChanged()), SLOT(onCurrentFunctionChanged()));
    connect(m_view, SIGNAL(peakPickerChanged()), SLOT(onPeakPickerChanged()));

    // We are updating the whole function anyway, so paramName if left out
    connect(m_view, SIGNAL(parameterChanged(QString,QString)), SLOT(onParameterChanged(QString)));
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
    fit->setProperty("Function", m_view->function("")->asString());
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->execute();

    IFunction_sptr fittedFunc = fit->getProperty("Function");
    m_view->setFunction(fittedFunc);

    const Mantid::MantidVec& x = m_data->readX(0);
    m_view->setFittedCurve(*(ALCHelper::curveDataFromFunction(fittedFunc, x)));
  }

  void ALCPeakFittingPresenter::onCurrentFunctionChanged()
  {
    if(auto index = m_view->currentFunctionIndex()) // If any function selected
    {
      IFunction_const_sptr currentFunc = m_view->function(*index);

      if (auto peakFunc = boost::dynamic_pointer_cast<const IPeakFunction>(currentFunc))
      {
        // If peak function selected - update and enable
        m_view->setPeakPicker(peakFunc);
        m_view->setPeakPickerEnabled(true);
        return;
      }
    }

    // Nothing or a non-peak function selected - disable Peak Picker
    m_view->setPeakPickerEnabled(false);
  }

  void ALCPeakFittingPresenter::onPeakPickerChanged()
  {
    auto index = m_view->currentFunctionIndex();

    // If PeakPicker is changed, it should be enabled, which means a peak function should be selected
    // (See onCurrentFunctionChanged)
    assert(index);

    auto peakFunc = m_view->peakPicker();

    // Update all the defined parameters of the peak function
    for (size_t i = 0; i < peakFunc->nParams(); ++i)
    {
      QString paramName = QString::fromStdString(peakFunc->parameterName(i));
      m_view->setParameter(*index, paramName, peakFunc->getParameter(paramName.toStdString()));
    }
  }

  void ALCPeakFittingPresenter::onParameterChanged(const QString& funcIndex)
  {
    auto currentIndex = m_view->currentFunctionIndex();

    // We are interested in parameter changed of the currently selected function only - that's what
    // PeakPicker is showing
    if (currentIndex && *currentIndex == funcIndex)
    {
      if(auto peak = boost::dynamic_pointer_cast<const IPeakFunction>(m_view->function(funcIndex)))
      {
        m_view->setPeakPicker(peak);
      }
    }
  }

} // namespace CustomInterfaces
} // namespace MantidQt
