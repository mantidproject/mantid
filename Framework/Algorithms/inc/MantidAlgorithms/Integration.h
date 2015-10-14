#ifndef MANTID_ALGORITHMS_INTEGRATION_H_
#define MANTID_ALGORITHMS_INTEGRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Takes a workspace as input and sums each spectrum contained within
    it, storing the result as a workspace of spectra with one Y & E value
    and two X values indicating the range which the integration covers.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. Must be a
   histogram. </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Range_lower - The X value to integrate from (default 0)</LI>
    <LI> Range_upper - The X value to integrate to (default max)</LI>
    <LI> StartWorkspaceIndex - Workspace index number to integrate from (default
   0)</LI>
    <LI> EndWorkspaceIndex - Workspace index number to integrate to (default
   max)</LI>
    <LI> IncludePartialBins - If true then partial bins from the beginning and
   end of the input range are also included in the integration (default
   false)</LI>
    </UL>

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
class DLLExport Integration : public API::Algorithm {
public:
  /// Default constructor
  Integration() : API::Algorithm(){};
  /// Destructor
  virtual ~Integration(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "Integration"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integration takes a 2D workspace or an EventWorkspace as input and "
           "sums the data values. Optionally, the range summed can be "
           "restricted in either dimension.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "Arithmetic;Transforms\\Rebin";
  }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  /// Get the input workspace
  API::MatrixWorkspace_const_sptr getInputWorkspace();
  /// Create the outputworkspace
  API::MatrixWorkspace_sptr
  getOutputWorkspace(API::MatrixWorkspace_const_sptr inWS, const int minSpec,
                     const int maxSpec);

  /// Input event workspace
  DataObjects::EventWorkspace_const_sptr inputEventWS;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_INTEGRATION_H_*/
