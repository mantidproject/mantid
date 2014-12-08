#ifndef MANTID_DATAHANDLING_FINDDETECTORSINSHAPE_H_
#define MANTID_DATAHANDLING_FINDDETECTORSINSHAPE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm for finding which detectors are contained within a user defined shape within the instrument.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D on which to perform the algorithm </LI>
    <LI> ShapeXML - An XML definition of the shape to be projected within the instruemnt of the workspace </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> IncludeMonitors - True/False whether to include monitors in the results</LI>
    </UL>

    Output Properties:
    <UL>
    <LI> DetectorList - An array property containing the detectors ids contained in the shape </LI>
    </UL>

    @author Nick Draper, Tessella plc
    @date 16/02/2009

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FindDetectorsInShape : public API::Algorithm
{
public:
  FindDetectorsInShape();
  virtual ~FindDetectorsInShape();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindDetectorsInShape";};
  /// Summary of algorithms purpose
  virtual const std::string summary() const {return "Used to find which instrument detectors are contained within a user-defined 3-D shape.";}

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Utility";}

private:
  
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_FINDDETECTORSINSHAPE_H_*/
