#ifndef MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_
#define MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**
    Set up the reduction options for ILL D33 reduction.

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport SetupILLD33Reduction : public API::Algorithm
{
public:
  /// Constructor
	SetupILLD33Reduction() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SetupILLD33Reduction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SetupILLD33Reduction"; }
  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Set up ILL D33 SANS reduction options.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  std::string _findFile(std::string dataRun);
  void setupSensitivity(boost::shared_ptr<Kernel::PropertyManager> reductionManager);
  void setupTransmission(boost::shared_ptr<Kernel::PropertyManager> reductionManager);
  void setupBackground(boost::shared_ptr<Kernel::PropertyManager> reductionManager);

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SETUPEQSANSREDUCTION_H_*/
