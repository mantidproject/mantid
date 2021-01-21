// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------
// Includes
//----------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
   Required properties:
   <UL>
   <LI>WorkspaceToCorrect - The input workspace to correct</LI>
   <LI>Filename - The filename containing the data to use</LI>
   <LI>FirstColumnValue - What does the first column of the file denote</LI>
   <LI>WorkspaceOperation - Whether to divide or multiply by the file data</LI>
   <LI>OutputWorkspace - The output workspace to use for the results </LI>
   </UL>

   @author Martyn Gigg, Tessella Support Services plc
   @date 19/01/2009
*/
class MANTID_ALGORITHMS_DLL CorrectToFile : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CorrectToFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Correct data using a file in the LOQ RKH format"; }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS;CorrectionFunctions"; }

private:
  /// used for the progress bar: the, approximate, fraction of processing time
  /// that taken up with loading the file
  static const double LOAD_TIME;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Load in the RKH file for that has the correction information
  API::MatrixWorkspace_sptr loadInFile(const std::string &corrFile);
  /// Multiply or divide the input workspace as specified by the user
  void doWkspAlgebra(const API::MatrixWorkspace_sptr &lhs, const API::MatrixWorkspace_sptr &rhs,
                     const std::string &algName, API::MatrixWorkspace_sptr &result);
};
} // namespace Algorithms
} // namespace Mantid
