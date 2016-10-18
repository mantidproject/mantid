#include "MantidKernel/NullValidator.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Kernel {
IValidator_sptr NullValidator::clone() const {
  return boost::make_shared<NullValidator>(*this);
}

/** Always returns valid, that is ""
 *  @returns an empty string
 */
std::string NullValidator::check(const boost::any &) const { return ""; }
} // namespace Kernel
} // namespace Mantid
