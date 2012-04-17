#ifndef MANTID_ALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_
#define MANTID_ALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Creates a single spectrum Workspace2D with X,Y, and E copied from an first non-integrated dimension of a IMDHistoWorkspace.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input IMDHistoWorkspace.. </LI>
 <LI> OutputWorkspace - The name of the output matrix workspace. </LI>
 <LI> Normalization -   Signal normalization method: NoNormalization, VolumeNormalization, or NumEventsNormalization</LI>
 </UL>

 @author Roman Tolchenov, Tessella plc
 @date 17/04/2012

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport ConvertMDHistoToMatrixWorkspace : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ConvertMDHistoToMatrixWorkspace() : API::Algorithm()
  {}
  /// Virtual destructor
  virtual ~ConvertMDHistoToMatrixWorkspace()
  {}
  /// Algorithm's name
  virtual const std::string name() const
  { return "ConvertMDHistoToMatrixWorkspace";}
  /// Algorithm's version
  virtual int version() const
  { return (1);}
  /// Algorithm's category for identification
  virtual const std::string category() const
  { return "Utility\\Workspaces";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTMDHISTOTOMATRIXWORKSPACE_H_*/
