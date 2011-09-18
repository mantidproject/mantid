#ifndef MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_
#define MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_
/*WIKI* 

The detector offset file created by this algorithm are in the form created by the ARIEL software. The offsets are a correction to the dSpacing values and are applied during the conversion from time-of-flight to dSpacing as follows:

:<math> d = \frac{h}{m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})</math>


==Usage==
'''Python'''
    LoadEmptyInstrument("POWGEN_Definition.xml","POWGEN")
    CreateCalFileByNames("POWGEN","PG3.cal","Group1,Group2,Group3,Group4")
    DspacemaptoCal("POWGEN","PG3_D1370_dspacemap_2010_09_12.dat","PG3.cal")

'''C++'''
    IAlgorithm* alg1 = FrameworkManager::Instance().createAlgorithm("LoadEmptyInstrument");
    alg1->setPropertyValue("Filename", "POWGEN_Definition.xml");
    alg1->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", "POWGEN");
    alg1->execute();
    IAlgorithm* alg2 = FrameworkManager::Instance().createAlgorithm("CreateCalFileByNames");
    alg2->setProperty<MatrixWorkspace_sptr>("InstrumentWorkspace", "POWGEN");
    alg2->setPropertyValue("GroupingFileName", "PG3.cal");
    alg2->setPropertyValue("GroupingNames", "Group1,Group2,Group3,Group4");
    alg2->execute();
    IAlgorithm* alg3 = FrameworkManager::Instance().createAlgorithm("DspacemaptoCal");
    alg3->setProperty<MatrixWorkspace_sptr>("InputWorkspace", "POWGEN");
    alg3->setPropertyValue("DspacemapFile", "PG3_D1370_dspacemap_2010_09_12.dat");
    alg3->setPropertyValue("CalibrationFile", "PG3.cal");
    alg3->execute();





*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid
{

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
class DLLExport DspacemaptoCal : public API::Algorithm, public API::DeprecatedAlgorithm
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

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

};



} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_DSPACEMAPTOCAL_H_ */
