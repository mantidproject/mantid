// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_
#define MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Sensitivity correction for SANS
*/
class DLLExport SANSSensitivityCorrection : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "SANSSensitivityCorrection";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform SANS sensitivity correction.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\SANS\\UsesPropertyManager";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Check whether we have a processed file of not
  bool fileCheck(const std::string &filePath);

  std::string m_output_message;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSSENSITIVITYCORRECTION_H_*/
