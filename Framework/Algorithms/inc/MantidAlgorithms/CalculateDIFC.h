// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEDIFC_H_
#define MANTID_ALGORITHMS_CALCULATEDIFC_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CalculateDIFC : Calculate the DIFC for every pixel
 */
class DLLExport CalculateDIFC : public API::Algorithm {
public:
  /// Algorithms name for identification. @see Algorithm::name
  const std::string name() const override;
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertDiffCal"};
  }
  const std::string category() const override;
  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override;

private:
  void init() override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATEDIFC_H_ */
