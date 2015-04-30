#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/CompositeFunction.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  void ALCPeakFittingModel::setData(MatrixWorkspace_const_sptr newData)
  {
    m_data = newData;
    emit dataChanged();
  }

  MatrixWorkspace_sptr ALCPeakFittingModel::exportWorkspace()
  {
    if ( m_data && m_fittedPeaks) {

      // Create a new workspace by cloning data one
      IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
      clone->setChild(true); // Don't want workspaces in ADS
      clone->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
      clone->setProperty("OutputWorkspace", "__NotUsed");
      clone->execute();

      Workspace_sptr cloneResult = clone->getProperty("OutputWorkspace");

      // Calculate function values for all data X values
      MatrixWorkspace_sptr peaks = ALCHelper::createWsFromFunction(m_fittedPeaks, m_data->readX(0));

      // Merge two workspaces
      IAlgorithm_sptr join = AlgorithmManager::Instance().create("ConjoinWorkspaces");
      join->setChild(true);
      join->setProperty("InputWorkspace1", cloneResult);
      join->setProperty("InputWorkspace2", peaks);
      join->setProperty("CheckOverlapping", false);
      join->execute();

      MatrixWorkspace_sptr result = join->getProperty("InputWorkspace1");

      // Update axis lables so that it's understandable what's what on workspace data view / plot
      TextAxis* yAxis = new TextAxis(result->getNumberHistograms());
      yAxis->setLabel(0,"Data");
      yAxis->setLabel(1,"FittedPeaks");
      result->replaceAxis(1,yAxis);

      return result;

    } else {
    
      return MatrixWorkspace_sptr();
    }
  }

  ITableWorkspace_sptr ALCPeakFittingModel::exportFittedPeaks()
  {
    if ( m_parameterTable ) {

      return m_parameterTable;

    } else {
    
      return ITableWorkspace_sptr();
    }
  }

  void ALCPeakFittingModel::setFittedPeaks(IFunction_const_sptr fittedPeaks)
  {
    m_fittedPeaks = fittedPeaks;
    emit fittedPeaksChanged();
  }

  void ALCPeakFittingModel::fitPeaks(IFunction_const_sptr peaks)
  {
    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", peaks->asString());
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->setProperty("CreateOutput", true);
    fit->execute();

    m_parameterTable = fit->getProperty("OutputParameters");

    setFittedPeaks(static_cast<IFunction_sptr>(fit->getProperty("Function")));
  }

} // namespace CustomInterfaces
} // namespace MantidQt

