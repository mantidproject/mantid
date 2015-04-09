#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

  void ALCBaselineModellingModel::fit(IFunction_const_sptr function, const std::vector<Section>& sections)
  {
    // Create a copy of the data
    IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
    clone->setChild(true);
    clone->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    clone->setProperty("OutputWorkspace", "__NotUsed__");
    clone->execute();

    Workspace_sptr cloned = clone->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr dataToFit = boost::dynamic_pointer_cast<MatrixWorkspace>(cloned);
    assert(dataToFit); // CloneWorkspace should take care of that

    disableUnwantedPoints(dataToFit, sections);

    IFunction_sptr funcToFit =
        FunctionFactory::Instance().createInitialized(function->asString());

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", funcToFit);
    fit->setProperty("InputWorkspace", dataToFit);
    fit->setProperty("CreateOutput", true);
    fit->execute();

    MatrixWorkspace_sptr fitOutput = fit->getProperty("OutputWorkspace");

    IAlgorithm_sptr extract = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
    extract->setChild(true);
    extract->setProperty("InputWorkspace", fitOutput);
    extract->setProperty("WorkspaceIndex", 2);
    extract->setProperty("OutputWorkspace", "__NotUsed__");
    extract->execute();

    setCorrectedData(extract->getProperty("OutputWorkspace"));
    setFittedFunction(FunctionFactory::Instance().createInitialized(funcToFit->asString()));
    m_sections = sections;
  }

  void ALCBaselineModellingModel::setData(MatrixWorkspace_const_sptr data)
  {
    m_data = data;
    emit dataChanged();

    setCorrectedData(MatrixWorkspace_const_sptr());
    setFittedFunction(IFunction_const_sptr());
  }

  /**
   * Disable points in the workpsace in the way that points which are not included in any of specified
   * sections are not used when fitting given workspace
   * @param ws :: Workspace to disable points in
   * @param sections :: Section we want to use for fitting
   */
  void ALCBaselineModellingModel::disableUnwantedPoints(MatrixWorkspace_sptr ws,
    const std::vector<IALCBaselineModellingModel::Section>& sections)
  {
    // Whether point with particular index should be disabled
    std::vector<bool> toDisable(ws->blocksize(), true);

    // Find points which are in at least one section, and exclude them from disable list
    for (size_t i = 0; i < ws->blocksize(); ++i)
    {
      for (auto it = sections.begin(); it != sections.end(); ++it)
      {
        if ( ws->dataX(0)[i] >= it->first && ws->dataX(0)[i] <= it->second )
        {
          toDisable[i] = false;
          break; // No need to check other sections
        }
      }
    }

    // XXX: Points are disabled by settings their errors to very high value. This makes those
    //      points to have very low weights during the fitting, effectively disabling them.

    const double DISABLED_ERR = std::numeric_limits<double>::max();

    // Disable chosen points
    for (size_t i = 0; i < ws->blocksize(); ++i)
    {
      if (toDisable[i])
      {
        ws->dataE(0)[i] = DISABLED_ERR;
      }
    }
  }

  MatrixWorkspace_sptr ALCBaselineModellingModel::exportWorkspace()
  {
    if ( m_data ) {

      IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
      clone->setChild(true);
      clone->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
      clone->setProperty("OutputWorkspace", "__NotUsed");
      clone->execute();

      Workspace_sptr cloneResult = clone->getProperty("OutputWorkspace");

      Workspace_sptr baseline = ALCHelper::createWsFromFunction(m_fittedFunction, m_data->readX(0));

      IAlgorithm_sptr join1 = AlgorithmManager::Instance().create("ConjoinWorkspaces");
      join1->setChild(true);
      join1->setProperty("InputWorkspace1", cloneResult);
      join1->setProperty("InputWorkspace2", baseline);
      join1->setProperty("CheckOverlapping", false);
      join1->execute();

      MatrixWorkspace_sptr join1Result = join1->getProperty("InputWorkspace1");

      IAlgorithm_sptr join2 = AlgorithmManager::Instance().create("ConjoinWorkspaces");
      join2->setChild(true);
      join2->setProperty("InputWorkspace1", join1Result);
      join2->setProperty("InputWorkspace2", boost::const_pointer_cast<MatrixWorkspace>(m_correctedData));
      join2->setProperty("CheckOverlapping", false);
      join2->execute();

      MatrixWorkspace_sptr result = join2->getProperty("InputWorkspace1");

      TextAxis* yAxis = new TextAxis(result->getNumberHistograms());
      yAxis->setLabel(0,"Data");
      yAxis->setLabel(1,"Baseline");
      yAxis->setLabel(2,"Corrected");
      result->replaceAxis(1,yAxis);

      return result;

    } else {
    
      return MatrixWorkspace_sptr();
    }
  }

  ITableWorkspace_sptr ALCBaselineModellingModel::exportSections()
  {
    if ( !m_sections.empty() ) {

      ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");

      table->addColumn("double", "Start X");
      table->addColumn("double", "End X");

      for(auto it = m_sections.begin(); it != m_sections.end(); ++it)
      {
        TableRow newRow = table->appendRow();
        newRow << it->first << it->second;
      }

      return table;

    } else {

      return ITableWorkspace_sptr();
    }
  }

  ITableWorkspace_sptr ALCBaselineModellingModel::exportModel()
  {
    if ( m_fittedFunction ) {

      ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");

      table->addColumn("str", "Function");

      TableRow newRow = table->appendRow();
      newRow << m_fittedFunction->asString();

      return table;

    } else {
      
      return ITableWorkspace_sptr();
    }
  }

  void ALCBaselineModellingModel::setCorrectedData(MatrixWorkspace_const_sptr data)
  {
    m_correctedData = data;
    emit correctedDataChanged();
  }

  void ALCBaselineModellingModel::setFittedFunction(IFunction_const_sptr function)
  {
    m_fittedFunction = function;
    emit fittedFunctionChanged();
  }

} // namespace CustomInterfaces
} // namespace Mantid
