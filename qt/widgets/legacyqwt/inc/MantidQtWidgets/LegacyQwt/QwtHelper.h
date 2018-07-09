#ifndef MANTID_API_QWTHELPER_H_
#define MANTID_API_QWTHELPER_H_

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include <qwt_data.h>

namespace MantidQt {
namespace API {
namespace QwtHelper {
/// Create Qwt curve data from a workspace
EXPORT_OPT_MANTIDQT_LEGACYQWT boost::shared_ptr<QwtData>
curveDataFromWs(Mantid::API::MatrixWorkspace_const_sptr ws, size_t wsIndex);

/// Create vector of Qwt curve data from a workspace, used for EnggDiffraction
/// GUI
EXPORT_OPT_MANTIDQT_LEGACYQWT std::vector<boost::shared_ptr<QwtData>>
curveDataFromWs(Mantid::API::MatrixWorkspace_const_sptr ws);

/// Create error vector from a workspace
EXPORT_OPT_MANTIDQT_LEGACYQWT std::vector<double>
curveErrorsFromWs(Mantid::API::MatrixWorkspace_const_sptr ws, size_t wsIndex);

/// Create Qwt curve data from a function
EXPORT_OPT_MANTIDQT_LEGACYQWT boost::shared_ptr<QwtData>
curveDataFromFunction(Mantid::API::IFunction_const_sptr func,
                      const std::vector<double> &xValues);

/// Create workspace filled with function values
EXPORT_OPT_MANTIDQT_LEGACYQWT Mantid::API::MatrixWorkspace_sptr
createWsFromFunction(Mantid::API::IFunction_const_sptr func,
                     const std::vector<double> &xValues);

/// Creates empty Qwt curve data
EXPORT_OPT_MANTIDQT_LEGACYQWT boost::shared_ptr<QwtData> emptyCurveData();

} // namespace QwtHelper
} // namespace API
} // namespace MantidQt

#endif /* MANTID_API_QWTHELPER_H_ */
