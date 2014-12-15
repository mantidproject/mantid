#ifndef MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_
#define MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  /**
      Workflow algorithm to compute a patched sensitivity correction for EQSANS.

      Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport ComputeSensitivity : public API::DataProcessorAlgorithm
{
public:
  /// (Empty) Constructor
  ComputeSensitivity() : API::DataProcessorAlgorithm() {}
  /// Virtual destructor
  virtual ~ComputeSensitivity() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ComputeSensitivity"; }
  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Workflow to calculate EQSANS sensitivity correction.";}
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS\\UsesPropertyManager"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_COMPUTESENSITIVITY_H_*/
