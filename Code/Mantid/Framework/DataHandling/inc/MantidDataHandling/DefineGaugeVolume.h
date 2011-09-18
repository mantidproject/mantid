#ifndef MANTID_DATAHANDLING_DEFINEGAUGEVOLUME_H_
#define MANTID_DATAHANDLING_DEFINEGAUGEVOLUME_H_
/*WIKI* 


Intended for use on data from engineering beamlines, this algorithm creates a shape object for use as the 'gauge volume' (i.e. the portion of the sample that is visible
to the detectors in a given run) of a larger sample in the
[[AbsorptionCorrection]] algorithm. The sample shape will also need to be defined using, e.g., the [[CreateSampleShape]] algorithm. Shapes are defined using XML descriptions that can be found [[HowToDefineGeometricShape|here]]. 

Internally, this works by attaching the XML string (after validating it) to a property called "GaugeVolume" on the workspace's [[Run]] object.


*WIKI*/

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{

/** 
    This class allows the shape of the sample to be defined by using the allowed XML
    expressions

    @author Russell Taylor, Tessella
    @date 04/10/2010

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
class DLLExport DefineGaugeVolume : public API::Algorithm
{
public:
  /// (Empty) Constructor
  DefineGaugeVolume() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~DefineGaugeVolume() {}
  /// Algorithm's name
  virtual const std::string name() const { return "DefineGaugeVolume"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Engineering"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
};

}
}

#endif /* MANTID_DATAHANDLING_DEFINEGAUGEVOLUME_H_*/
