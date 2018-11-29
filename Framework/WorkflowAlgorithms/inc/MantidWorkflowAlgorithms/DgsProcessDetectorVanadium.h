// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_
#define MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsProcessDetectorVanadium : This is the algorithm responsible for
processing the
detector vanadium into the form needed for the normalisation of sample data in
the
convert to energy transfer process.

@date 2012-07-25
 */
class DLLExport DgsProcessDetectorVanadium : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to process detector vanadium.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_DGSPROCESSDETECTORVANADIUM_H_ */
