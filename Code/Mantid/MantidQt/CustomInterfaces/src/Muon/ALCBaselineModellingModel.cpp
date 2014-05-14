#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"

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

    m_correctedData = extract->getProperty("OutputWorkspace");
    m_fittedFunction = FunctionFactory::Instance().createInitialized(funcToFit->asString());
    m_sections = sections;
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

} // namespace CustomInterfaces
} // namespace Mantid
