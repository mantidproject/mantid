#ifndef MANTID_CUSTOMINTERFACES_ALCHELPER_H_
#define MANTID_CUSTOMINTERFACES_ALCHELPER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
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

  /// Create Qwt curve data from a function
  boost::shared_ptr<QwtData> curveDataFromFunction(IFunction_const_sptr func,
                                                 const std::vector<double>& xValues);

  /// Create workspace filled with function values
  MatrixWorkspace_sptr createWsFromFunction(IFunction_const_sptr func,
                                            const std::vector<double>& xValues);

}
} // namespace CustomInterfaces
} // namespace MantidQt

#endif  /* MANTID_CUSTOMINTERFACES_ALCHELPER_H_ */
