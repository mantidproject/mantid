#ifndef MANTID_ALGORITHMS_EQSANSPATCHSENSITIVITY_H_
#define MANTID_ALGORITHMS_EQSANSPATCHSENSITIVITY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Calculate the detector sensitivity and patch the pixels that are masked in a
   second workspace.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport EQSANSPatchSensitivity : public API::Algorithm {
public:
  /// (Empty) Constructor
  EQSANSPatchSensitivity() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSPatchSensitivity() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSPatchSensitivity"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Patch EQSANS sensitivity correction.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSPATCHSENSITIVITY_H_*/
