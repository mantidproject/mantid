#ifndef MANTID_ALGORITHMS_CALTODSPACEMAP_H_
#define MANTID_ALGORITHMS_CALTODSPACEMAP_H_
/*WIKI* 

This is the inverse of the DspacemaptoCal algorithm.  The detector offset file created by this algorithm are in the form created by the ARIEL software. The offsets are a correction to the dSpacing values and are applied during the conversion from time-of-flight to dSpacing as follows:

:<math> d = \frac{h}{m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})</math>


==Usage==
'''Python'''
    LoadEmptyInstrument("POWGEN_Definition.xml","POWGEN")
    CaltoDspacemap("POWGEN","PG3.cal", "PG3.dat")

'''C++'''
    IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
    alg1->setPropertyValue("Filename", "POWGEN_Definition.xml");
    alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", "POWGEN");
    alg1->execute();
    IAlgorithm* alg2 = FrameworkManager::Instance().createAlgorithm("DspacemaptoCal");
    alg2->setProperty<MatrixWorkspace_sptr>("InputWorkspace", "POWGEN");
    alg2->setPropertyValue("CalibrationFile", "PG3.cal");
    alg2->setPropertyValue("DspacemapFile", "PG3.dat");
    alg2->execute();





*WIKI*/

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
