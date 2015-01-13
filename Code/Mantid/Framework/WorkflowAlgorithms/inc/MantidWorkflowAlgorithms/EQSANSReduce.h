#ifndef MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_
#define MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Perform EQSANS data reduction.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport EQSANSReduce : public API::DataProcessorAlgorithm {
public:
  /// (Empty) Constructor
  EQSANSReduce() : API::DataProcessorAlgorithm() {}
  /// Virtual destructor
  virtual ~EQSANSReduce() {}
  /// Algorithm's name for identification. @see Algorithm::name
  virtual const std::string name() const { return "EQSANSReduce"; }
  /// Algorithm's version for identification. @see Algorithm::version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification. @see Algorithm::category
  virtual const std::string category() const {
    return "Workflow\\SANS\\UsesPropertyManager";
  }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform EQSANS data reduction.";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Perform data reduction
  void performReduction(API::Workspace_sptr workspace);
  /// Post-process the reduced data
  API::Workspace_sptr postProcess(API::Workspace_sptr workspace);
  /// Load input file or workspace
  API::Workspace_sptr loadInputData();
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_WORKFLOWALGORITHMS_EQSANSREDUCE_H_*/
