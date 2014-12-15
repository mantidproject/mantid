#ifndef MANTID_DATAHANDLING_MASKDETECTORSINSHAPE_H_
#define MANTID_DATAHANDLING_MASKDETECTORSINSHAPE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm for finding masking detectors that are contained within a user defined shape within the instrument.

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
    <LI> DetectorList - An array property containing the detectors ids masked in the shape </LI>
    </UL>

    @author Nick Draper, Tessella plc
    @date 16/02/2009

    Copyright &copy; 2009-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport MaskDetectorsInShape : public API::Algorithm
{
public:
  MaskDetectorsInShape();
  virtual ~MaskDetectorsInShape();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaskDetectorsInShape";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Masking";}
  ///Summary of algorithms purpose
  virtual const std::string summary() const {return "Masks detectors whose centres fall within the given 3D shape.";}
private:
  // Implement abstract Algorithm methods
  void init();
  void exec();

	//internal functions
	std::vector<int> runFindDetectorsInShape(API::MatrixWorkspace_sptr workspace,
		const std::string shapeXML, const bool includeMonitors);
	/// Calls MaskDetectors as a Child Algorithm
	void runMaskDetectors(API::MatrixWorkspace_sptr workspace, const std::vector<int> detectorIds);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MASKDETECTORSINSHAPE_H_*/
