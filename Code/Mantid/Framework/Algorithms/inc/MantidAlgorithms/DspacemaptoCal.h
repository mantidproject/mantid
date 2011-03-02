#ifndef MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_
#define MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

namespace Algorithms
{

/** Performs a unit change from TOF to dSpacing, correcting the X values to account for small
    errors in the detector positions.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace whose detectors are to be aligned </LI>
    <LI> OutputWorkspace - The name of the Workspace in which the result of the algorithm will be stored </LI>
    <LI> CalibrationFile - The file containing the detector offsets </LI>
    </UL>

    @author Vickie Lynch ORNL
    @date 04/01/2011

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
class DLLExport DspacemaptoCal : public API::Algorithm
{
public:
  DspacemaptoCal();
  virtual ~DspacemaptoCal();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "DspacemaptoCal";};
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

  void readVulcanAsciiFile(const std::string& fileName, std::map<int,double> & vulcan);
  void readVulcanBinaryFile(const std::string& fileName, std::map<int,double> & vulcan);

  void CalculateOffsetsFromDSpacemapFile(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                    const std::string DFileName, std::string calFileName,
                                    std::map<int,double> &offsets, std::map<int,int> &groups);

  void CalculateOffsetsFromVulcanFactors(Mantid::API::MatrixWorkspace_const_sptr inputWS,
                                    std::string calFileName, std::map<int, double> & vulcan,
                                    std::map<int,double> &offsets, std::map<int,int> &groups);

  /// Pointer for an event workspace
  EventWorkspace_const_sptr eventW;

};



} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_ */
