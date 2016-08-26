#ifndef MANTID_API_MUPARSERUTILS_H_
#define MANTID_API_MUPARSERUTILS_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/muParser_Silent.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
namespace MuParserUtils {
// A map from constant values to their names in the default parser.
extern const MANTID_API_DLL std::map<double, std::string> MUPARSER_CONSTANTS;

std::unique_ptr<mu::Parser> MANTID_API_DLL allocateDefaultMuParser();
mu::Parser MANTID_API_DLL createDefaultMuParser();
} // namespace MuParserUtils
} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MUPARSERUTILS_H_ */
