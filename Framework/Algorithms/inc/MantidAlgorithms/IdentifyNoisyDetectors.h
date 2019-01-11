// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS
#define MANTID_ALGORITHMS_IDENTIFYNOISYDETECTORS

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
  Identifies "bad" detectors based on their standard deviation, and how this
  differs from the
  standard deviation of other detectors. Runs through the process three times to
  get a narrower
  view.
  @author Michael Whitty
  @date 24/01/2011
*/
class DLLExport IdentifyNoisyDetectors : public API::Algorithm {
public:
  const std::string name() const override {
    return "IdentifyNoisyDetectors";
  } ///< @return the algorithms name
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm creates a single-column workspace where the Y "
           "values are populated withs 1s and 0s, 0 signifying that the "
           "detector is to be considered 'bad' based on the method described "
           "below.";
  }

  const std::string category() const override {
    return "Diagnostics";
  } ///< @return the algorithms category
  int version() const override {
    return (1);
  } ///< @return version number of algorithm

  const std::vector<std::string> seeAlso() const override {
    return {"CreatePSDBleedMask"};
  }

private:
  void init() override; ///< Initialise the algorithm. Declare properties, etc.
  void exec() override; ///< Executes the algorithm.

  void getStdDev(API::Progress &progress,
                 Mantid::API::MatrixWorkspace_sptr valid,
                 Mantid::API::MatrixWorkspace_sptr values);
};
} // namespace Algorithms
} // namespace Mantid
#endif
