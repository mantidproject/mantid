#ifndef MANTID_GEOMETRY_INSTRUMENT_H_
#define MANTID_GEOMETRY_INSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <string>
#include <map>
#include <ostream>

namespace Mantid
{

  namespace Geometry
  {

    //------------------------------------------------------------------
    // Forward declarations
    //------------------------------------------------------------------
    class XMLlogfile;

    /**
    Base Instrument Class.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Instrument : public Geometry::CompAssembly, public IInstrument
    {
    public:
      ///String description of the type of component
      virtual std::string type() const { return "Instrument"; }

      Instrument(const boost::shared_ptr<Instrument> instr, ParameterMap_sptr map);
      Instrument();
      Instrument(const std::string& name);
      ///Virtual destructor
      virtual ~Instrument() {}

      Geometry::IObjComponent_sptr getSource() const;
      Geometry::IObjComponent_sptr getSample() const;
      Geometry::IDetector_sptr getDetector(const int &detector_id) const;

      /// Returns a pointer to the geometrical object representing the monitor with the given ID
      Geometry::IDetector_sptr getMonitor(const int &detector_id) const;

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be 'the' samplePos Component. For now it is assumed that we have
      /// at most one of these.
      void markAsSamplePos(Geometry::ObjComponent*);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be 'the' source Component. For now it is assumed that we have
      /// at most one of these.
      void markAsSource(Geometry::ObjComponent*);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be a Detector component by adding it to _detectorCache
      void markAsDetector(Geometry::IDetector*);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be a monitor and also add it to _detectorCache for possible later retrieval
      void markAsMonitor(Geometry::IDetector*);

      /// return reference to detector cache 
      std::map<int, Geometry::IDetector_sptr> getDetectors() const;

      /// returns a list containing  detector ids of monitors
      const std::vector<int> getMonitors()const ;
      /// Get the bounding box for this component and store it in the given argument
      void getBoundingBox(BoundingBox& boundingBox) const;

      /// Get pointers to plottable components
      plottables_const_sptr getPlottable() const;

      std::string getName()const{return Geometry::CompAssembly::getName();}

      /// Returns a shared pointer to a component
      boost::shared_ptr<Geometry::IComponent> getComponentByID(Geometry::ComponentID id);

      /// Get information about the parameters described in the instrument definition file
      std::multimap<std::string, boost::shared_ptr<XMLlogfile> >& getLogfileCache() {return _logfileCache;}

      /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possiblities are "Z+, Z-, X+, ..."
      std::string getDefaultAxis() const {return m_defaultViewAxis;}
      /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possiblities are "Z+, Z-, X+, ..."
      void setDefaultViewAxis(const std::string &axis) {m_defaultViewAxis = axis;}
      // Allow access by index
      using CompAssembly::getChild;


      /// Pointer to the 'real' instrument, for parametrized instruments
      boost::shared_ptr<Instrument> baseInstrument() const;

      /// Pointer to the NOT const ParameterMap holding the parameters of the modified instrument components.
      Geometry::ParameterMap_sptr getParameterMap() const;


    private:
      /// Private copy assignment operator
      Instrument& operator=(const Instrument&);
      /// Private copy constructor
      Instrument(const Instrument&);

      /// Static reference to the logger class
      static Kernel::Logger& g_log;

      /// Get a child by name
      Geometry::IComponent* getChild(const std::string& name) const;

      /// Add a plottable component
      void appendPlottable(const Geometry::CompAssembly& ca,std::vector<Geometry::IObjComponent_const_sptr>& lst)const;

      /// Map which holds detector-IDs and pointers to detector components
      std::map<int, Geometry::IDetector*> _detectorCache;

      /// Purpose to hold copy of source component. For now assumed to
      /// be just one component
      Geometry::ObjComponent* _sourceCache;

      /// Purpose to hold copy of samplePos component. For now assumed to
      /// be just one component
      Geometry::ObjComponent* _sampleCache;

      /// To store info about the parameters defined in IDF. Indexed according to logfile-IDs,
      /// which equals logfile filename minus the run number and file extension
      std::multimap<std::string, boost::shared_ptr<XMLlogfile> > _logfileCache;

      /// a vector holding detector ids of monitor s
      std::vector<int> m_monitorCache;

      /// Stores from which side the instrument will be viewed from, initially in the instrument viewer, possiblities are "Z+, Z-, X+, ..."
      std::string m_defaultViewAxis;

      /// Pointer to the "real" instrument, for parametrized Instrument
      boost::shared_ptr<Instrument> m_instr;

      /// Non-const pointer to the parameter map
      ParameterMap_sptr m_map_nonconst;

    };

    /// Shared pointer to an instrument object
    typedef boost::shared_ptr<Instrument> Instrument_sptr;
    /// Shared pointer to an const instrument object
    typedef boost::shared_ptr<const Instrument> Instrument_const_sptr;

  } // namespace Geometry
} //Namespace Mantid
#endif /*MANTID_APIINSTRUMENT_H_*/
