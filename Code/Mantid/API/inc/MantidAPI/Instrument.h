#ifndef MANTID_KERNEL_INSTRUMENT_H_
#define MANTID_KERNEL_INSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "CompAssembly.h"
#include "ObjComponent.h"
#include "Detector.h"
#include <string>
#include <ostream> 

namespace Mantid
{
namespace API
{
/** @class Instrument Instrument.h
 	
 	  Base Instrument Class.
 			    	
    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory
 	
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
class DLLExport Instrument : public Geometry::CompAssembly
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "Instrument"; }

  Instrument();
  Instrument(const std::string& name);
  ///Virtual destructor
  virtual ~Instrument() {}

  Geometry::ObjComponent* getSource();
  Geometry::ObjComponent* getSamplePos();
  Geometry::IDetector* getDetector(const int &detector_id);
  void detectorLocation(const int &detector_id, double &l2, double &twoTheta);

  /// mark a Component which has already been added to the Instrument class
  /// to be 'the' samplePos Component. For now it is assumed that we have
  /// at most one of these.
  void markAsSamplePos(Geometry::ObjComponent*);

  /// mark a Component which has already been added to the Instrument class
  /// to be 'the' source Component. For now it is assumed that we have
  /// at most one of these.
  void markAsSource(Geometry::ObjComponent*);

  /// mark a Component which has already been added to the Instrument class
  /// to be a Detector component, and add it to a detector cache for possible
  /// later retrievel
  void markAsDetector(Geometry::IDetector*);

private:
  /// Private copy assignment operator
  Instrument& operator=(const Instrument&);
  /// Private copy constructor
  Instrument(const Instrument&);

  /// Static reference to the logger class
  static Kernel::Logger& g_log;

  Geometry::Component* getChild(const std::string& name);

  /// Map which holds detector-IDs and pointers to detector components 
  // May want to change this to an unordered map (hash map) at some point
  std::map<int, Geometry::IDetector*> _detectorCache;

  /// Purpose to hold copy of source component. For now assumed to
  /// be just one component
  Geometry::ObjComponent* _sourceCache;

  /// Purpose to hold copy of samplePos component. For now assumed to
  /// be just one component
  Geometry::ObjComponent* _samplePosCache;
};

} // namespace API
} //Namespace Mantid
#endif /*MANTID_KERNEL_INSTRUMENT_H_*/
