#ifndef ALGORITHMSCORRECTTOFILE_H_
#define ALGORITHMSCORRECTTOFILE_H_

//-----------------------------
// Includes
//----------------------------
#include "MantidAPI/Algorithm.h"

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

   Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

   This file is part of Mantid.

   Mantid is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Mantid is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   File change history is stored at: <https://github.com/mantidproject/mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CorrectToFile : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CorrectToFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Correct data using a file in the LOQ RKH format";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "SANS;CorrectionFunctions";
  }

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
  void doWkspAlgebra(API::MatrixWorkspace_sptr lhs,
                     API::MatrixWorkspace_sptr rhs, const std::string &algName,
                     API::MatrixWorkspace_sptr &result);
};
} // namespace Algorithms
} // namespace Mantid
#endif /*ALGORITHMSCORRECTTOFILE_H_*/
