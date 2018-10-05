// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RESIZERECTANGULARDETECTOR_H_
#define MANTID_ALGORITHMS_RESIZERECTANGULARDETECTOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ResizeRectangularDetector : TODO: DESCRIPTION

  @date 2011-11-22
*/
class DLLExport ResizeRectangularDetector : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Resize a RectangularDetector in X and/or Y.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ModifyDetectorDotDatFile"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_RESIZERECTANGULARDETECTOR_H_ */