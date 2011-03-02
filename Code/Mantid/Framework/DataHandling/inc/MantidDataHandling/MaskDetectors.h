#ifndef MANTID_DATAHANDLING_MASKDETECTORS_H_
#define MANTID_DATAHANDLING_MASKDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace DataHandling
{
/** An algorithm to mask a detector, or set of detectors.
    The workspace spectra associated with those detectors are zeroed.

    Required Properties:
    <UL>
    <LI> Workspace - The name of the (input & output) Workspace2D on which to perform the algorithm </LI>
    </UL>

    Optional Properties (One should be set. The highest listed below will be used if more than one is.):
    <UL>
    <LI> SpectraList - An ArrayProperty containing a list of spectra to mask </LI>
    <LI> DetectorList - An ArrayProperty containing a list of detector IDs to mask </LI>
    <LI> WorkspaceIndexList - An ArrayProperty containing the workspace indices to mask </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 15/04/2008

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport MaskDetectors : public API::Algorithm
{
public:
  MaskDetectors();
  virtual ~MaskDetectors();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaskDetectors";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Detectors";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();
  void fillIndexListFromSpectra(std::vector<int>& indexList, const std::vector<int>& spectraList,
                                const API::MatrixWorkspace_sptr WS);
  void appendToIndexListFromWS(std::vector<int>& indexList, const API::MatrixWorkspace_sptr maskedWorkspace);
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_MASKDETECTORS_H_*/
