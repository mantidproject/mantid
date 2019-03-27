// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
std::string NullValidator::check(const boost::any & /*unused*/) const {
  return "";
}
} // namespace Kernel
} // namespace Mantid
