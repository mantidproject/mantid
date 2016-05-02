#ifndef MANTID_CUSTOMINTERFACES_ALCHELPER_H_
#define MANTID_CUSTOMINTERFACES_ALCHELPER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/IFunction.h"

#include "qwt_data.h"

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
namespace ALCHelper
{
  /// Create Qwt curve data from a workspace
  boost::shared_ptr<QwtData> curveDataFromWs(MatrixWorkspace_const_sptr ws, size_t wsIndex);

  /// Create vector of Qwt curve data from a workspace, used for EnggDiffraction GUI
  std::vector<boost::shared_ptr<QwtData>> curveDataFromWs(MatrixWorkspace_const_sptr ws);

  /// Create error vector from a workspace
  std::vector<double> curveErrorsFromWs(MatrixWorkspace_const_sptr ws, size_t wsIndex);

  /// Create Qwt curve data from a function
  boost::shared_ptr<QwtData> curveDataFromFunction(IFunction_const_sptr func,
                                                 const std::vector<double>& xValues);

  /// Create workspace filled with function values
  MatrixWorkspace_sptr createWsFromFunction(IFunction_const_sptr func,
                                            const std::vector<double>& xValues);

  /// Creates empty Qwt curve data
  boost::shared_ptr<QwtData> emptyCurveData();

}
} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCHELPER_H_ */
