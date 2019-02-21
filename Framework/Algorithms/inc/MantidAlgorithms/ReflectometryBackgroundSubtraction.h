// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTION_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTION_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** ReflectometryBackgroundSubtraction : TODO: DESCRIPTION
*/
class DLLExport ReflectometryBackgroundSubtraction
    : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYBACKGROUNDSUBTRACTION_H_ */