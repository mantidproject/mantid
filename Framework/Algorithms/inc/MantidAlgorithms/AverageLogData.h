// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_AVERAGELOGDATA_H_
#define MANTID_ALGORITHMS_AVERAGELOGDATA_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** AverageLogData : TODO: DESCRIPTION
*/
class DLLExport AverageLogData : public API::Algorithm {
public:
  AverageLogData();
  ~AverageLogData() override;

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override {
    return "Computes the proton charge averaged value of a given log.";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_AVERAGELOGDATA_H_ */
