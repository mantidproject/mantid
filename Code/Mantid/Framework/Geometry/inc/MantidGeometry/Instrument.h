#ifndef MANTID_GEOMETRY_INSTRUMENT_H_
#define MANTID_GEOMETRY_INSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

#include "MantidKernel/DateAndTime.h"

#include <string>
#include <map>

namespace Mantid
{
  /// Typedef of a map from detector ID to detector shared pointer.
  typedef std::map<detid_t, Geometry::IDetector_const_sptr> detid2det_map;

  namespace Geometry
  {

    //------------------------------------------------------------------
    // Forward declarations
    //------------------------------------------------------------------
    class XMLInstrumentParameter;
    class ParameterMap;
    class ReferenceFrame;
    /// Convenience typedef
    typedef std::map<std::pair<std::string,const IComponent*>, boost::shared_ptr<XMLInstrumentParameter> > InstrumentParameterCache;

    /**
    Base Instrument Class.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_GEOMETRY_DLL Instrument : public CompAssembly
    {
    public:
      ///String description of the type of component
      virtual std::string type() const { return "Instrument"; }

      Instrument(const boost::shared_ptr<const Instrument> instr, boost::shared_ptr<ParameterMap> map);
      Instrument();
      Instrument(const std::string& name);
      Instrument(const Instrument&);
      ///Virtual destructor
      virtual ~Instrument();

      Instrument* clone() const;

      IComponent_const_sptr getSource() const;
      IObjComponent_const_sptr getChopperPoint(const size_t index = 0) const;
      size_t getNumberOfChopperPoints() const;
      IComponent_const_sptr getSample() const;
      Kernel::V3D getBeamDirection() const;

      IDetector_const_sptr getDetector(const detid_t &detector_id) const;
      const IDetector* getBaseDetector(const detid_t &detector_id) const;
      bool isMonitor(const detid_t &detector_id) const;
      bool isMonitor(const std::set<detid_t> &detector_ids) const;
      bool isDetectorMasked(const detid_t &detector_id) const;
      bool isDetectorMasked(const std::set<detid_t> &detector_ids) const;

      /// Returns a pointer to the geometrical object for the given set of IDs
      IDetector_const_sptr getDetectorG(const std::vector<detid_t> &det_ids) const;

      /// Returns a list of Detectors for the given detectors ids
      std::vector<IDetector_const_sptr> getDetectors(const std::vector<detid_t> &det_ids) const;

      /// Returns a list of Detectors for the given detectors ids
      std::vector<IDetector_const_sptr> getDetectors(const std::set<detid_t> &det_ids) const;

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be 'the' samplePos Component. For now it is assumed that we have
      /// at most one of these.
      void markAsSamplePos(const IComponent*);

      /// Marks a Component which already exists in the instrument to the chopper cache
      void markAsChopperPoint(const ObjComponent *comp);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be 'the' source Component. For now it is assumed that we have
      /// at most one of these.
      void markAsSource(const IComponent*);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be a Detector component by adding it to _detectorCache
      void markAsDetector(const IDetector*);

      /// mark a Component which has already been added to the Instrument (as a child comp.)
      /// to be a monitor and also add it to _detectorCache for possible later retrieval
      void markAsMonitor(IDetector*);

      /// Remove a detector from the instrument
      void removeDetector(IDetector*);

      /// return reference to detector cache 
      void getDetectors(detid2det_map & out_map) const;

      std::vector<detid_t> getDetectorIDs(bool skipMonitors = false) const;

      std::size_t getNumberDetectors(bool skipMonitors = false) const;

      void getMinMaxDetectorIDs(detid_t & min, detid_t & max) const;

      void getDetectorsInBank(std::vector<IDetector_const_sptr> & dets, const std::string & bankName) const;

      /// Returns a list containing the detector ids of monitors
      std::vector<detid_t> getMonitors() const;
      /// Returns the number of monitors
      size_t numMonitors() const;
  
      /// Get the bounding box for this component and store it in the given argument
      void getBoundingBox(BoundingBox& boundingBox) const;

      /// Get pointers to plottable components
      boost::shared_ptr<const std::vector<IObjComponent_const_sptr> > getPlottable() const;

      /// Returns a shared pointer to a component
      boost::shared_ptr<const IComponent> getComponentByID(ComponentID id) const;

      /// Returns pointers to all components encountered with the given name
      std::vector<boost::shared_ptr<const IComponent> > getAllComponentsWithName(const std::string & cname) const;

      /// Get information about the parameters described in the instrument definition file and associated parameter files
      InstrumentParameterCache& getLogfileCache() {return m_logfileCache;}
      const InstrumentParameterCache& getLogfileCache() const {return m_logfileCache;}

      /// Get information about the units used for parameters described in the IDF and associated parameter files
      std::map<std::string, std::string>& getLogfileUnit() {return m_logfileUnit;}

      /// Get the default type of the instrument view. The possible values are:
      /// 3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z
      std::string getDefaultView() const {return m_defaultView;}
      /// Set the default type of the instrument view. The possible values are:
      /// 3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z
      void setDefaultView(const std::string& type);
      /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possibilities are "Z+, Z-, X+, ..."
      std::string getDefaultAxis() const {return m_defaultViewAxis;}
      /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possibilities are "Z+, Z-, X+, ..."
      void setDefaultViewAxis(const std::string &axis) {m_defaultViewAxis = axis;}
      // Allow access by index
      using CompAssembly::getChild;

      /// Pointer to the 'real' instrument, for parametrized instruments
      boost::shared_ptr<const Instrument> baseInstrument() const;

      /// Pointer to the NOT const ParameterMap holding the parameters of the modified instrument components.
      boost::shared_ptr<ParameterMap> getParameterMap() const;

      /// @return the date from which the instrument definition begins to be valid.
      Kernel::DateAndTime getValidFromDate() const { return m_ValidFrom; }

      /// @return the date at which the instrument definition is no longer valid.
      Kernel::DateAndTime getValidToDate() const { return m_ValidTo; }

      /// Set the date from which the instrument definition begins to be valid.
      /// @param val :: date
      void setValidFromDate(const Kernel::DateAndTime & val);

      /// Set the date at which the instrument definition is no longer valid.
      /// @param val :: date
      void setValidToDate(const Kernel::DateAndTime & val) { m_ValidTo = val; }

      // Methods for use with indirect geometry instruments,
      // where the physical instrument differs from the 'neutronic' one
      boost::shared_ptr<const Instrument> getPhysicalInstrument() const;
      void setPhysicalInstrument(boost::shared_ptr<const Instrument>);

      // ----- Useful static functions ------
      static double calcConversion(const double l1, const Kernel::V3D &beamline, const double beamline_norm,
          const Kernel::V3D &samplePos, const IDetector_const_sptr &det, const double offset);

      static double calcConversion(const double l1,
                            const Kernel::V3D &beamline,
                            const double beamline_norm,
                            const Kernel::V3D &samplePos,
                            const boost::shared_ptr<const Instrument> &instrument,
                            const std::vector<detid_t> &detectors,
                            const std::map<detid_t,double> &offsets);

      void getInstrumentParameters(double & l1, Kernel::V3D & beamline,
          double & beamline_norm, Kernel::V3D & samplePos) const;

      void saveNexus(::NeXus::File * file, const std::string & group) const;
      void loadNexus(::NeXus::File * file, const std::string & group);

      void setFilename(const std::string & filename);
      const std::string & getFilename() const;
      void setXmlText(const std::string & filename);
      const std::string & getXmlText() const;

      /// Set reference Frame
      void setReferenceFrame(boost::shared_ptr<ReferenceFrame> frame);
      /// Get refernce Frame
      boost::shared_ptr<const ReferenceFrame> getReferenceFrame() const;

      /// To determine whether the instrument contains elements of some type
      enum ContainsState {Full, Partial, None};

      /// Check whether instrument contains rectangular detectors.
      /// @return Full if all detectors are rect., Partial if some, None if none 
      ContainsState containsRectDetectors() const;

    private:
      /// Save information about a set of detectors to Nexus
      void saveDetectorSetInfoToNexus (::NeXus::File * file, std::vector<detid_t> detIDs ) const;

      /// Private copy assignment operator
      Instrument& operator=(const Instrument&);

      /// Add a plottable component
      void appendPlottable(const CompAssembly& ca,std::vector<IObjComponent_const_sptr>& lst)const;

      /// Map which holds detector-IDs and pointers to detector components
      std::map<detid_t, IDetector_const_sptr > m_detectorCache;

      /// Purpose to hold copy of source component. For now assumed to be just one component
      const IComponent* m_sourceCache;

      /// Hold a list of places where a chopper can be situated
      /// A pointer so that parameterized instruments are still fast to create.
      std::vector<const ObjComponent*> * m_chopperPoints;

      /// Purpose to hold copy of samplePos component. For now assumed to be just one component
      const IComponent* m_sampleCache;

      /// To store info about the parameters defined in IDF. Indexed according to logfile-IDs, which equals logfile filename minus the run number and file extension
      InstrumentParameterCache m_logfileCache;

      /// Store units used by users to specify angles in IDFs and associated parameter files.
      /// By default this one is empty meaning that the default of angle=degree etc are used
      /// see <http://www.mantidproject.org/IDF>
      /// However if map below contains e.g. <"angle", "radian"> it means
      /// that all "angle"-parameters in the _logfileCache are assumed to have been specified
      /// by the user in radian (not degrees)    
      std::map<std::string, std::string> m_logfileUnit;

      /// a vector holding detector ids of monitor s
      std::vector<detid_t> m_monitorCache;

      /// Stores the default type of the instrument view: 3D or one of the "unwrapped"
      std::string m_defaultView;
      /// Stores from which side the instrument will be viewed from, initially in the instrument viewer, possibilities are "Z+, Z-, X+, ..."
      std::string m_defaultViewAxis;

      /// Pointer to the "real" instrument, for parametrized Instrument
      boost::shared_ptr<const Instrument> m_instr;

      /// Non-const pointer to the parameter map
      boost::shared_ptr<ParameterMap> m_map_nonconst;

      /// the date from which the instrument definition begins to be valid.
      Kernel::DateAndTime m_ValidFrom;
      /// the date at which the instrument definition is no longer valid.
      Kernel::DateAndTime m_ValidTo;

      /// Path to the original IDF .xml file that was loaded for this instrument
      mutable std::string m_filename;

      /// Contents of the IDF .xml file that was loaded for this instrument
      mutable std::string m_xmlText;

      /// Pointer to the physical instrument, where this differs from the 'neutronic' one (indirect geometry)
      boost::shared_ptr<const Instrument> m_physicalInstrument;

      /// Pointer to the reference frame object.
      boost::shared_ptr<ReferenceFrame> m_referenceFrame;
    };

    /// Shared pointer to an instrument object
    typedef boost::shared_ptr<Instrument> Instrument_sptr;
    /// Shared pointer to an const instrument object
    typedef boost::shared_ptr<const Instrument> Instrument_const_sptr;

  } // namespace Geometry
} //Namespace Mantid
#endif /*MANTID_GEOMETRY_INSTRUMENT_H_*/
