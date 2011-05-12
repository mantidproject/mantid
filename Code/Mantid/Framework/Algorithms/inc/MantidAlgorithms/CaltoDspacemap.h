#ifndef MANTID_ALGORITHMS_CALTODSPACEMAP_H_
#define MANTID_ALGORITHMS_CALTODSPACEMAP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid
{

namespace Algorithms
{

/** Performs a unit change from TOF to dSpacing, correcting the X values to account for small
    errors in the detector positions.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace whose detectors are to be aligned </LI>
    <LI> CalibrationFile - The file containing the detector offsets </LI>
    </UL>

    @author Vickie Lynch ORNL
    @date 04/03/2011

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport CaltoDspacemap : public API::Algorithm, public API::DeprecatedAlgorithm
{
public:
  CaltoDspacemap();
  virtual ~CaltoDspacemap();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CaltoDspacemap";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction";}

  static void WriteCalibrationFile(std::string calFileName, const std::map<int, Geometry::IDetector_sptr> & allDetectors ,
                                    const std::map<int,double> &offsets, const std::map<int,bool> &selects, std::map<int,int> &groups);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

  void execEvent();

  void CalculateDspaceFromCal(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                    const std::string DFileName,
                                    Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS);

  /// Pointer for an event workspace
  Mantid::DataObjects::EventWorkspace_const_sptr eventW;

};



} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALTODSPACEMAP_H_ */
