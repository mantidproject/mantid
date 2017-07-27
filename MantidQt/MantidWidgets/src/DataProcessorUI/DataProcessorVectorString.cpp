#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorVectorString.h"
namespace MantidQt {
namespace MantidWidgets {
/**
Create string of comma separated list of values from a QStringList
@param param_vec : vector of values
@return string of comma separated list of values
*/
QString vectorString(const QStringList &param) { return param.join(", "); }
}
}
