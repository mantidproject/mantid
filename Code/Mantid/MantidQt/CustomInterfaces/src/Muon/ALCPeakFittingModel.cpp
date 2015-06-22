#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingModel.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/ITableWorkspace.h"
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
    if ( m_data && m_data->getNumberHistograms() > 2 ) {

      return boost::const_pointer_cast<MatrixWorkspace>(m_data);

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
    fit->setProperty("OutputCompositeMembers", true);
    fit->execute();

    m_data = fit->getProperty("OutputWorkspace");
    m_parameterTable = fit->getProperty("OutputParameters");
    setFittedPeaks(static_cast<IFunction_sptr>(fit->getProperty("Function")));
  }

} // namespace CustomInterfaces
} // namespace MantidQt

