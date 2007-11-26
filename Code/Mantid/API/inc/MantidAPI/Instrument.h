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
 	
 	  Base Instrument Class, very basic at the moment
 		
 			    	
    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
 	    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories
 	
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
	virtual std::string type() const {return "Instrument";}
	
	Instrument();
	Instrument(const std::string& name);
	virtual ~Instrument() {}
	Instrument(const Instrument&);

	Geometry::ObjComponent* getSource();
	Geometry::ObjComponent* getSamplePos();
	Geometry::CompAssembly* getDetectors();
	Geometry::Detector* getDetector(const int &detector_id);

private:
	/// Static reference to the logger class
	static Kernel::Logger& g_log;

	Geometry::Component* getChild(const std::string& name);

	/// a chache of the detectors assembly
	Geometry::CompAssembly* _detectorsCacheValue;
	
};

} // namespace API
} //Namespace Mantid
#endif /*MANTID_KERNEL_INSTRUMENT_H_*/
