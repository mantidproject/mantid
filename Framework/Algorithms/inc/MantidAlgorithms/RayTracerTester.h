// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RAYTRACERTESTER_H_
#define MANTID_ALGORITHMS_RAYTRACERTESTER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Algorithm to test ray tracer by spraying evenly spaced rays around.

  @author Janik Zikovsky
  @date 2011-09-23
*/
class DLLExport RayTracerTester : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "RayTracerTester"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to test ray tracer by spraying evenly spaced rays "
           "around.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Development"; }

  /// Run the algorithm
  void exec() override;

private:
  /// Initialise the properties
  void init() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RAYTRACERTESTER_H_ */
