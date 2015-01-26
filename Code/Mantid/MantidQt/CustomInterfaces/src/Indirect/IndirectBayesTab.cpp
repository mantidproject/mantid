#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectBayesTab.h"

using MantidQt::MantidWidgets::RangeSelector;

namespace MantidQt
{
	namespace CustomInterfaces
	{

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    IndirectBayesTab::IndirectBayesTab(QWidget * parent) : IndirectTab(parent),
      m_propTree(new QtTreePropertyBrowser())
    {
      m_propTree->setFactoryForManager(m_dblManager, m_dblEdFac);

      // Connect double maneger signals
      connect(m_dblManager, SIGNAL(valueChanged(QtProperty*, double)), this, SLOT(updateProperties(QtProperty*, double)));
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    IndirectBayesTab::~IndirectBayesTab()
    {
    }

    /**
     * Method to build a URL to the appropriate page on the wiki for this tab.
     *
     * @return The URL to the wiki page
     */
    QString IndirectBayesTab::tabHelpURL()
    {
      return "http://www.mantidproject.org/IndirectBayes:" + help();
    }

    /**
     * Emits a signal to run a python script using the method in the parent
     * UserSubWindow
     *
     * @param pyInput :: A string of python code to execute
     */
    void IndirectBayesTab::runPythonScript(const QString& pyInput)
    {
      emit runAsPythonScript(pyInput, true);
    }

    /**
     * Checks the workspace's intrument for a resolution parameter to use as
     * a default for the energy range on the mini plot
     *
     * @param workspace :: Name of the workspace to use
     * @param res :: The retrieved values for the resolution parameter (if one was found)
     */
    bool IndirectBayesTab::getInstrumentResolution(const QString& workspace, std::pair<double,double>& res)
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
      return getInstrumentResolution(ws, res);
    }

    /**
     * Checks the workspace's intrument for a resolution parameter to use as
     * a default for the energy range on the mini plot
     *
     * @param ws :: Pointer to the workspace to use
     * @param res :: The retrieved values for the resolution parameter (if one was found)
     */
    bool IndirectBayesTab::getInstrumentResolution(Mantid::API::MatrixWorkspace_const_sptr ws, std::pair<double,double>& res)
    {
      auto inst = ws->getInstrument();
      auto analyser = inst->getStringParameter("analyser");

      if(analyser.size() > 0)
      {
        auto comp = inst->getComponentByName(analyser[0]);
        auto params = comp->getNumberParameter("resolution", true);

        //set the default instrument resolution
        if(params.size() > 0)
        {
          res = std::make_pair(-params[0], params[0]);
          return true;
        }
      }

      return false;
    }

    /**
     * Set the position of the lower guide on the mini plot
     *
     * @param rs :: Range selector to update
     * @param lower :: The lower guide property in the property browser
     * @param upper :: The upper guide property in the property browser
     * @param value :: The value of the lower guide
     */
    void IndirectBayesTab::updateLowerGuide(RangeSelector* rs, QtProperty* lower, QtProperty* upper, double value)
    {
      // Check if the user is setting the max less than the min
      if(value > m_dblManager->value(upper))
        m_dblManager->setValue(lower, m_dblManager->value(upper));
      else
        rs->setMinimum(value);
    }

    /**
     * Set the position of the upper guide on the mini plot
     *
     * @param rs :: Range selector to update
     * @param lower :: The lower guide property in the property browser
     * @param upper :: The upper guide property in the property browser
     * @param value :: The value of the upper guide
     */
    void IndirectBayesTab::updateUpperGuide(RangeSelector* rs, QtProperty* lower, QtProperty* upper, double value)
    {
      // Check if the user is setting the min greater than the max
      if(value < m_dblManager->value(lower))
        m_dblManager->setValue(upper, m_dblManager->value(lower));
      else
        rs->setMaximum(value);
    }

  }
} // namespace MantidQt
