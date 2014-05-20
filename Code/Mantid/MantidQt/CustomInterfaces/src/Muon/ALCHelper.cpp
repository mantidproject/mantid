#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AlgorithmManager.h"

#include "QMessageBox"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace ALCHelper
{
  /**
   * Creates QwtData using X and Y values from the workspace spectra.
   * @param ws :: Workspace with X and Y values to use
   * @param wsIndex :: Workspace index to use
   * @return Pointer to created QwtData
   */
  boost::shared_ptr<QwtData> curveDataFromWs(MatrixWorkspace_const_sptr ws, size_t wsIndex)
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
  boost::shared_ptr<QwtData> curveDataFromFunction(IFunction_const_sptr func,
                                                   const std::vector<double>& xValues)
  {
    MatrixWorkspace_sptr ws = createWsFromFunction(func, xValues);
    return curveDataFromWs(ws, 0);
  }

  /**
   * Creates a single-spectrum workspace filled with function values for given X values
   * @param func :: Function to calculate values
   * @param xValues :: X values to use
   * @return Single-spectrum workspace with calculated function values
   */
  MatrixWorkspace_sptr createWsFromFunction(IFunction_const_sptr func,
                                            const std::vector<double>& xValues)
  {
    auto inputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, xValues.size(), xValues.size()));
    inputWs->dataX(0) = xValues;

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setChild(true); // Don't want workspace in the ADS
    fit->setProperty("Function", func->asString());
    fit->setProperty("InputWorkspace", inputWs);
    fit->setProperty("MaxIterations", 0); // Don't want to fit, just calculate output workspace
    fit->setProperty("CreateOutput", true);
    fit->execute();

    MatrixWorkspace_sptr fitOutput = fit->getProperty("OutputWorkspace");

    IAlgorithm_sptr extract = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
    extract->setChild(true); // Don't want workspace in the ADS
    extract->setProperty("InputWorkspace", fitOutput);
    extract->setProperty("WorkspaceIndex", 1); // "Calc"
    extract->setPropertyValue("OutputWorkspace", "__NotUsed");
    extract->execute();

    return extract->getProperty("OutputWorkspace");
  }

  boost::shared_ptr<QwtData> emptyCurveData()
  {
    QwtArray<double> x,y; // Empty arrays -> empty data
    return boost::make_shared<QwtArrayData>(x,y);
  }

}
} // namespace CustomInterfaces
} // namespace MantidQt
