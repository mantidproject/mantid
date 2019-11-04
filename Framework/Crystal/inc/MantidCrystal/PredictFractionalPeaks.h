// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS_H_
#define MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/**
 *
 */
class DLLExport PredictFractionalPeaks : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "PredictFractionalPeaks"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The offsets can be from hkl values in a range of hkl values or "
           "from peaks in the input PeaksWorkspace";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"PredictPeaks"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }
  /// Return any errors in cross-property validation
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PREDICTFRACTIONALPEAKS */
