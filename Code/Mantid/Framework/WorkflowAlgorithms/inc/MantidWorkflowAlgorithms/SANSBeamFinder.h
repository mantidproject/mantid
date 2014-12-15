#ifndef MANTID_ALGORITHMS_SANSBEAMFINDER_H_
#define MANTID_ALGORITHMS_SANSBEAMFINDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  /** Beam Finder for SANS instruments

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

class DLLExport SANSBeamFinder : public API::Algorithm
{
public:
  /// (Empty) Constructor
  SANSBeamFinder() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~SANSBeamFinder() {}
  /// Algorithm's name
  virtual const std::string name() const { return "SANSBeamFinder"; }
  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Beam finder workflow algorithm for SANS instruments.";}
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\SANS\\UsesPropertyManager"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  API::MatrixWorkspace_sptr loadBeamFinderFile(const std::string& beamCenterFile);
  void maskEdges(API::MatrixWorkspace_sptr beamCenterWS, int high, int low, int left, int right);

  boost::shared_ptr<Kernel::PropertyManager> m_reductionManager;
  std::string m_output_message;

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SANSBEAMFINDER_H_*/
