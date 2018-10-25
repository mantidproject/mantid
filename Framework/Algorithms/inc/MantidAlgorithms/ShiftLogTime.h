// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SHIFTLOGTIME_H_
#define MANTID_ALGORITHMS_SHIFTLOGTIME_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ShiftLogTime : TODO: DESCRIPTION

  @date 2011-08-26
*/
class DLLExport ShiftLogTime : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateLogTimeCorrection", "ChangePulsetime", "ChangeLogTime"};
  }
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Shifts the indexes of the specified log. This will make the log "
           "shorter by the specified shift.";
  }

private:
  /// Sets documentation strings for this algorithm

  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SHIFTLOGTIME_H_ */
