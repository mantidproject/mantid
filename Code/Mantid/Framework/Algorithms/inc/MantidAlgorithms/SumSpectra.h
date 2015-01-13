#ifndef MANTID_ALGORITHMS_SUMSPECTRA_H_
#define MANTID_ALGORITHMS_SUMSPECTRA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

#include <set>

namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and sums all of the spectra within it maintaining
   the existing bin structure and units.
    The result is stored as a new workspace containing a single spectra.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartSpectrum - Workspace index number to integrate from (default
   0)</LI>
    <LI> EndSpectrum - Workspace index number to integrate to (default max)</LI>
    <LI> IncludeMonitors - Whether to include monitor spectra in the sum
   (default yes)
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 22/01/2009

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport SumSpectra : public API::Algorithm {
public:
  /// Default constructor
  SumSpectra() : API::Algorithm(){};
  /// Destructor
  virtual ~SumSpectra(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SumSpectra"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "The SumSpectra algorithm adds the data values in each time bin "
           "across a range of spectra; the output workspace has a single "
           "spectrum. If the input is an EventWorkspace, the output is also an "
           "EventWorkspace; otherwise it will be a Workspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Grouping"; }

private:
  /// Handle logic for RebinnedOutput workspaces
  void doRebinnedOutput(API::MatrixWorkspace_sptr outputWorkspace,
                        API::Progress &progress, size_t &numSpectra,
                        size_t &numMasked, size_t &numZeros);
  /// Handle logic for Workspace2D workspaces
  void doWorkspace2D(API::MatrixWorkspace_const_sptr localworkspace,
                     API::ISpectrum *outSpec, API::Progress &progress,
                     size_t &numSpectra, size_t &numMasked, size_t &numZeros);

  // Overridden Algorithm methods
  void init();
  void exec();
  void execEvent(DataObjects::EventWorkspace_const_sptr localworkspace,
                 std::set<int> &indices);
  specid_t getOutputSpecId(API::MatrixWorkspace_const_sptr localworkspace);

  /// The output spectrum id
  specid_t m_outSpecId;
  /// The spectrum to start the integration from
  int m_MinSpec;
  /// The spectrum to finish the integration at
  int m_MaxSpec;
  /// Set true to keep monitors
  bool keepMonitors;
  /// numberOfSpectra in the input
  int numberOfSpectra;
  /// Blocksize of the input workspace
  int yLength;
  /// Set of indicies to sum
  std::set<int> indices;

  // if calculateing additional workspace with specially weighted averages is
  // necessary
  bool m_CalculateWeightedSum;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SUMSPECTRA_H_*/
