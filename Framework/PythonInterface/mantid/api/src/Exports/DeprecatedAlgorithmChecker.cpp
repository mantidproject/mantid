// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include <boost/python/class.hpp>

using Mantid::API::AlgorithmManager;
using Mantid::API::DeprecatedAlgorithm;
using Mantid::API::IAlgorithm_sptr;

using namespace boost::python;

namespace {
/**
 * It is not going to be possible to directly test if an algorithm is deprecated
 *from Python. This
 * is because we only export up to API and the deprecation happens at the
 *concrete layer meaning
 * Python cannot work out that the concrete class inherits from
 *DeprecatedAlgorithm.
 *
 * To work around this we create a small DeprecatedAlgorithmTester class to
 *handle to querying the C++
 * inheritance structure
 */
class DeprecatedAlgorithmChecker {
public:
  /**
   * Constructor. Throws if the algorithm does not exist
   * @param algName The name of the algorithm
   * @param version The algorithm version
   */
  DeprecatedAlgorithmChecker(const std::string &algName, int version)
      : m_alg(AlgorithmManager::Instance().createUnmanaged(algName, version)) {}

  /// Check if the algorithm is deprecated
  /// @returns A string containing a deprecation message if the algorithm is
  /// deprecated, empty string otherwise
  const std::string isDeprecated() const {
    std::string deprecMessage;
    auto *depr = dynamic_cast<DeprecatedAlgorithm *>(m_alg.get());
    if (depr)
      deprecMessage = depr->deprecationMsg(m_alg.get());
    return deprecMessage;
  }

private:
  /// Private default constructor
  DeprecatedAlgorithmChecker();
  /// Pointer to unmanaged algorithm
  IAlgorithm_sptr m_alg;
};
} // namespace

void export_DeprecatedAlgorithmChecker() {
  class_<DeprecatedAlgorithmChecker>("DeprecatedAlgorithmChecker", no_init)
      .def(init<const std::string &, int>(
          (arg("algName"), arg("version")),
          "Constructs a DeprecatedAlgorithmChecker for the given algorithm & "
          "version. (-1 indicates latest version)"))
      .def("isDeprecated", &DeprecatedAlgorithmChecker::isDeprecated,
           "A string containing a deprecation message if the algorithm is "
           "deprecated, empty string otherwise");
}
