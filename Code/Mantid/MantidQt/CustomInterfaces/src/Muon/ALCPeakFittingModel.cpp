#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/TableRow.h"

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
  }

  ITableWorkspace_sptr ALCPeakFittingModel::exportFittedPeaks()
  {
    ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");

    table->addColumn("str", "Function");

    TableRow newRow = table->appendRow();
    newRow << m_fittedPeaks->asString();

    return table;
  }

  void ALCPeakFittingModel::fitPeaks(IFunction_const_sptr peaks)
  {
    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true);
    fit->setProperty("Function", peaks->asString());
    fit->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(m_data));
    fit->execute();

    m_fittedPeaks = static_cast<IFunction_sptr>(fit->getProperty("Function"));
    emit fittedPeaksChanged();
  }

} // namespace CustomInterfaces
} // namespace MantidQt

