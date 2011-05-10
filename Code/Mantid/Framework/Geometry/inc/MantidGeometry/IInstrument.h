#ifndef MANTID_GEOMETRY_IINSTRUMENT_H_
#define MANTID_GEOMETRY_IINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/Detector.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>

namespace Mantid
{

namespace Geometry
{

/** IInstrument class. The abstract instrument class it is the base for 
    Instrument and Instrument classes.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IInstrument : public virtual Geometry::ICompAssembly
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "IInstrument"; }

  ///Virtual destructor
  virtual ~IInstrument() {}

  /// Returns a pointer to the geometrical object representing the source
  virtual Geometry::IObjComponent_sptr getSource() const = 0;
  /// Returns a pointer to the geometrical object representing the sample
  virtual Geometry::IObjComponent_sptr getSample() const = 0;
  /// Returns a unit vector pointing in the direction of the beam
  Geometry::V3D getBeamDirection() const;

  /// Returns a pointer to the geometrical object for the detector with the given ID
  virtual Geometry::IDetector_sptr getDetector(const int &detector_id) const = 0;

  /// Fill a vector with all the detectors contained in a named component
  virtual void getDetectorsInBank(std::vector<Geometry::IDetector_sptr> & dets, const std::string & bankName) = 0;

  /// Returns a pointer to the geometrical object representing the monitor with the given ID
  virtual Geometry::IDetector_sptr getMonitor(const int &detector_id) const = 0;

  virtual std::string getName() const = 0;

  /// Returns a shared pointer to a component
  virtual boost::shared_ptr<Geometry::IComponent> getComponentByID(Geometry::ComponentID id) = 0;

  /// Returns a shared pointer to a component
  virtual boost::shared_ptr<const Geometry::IComponent> getComponentByID(Geometry::ComponentID id)const = 0;

  /// Returns a pointer to the first component encountered with the given name
  boost::shared_ptr<Geometry::IComponent> getComponentByName(const std::string & cname);

  /// Returns pointers to all components encountered with the given name
  std::vector<boost::shared_ptr<Geometry::IComponent> > getAllComponentsWithName(const std::string & cname);

  /// return map of detector ID : detector sptr
  virtual void getDetectors(std::map<int, Geometry::IDetector_sptr>  & out_dets) const = 0;

  /// return a vector with a list of the detector IDs
  virtual std::vector<int> getDetectorIDs(bool skipMonitors = false) const = 0;

  /// The type used to deliver the set of plottable components
  typedef std::vector<Geometry::IObjComponent_const_sptr> plottables;
  /// A constant shared pointer to a vector of plotables
  typedef const boost::shared_ptr<const plottables> plottables_const_sptr;
  /// Get pointers to plottable components
  virtual plottables_const_sptr getPlottable() const = 0;

  /// returns a list containing  detector ids of monitors
  virtual  const std::vector<int> getMonitors()const=0;
  /// Retrieves from which side the instrument to be viewed from when the instrument viewer first starts, possiblities are "Z+, Z-, X+, ..."
  virtual std::string getDefaultAxis() const=0;

  virtual void getInstrumentParameters(double & l1, Geometry::V3D & beamline,
      double & beamline_norm, Geometry::V3D & samplePos) const = 0;
};

/// Shared pointer to IInstrument
typedef boost::shared_ptr<IInstrument> IInstrument_sptr;
/// Shared pointer to IInstrument (const version)
typedef boost::shared_ptr<const IInstrument> IInstrument_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_IINSTRUMENT_H_*/
