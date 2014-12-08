#ifndef MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_
#define MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Copies a single spectrum from a 2D Workspace into a new workspace.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input. </LI>
    <LI> OutputWorkspace - The name under which to store the output workspace. </LI>
    <LI> SpectrumIndex - The workspace index number of the spectrum to extract. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 30/06/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport ExtractSingleSpectrum : public API::Algorithm
{
public:
  /// (Empty) Constructor
  ExtractSingleSpectrum() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ExtractSingleSpectrum() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ExtractSingleSpectrum"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Extracts the specified spectrum from a workspace and places it in a new single-spectrum workspace.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Transforms\\Splitting"; }

private:
  
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EXTRACTSINGLESPECTRUM_H_*/
