// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_INSTRUMENT_H_
#define MANTID_GEOMETRY_INSTRUMENT_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument_fwd.h"

#include "MantidKernel/DateAndTime.h"

#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Mantid {
/// Typedef of a map from detector ID to detector shared pointer.
using detid2det_map = std::map<detid_t, Geometry::IDetector_const_sptr>;

namespace Geometry {
class ComponentInfo;
class DetectorInfo;
class XMLInstrumentParameter;
class ParameterMap;
class ReferenceFrame;
/// Convenience typedef
using InstrumentParameterCache =
    std::map<std::pair<std::string, const IComponent *>,
             boost::shared_ptr<XMLInstrumentParameter>>;

/**
Base Instrument Class.

@author Nick Draper, ISIS, RAL
@date 26/09/2007
@author Anders Markvardsen, ISIS, RAL
@date 1/4/2008
*/
class MANTID_GEOMETRY_DLL Instrument : public CompAssembly {
public:
  /// String description of the type of component
  std::string type() const override { return "Instrument"; }

  Instrument(const boost::shared_ptr<const Instrument> instr,
             boost::shared_ptr<ParameterMap> map);
  Instrument();
  Instrument(const std::string &name);
  Instrument(const Instrument &);

  Instrument *clone() const override;

  IComponent_const_sptr getSource() const;
  IComponent_const_sptr getSample() const;
  Kernel::V3D getBeamDirection() const;

  IDetector_const_sptr getDetector(const detid_t &detector_id) const;
  const IDetector *getBaseDetector(const detid_t &detector_id) const;
  bool isMonitor(const detid_t &detector_id) const;
  bool isMonitor(const std::set<detid_t> &detector_ids) const;

  /// Returns a pointer to the geometrical object for the given set of IDs
  IDetector_const_sptr getDetectorG(const std::set<detid_t> &det_ids) const;

  /// Returns a list of Detectors for the given detectors ids
  std::vector<IDetector_const_sptr>
  getDetectors(const std::vector<detid_t> &det_ids) const;

  /// Returns a list of Detectors for the given detectors ids
  std::vector<IDetector_const_sptr>
  getDetectors(const std::set<detid_t> &det_ids) const;

  /// mark a Component which has already been added to the Instrument (as a
  /// child comp.)
  /// to be 'the' samplePos Component. For now it is assumed that we have
  /// at most one of these.
  void markAsSamplePos(const IComponent *);

  /// mark a Component which has already been added to the Instrument (as a
  /// child comp.)
  /// to be 'the' source Component. For now it is assumed that we have
  /// at most one of these.
  void markAsSource(const IComponent *);

  /// mark a Component which has already been added to the Instrument (as a
  /// child comp.)
  /// to be a Detector component by adding it to _detectorCache
  void markAsDetector(const IDetector *);
  void markAsDetectorIncomplete(const IDetector *);
  void markAsDetectorFinalize();

  /// mark a Component which has already been added to the Instrument (as a
  /// child comp.)
  /// to be a monitor and also add it to _detectorCache for possible later
  /// retrieval
  void markAsMonitor(const IDetector *);

  /// Remove a detector from the instrument
  void removeDetector(IDetector *);

  /// return reference to detector cache
  void getDetectors(detid2det_map &out_map) const;

  std::vector<detid_t> getDetectorIDs(bool skipMonitors = false) const;

  std::size_t getNumberDetectors(bool skipMonitors = false) const;

  void getMinMaxDetectorIDs(detid_t &min, detid_t &max) const;

  void getDetectorsInBank(std::vector<IDetector_const_sptr> &dets,
                          const IComponent &comp) const;
  void getDetectorsInBank(std::vector<IDetector_const_sptr> &dets,
                          const std::string &bankName) const;

  /// Returns a list containing the detector ids of monitors
  std::vector<detid_t> getMonitors() const;

  /// Get the bounding box for this component and store it in the given argument
  void getBoundingBox(BoundingBox &assemblyBox) const override;

  /// Get pointers to plottable components
  boost::shared_ptr<const std::vector<IObjComponent_const_sptr>>
  getPlottable() const;

  /// Returns a shared pointer to a component
  boost::shared_ptr<const IComponent>
  getComponentByID(const IComponent *id) const;

  /// Returns pointers to all components encountered with the given name
  std::vector<boost::shared_ptr<const IComponent>>
  getAllComponentsWithName(const std::string &cname) const;

  /// Get information about the parameters described in the instrument
  /// definition file and associated parameter files
  InstrumentParameterCache &getLogfileCache() { return m_logfileCache; }
  const InstrumentParameterCache &getLogfileCache() const {
    return m_logfileCache;
  }

  /// Get information about the units used for parameters described in the IDF
  /// and associated parameter files
  std::map<std::string, std::string> &getLogfileUnit() { return m_logfileUnit; }

  /// Get the default type of the instrument view. The possible values are:
  /// 3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y,
  /// SPHERICAL_Z
  std::string getDefaultView() const { return m_defaultView; }
  /// Set the default type of the instrument view. The possible values are:
  /// 3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y,
  /// SPHERICAL_Z
  void setDefaultView(const std::string &type);
  /// Retrieves from which side the instrument to be viewed from when the
  /// instrument viewer first starts, possibilities are "Z+, Z-, X+, ..."
  std::string getDefaultAxis() const { return m_defaultViewAxis; }
  /// Retrieves from which side the instrument to be viewed from when the
  /// instrument viewer first starts, possibilities are "Z+, Z-, X+, ..."
  void setDefaultViewAxis(const std::string &axis) { m_defaultViewAxis = axis; }
  // Allow access by index
  using CompAssembly::getChild;

  /// Pointer to the 'real' instrument, for parametrized instruments
  boost::shared_ptr<const Instrument> baseInstrument() const;

  /// Pointer to the NOT const ParameterMap holding the parameters of the
  /// modified instrument components.
  boost::shared_ptr<ParameterMap> getParameterMap() const;

  /// @return the date from which the instrument definition begins to be valid.
  Types::Core::DateAndTime getValidFromDate() const { return m_ValidFrom; }

  /// @return the date at which the instrument definition is no longer valid.
  Types::Core::DateAndTime getValidToDate() const { return m_ValidTo; }

  /// Set the date from which the instrument definition begins to be valid.
  /// @param val :: date
  void setValidFromDate(const Types::Core::DateAndTime &val);

  /// Set the date at which the instrument definition is no longer valid.
  /// @param val :: date
  void setValidToDate(const Types::Core::DateAndTime &val) { m_ValidTo = val; }

  // Methods for use with indirect geometry instruments,
  // where the physical instrument differs from the 'neutronic' one
  boost::shared_ptr<const Instrument> getPhysicalInstrument() const;
  void setPhysicalInstrument(std::unique_ptr<Instrument>);

  void getInstrumentParameters(double &l1, Kernel::V3D &beamline,
                               double &beamline_norm,
                               Kernel::V3D &samplePos) const;

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  void loadNexus(::NeXus::File *file, const std::string &group);

  void setFilename(const std::string &filename);
  const std::string &getFilename() const;
  void setXmlText(const std::string &XmlText);
  const std::string &getXmlText() const;

  /// Set reference Frame
  void setReferenceFrame(boost::shared_ptr<ReferenceFrame> frame);
  /// Get refernce Frame
  boost::shared_ptr<const ReferenceFrame> getReferenceFrame() const;

  /// To determine whether the instrument contains elements of some type
  enum ContainsState { Full, Partial, None };

  /// Check whether instrument contains rectangular detectors.
  /// @return Full if all detectors are rect., Partial if some, None if none
  ContainsState containsRectDetectors() const;

  bool isMonitorViaIndex(const size_t index) const;
  size_t detectorIndex(const detid_t detID) const;
  boost::shared_ptr<ParameterMap> makeLegacyParameterMap() const;

  bool isEmptyInstrument() const;

  /// Add a component to the instrument
  virtual int add(IComponent *component) override;

  void parseTreeAndCacheBeamline();
  std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
  makeBeamline(ParameterMap &pmap, const ParameterMap *source = nullptr) const;

private:
  /// Save information about a set of detectors to Nexus
  void saveDetectorSetInfoToNexus(::NeXus::File *file,
                                  const std::vector<detid_t> &detIDs) const;

  /// Private copy assignment operator
  Instrument &operator=(const Instrument &);

  /// Add a plottable component
  void appendPlottable(const CompAssembly &ca,
                       std::vector<IObjComponent_const_sptr> &lst) const;

  std::pair<std::unique_ptr<ComponentInfo>, std::unique_ptr<DetectorInfo>>
  makeWrappers(ParameterMap &pmap, const ComponentInfo &componentInfo,
               const DetectorInfo &detectorInfo) const;

  /// Map which holds detector-IDs and pointers to detector components, and
  /// monitor flags.
  std::vector<std::tuple<detid_t, IDetector_const_sptr, bool>> m_detectorCache;

  /// Purpose to hold copy of source component. For now assumed to be just one
  /// component
  const IComponent *m_sourceCache;

  /// Purpose to hold copy of samplePos component. For now assumed to be just
  /// one component
  const IComponent *m_sampleCache;

  /// To store info about the parameters defined in IDF. Indexed according to
  /// logfile-IDs, which equals logfile filename minus the run number and file
  /// extension
  InstrumentParameterCache m_logfileCache;

  /// Store units used by users to specify angles in IDFs and associated
  /// parameter files.
  /// By default this one is empty meaning that the default of angle=degree etc
  /// are used
  /// see <http://www.mantidproject.org/IDF>
  /// However if map below contains e.g. <"angle", "radian"> it means
  /// that all "angle"-parameters in the _logfileCache are assumed to have been
  /// specified
  /// by the user in radian (not degrees)
  std::map<std::string, std::string> m_logfileUnit;

  /// Stores the default type of the instrument view: 3D or one of the
  /// "unwrapped"
  std::string m_defaultView;
  /// Stores from which side the instrument will be viewed from, initially in
  /// the instrument viewer, possibilities are "Z+, Z-, X+, ..."
  std::string m_defaultViewAxis;

  /// Pointer to the "real" instrument, for parametrized Instrument
  boost::shared_ptr<const Instrument> m_instr;

  /// Non-const pointer to the parameter map
  boost::shared_ptr<ParameterMap> m_map_nonconst;

  /// the date from which the instrument definition begins to be valid.
  Types::Core::DateAndTime m_ValidFrom;
  /// the date at which the instrument definition is no longer valid.
  Types::Core::DateAndTime m_ValidTo;

  /// Path to the original IDF .xml file that was loaded for this instrument
  mutable std::string m_filename;

  /// Contents of the IDF .xml file that was loaded for this instrument
  mutable std::string m_xmlText;

  /// Pointer to the physical instrument, where this differs from the
  /// 'neutronic' one (indirect geometry)
  boost::shared_ptr<const Instrument> m_physicalInstrument;

  /// Pointer to the reference frame object.
  boost::shared_ptr<ReferenceFrame> m_referenceFrame;

  /// Pointer to the DetectorInfo object. May be NULL.
  boost::shared_ptr<const DetectorInfo> m_detectorInfo{nullptr};

  /// Pointer to the ComponentInfo object. May be NULL.
  boost::shared_ptr<const ComponentInfo> m_componentInfo{nullptr};

  /// Flag - is this the physical rather than neutronic instrument
  bool m_isPhysicalInstrument{false};
};
namespace Conversion {

MANTID_GEOMETRY_DLL double tofToDSpacingFactor(const double l1, const double l2,
                                               const double twoTheta,
                                               const double offset);

double MANTID_GEOMETRY_DLL
tofToDSpacingFactor(const double l1, const double l2, const double twoTheta,
                    const std::vector<detid_t> &detectors,
                    const std::map<detid_t, double> &offsets);
} // namespace Conversion

} // namespace Geometry
} // Namespace Mantid
#endif /*MANTID_GEOMETRY_INSTRUMENT_H_*/
