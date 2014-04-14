#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include "MantidAPI/FunctionDomain1D.h"

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
    FunctionDomain1DVector domain(xValues);
    FunctionValues values(domain);

    func->function(domain, values);
    assert(values.size() != 0);

    size_t size = xValues.size();

    return boost::make_shared<QwtArrayData>(&xValues[0], values.getPointerToCalculated(0), size);
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
