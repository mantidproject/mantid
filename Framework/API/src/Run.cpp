// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"

#include <nexus/NeXusFile.hpp>

#include <boost/lexical_cast.hpp>
#include <memory>

#include <algorithm>
#include <numeric>

namespace Mantid {
namespace API {

using namespace Kernel;

namespace {
/// The number of log entries summed when adding a run
const int ADDABLES = 12;
/// The names of the log entries summed when adding two runs together
const std::string ADDABLE[ADDABLES] = {
    "tot_prtn_chrg",   "rawfrm",          "goodfrm",         "dur",
    "gd_prtn_chrg",    "uA.hour",         "monitor0_counts", "monitor1_counts",
    "monitor2_counts", "monitor3_counts", "monitor4_counts", "monitor5_counts"};
/// Name of the goniometer log when saved to a NeXus file
const char *GONIOMETER_LOG_NAME = "goniometer";
const char *GONIOMETERS_LOG_NAME = "goniometers";
/// Name of the stored histogram bins log when saved to NeXus
const char *HISTO_BINS_LOG_NAME = "processed_histogram_bins";
const char *PEAK_RADIUS_GROUP = "peak_radius";
const char *INNER_BKG_RADIUS_GROUP = "inner_bkg_radius";
const char *OUTER_BKG_RADIUS_GROUP = "outer_bkg_radius";

/// static logger object
Kernel::Logger g_log("Run");
} // namespace

Run::Run() {
  m_goniometers.clear();
  m_goniometers.push_back(std::make_unique<Geometry::Goniometer>());
}

Run::Run(const Run &other) : LogManager(other), m_histoBins(other.m_histoBins) {
  this->copyGoniometers(other);
}

// Defined as default in source for forward declaration with std::unique_ptr.
Run::~Run() = default;

Run &Run::operator=(const Run &other) {
  LogManager::operator=(other);
  copyGoniometers(other);
  m_histoBins = other.m_histoBins;
  return *this;
}

bool Run::operator==(const Run &other) {
  if (m_goniometers.size() != other.m_goniometers.size())
    return false;
  for (size_t i = 0; i < m_goniometers.size(); i++) {
    if (*m_goniometers[i] != *other.m_goniometers[i])
      return false;
  }
  return LogManager::operator==(other) &&
         this->m_histoBins == other.m_histoBins;
}

bool Run::operator!=(const Run &other) { return !this->operator==(other); }

std::shared_ptr<Run> Run::clone() {
  auto clone = std::make_shared<Run>();
  for (auto property : this->m_manager->getProperties()) {
    clone->addProperty(property->clone());
  }
  clone->copyGoniometers(const_cast<Run &>(*this));
  clone->m_histoBins = this->m_histoBins;
  return clone;
}

//-----------------------------------------------------------------------------------------------
/**
 * Filter out a run by time. Takes out any TimeSeriesProperty log entries
 *outside of the given
 *  absolute time range.
 *
 * Total proton charge will get re-integrated after filtering.
 *
 * @param start :: Absolute start time. Any log entries at times >= to this time
 *are kept.
 * @param stop :: Absolute stop time. Any log entries at times < than this time
 *are kept.
 */
void Run::filterByTime(const Types::Core::DateAndTime start,
                       const Types::Core::DateAndTime stop) {
  LogManager::filterByTime(start, stop);
  // Re-integrate proton charge
  this->integrateProtonCharge();
}

/**
 * Adds just the properties that are safe to add. All time series are
 * merged together and the list of addable properties are added
 * @param rhs The object that is being added to this.
 * @returns A reference to the summed object
 */
Run &Run::operator+=(const Run &rhs) {
  // merge and copy properties where there is no risk of corrupting data
  mergeMergables(*m_manager, *rhs.m_manager);

  // Other properties are added together if they are on the approved list
  for (const auto &name : ADDABLE) {
    if (rhs.m_manager->existsProperty(name)) {
      // get a pointer to the property on the right-hand side workspace
      Property *right = rhs.m_manager->getProperty(name);

      // now deal with the left-hand side
      if (m_manager->existsProperty(name)) {
        Property *left = m_manager->getProperty(name);
        left->operator+=(right);
      } else
        // no property on the left-hand side, create one and copy the
        // right-hand side across verbatim
        m_manager->declareProperty(std::unique_ptr<Property>(right->clone()),
                                   "");
    }
  }
  return *this;
}

//-----------------------------------------------------------------------------------------------
/**
 * Split a run by time (splits the TimeSeriesProperties contained).
 *
 * Total proton charge will get re-integrated after filtering.
 *
 * @param splitter :: TimeSplitterType with the intervals and destinations.
 * @param outputs :: Vector of output runs.
 */
void Run::splitByTime(TimeSplitterType &splitter,
                      std::vector<LogManager *> outputs) const {

  // std::vector<LogManager *> outputsBase(outputs.begin(),outputs.end());
  LogManager::splitByTime(splitter, outputs);

  // Re-integrate proton charge of all outputs
  for (auto output : outputs) {
    if (output) {
      auto run = dynamic_cast<Run *>(output);
      if (run)
        run->integrateProtonCharge();
    }
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Set the good proton charge total for this run
 *  @param charge :: The proton charge in uA.hour
 */
void Run::setProtonCharge(const double charge) {
  const std::string PROTON_CHARGE_UNITS("uA.hour");
  if (!hasProperty(PROTON_CHARGE_LOG_NAME)) {
    addProperty(PROTON_CHARGE_LOG_NAME, charge, PROTON_CHARGE_UNITS);
  } else {
    Kernel::Property *charge_prop = getProperty(PROTON_CHARGE_LOG_NAME);
    charge_prop->setValue(boost::lexical_cast<std::string>(charge));
    charge_prop->setUnits(PROTON_CHARGE_UNITS);
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Retrieve the total good proton charge delivered in this run
 * @return The proton charge in uA.hour
 * @throw Exception::NotFoundError if the proton charge has not been set
 */
double Run::getProtonCharge() const {
  double charge = 0.0;
  if (!m_manager->existsProperty(PROTON_CHARGE_LOG_NAME)) {
    integrateProtonCharge();
  }
  if (m_manager->existsProperty(PROTON_CHARGE_LOG_NAME)) {
    charge = m_manager->getProperty(PROTON_CHARGE_LOG_NAME);
  } else {
    g_log.warning() << PROTON_CHARGE_LOG_NAME
                    << " log was not found. Proton Charge set to 0.0\n";
  }
  return charge;
}

//-----------------------------------------------------------------------------------------------
/**
 * Calculate the total proton charge by integrating up all the entries in the
 * "proton_charge" time series log. This is then saved in the log entry
 * using setProtonCharge().
 * If "proton_charge" is not found, the value is not stored
 */
void Run::integrateProtonCharge(const std::string &logname) const {
  Kernel::TimeSeriesProperty<double> *log = nullptr;

  if (this->hasProperty(logname)) {
    try {
      log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          this->getProperty(logname));
    } catch (Exception::NotFoundError &) {
      g_log.warning(logname +
                    " log was not found. The value of the total proton "
                    "charge has not been set");
      return;
    }
  }

  if (log) {
    const std::vector<double> logValues = log->valuesAsVector();
    double total = std::accumulate(logValues.begin(), logValues.end(), 0.0);
    const std::string &unit = log->units();
    // Do we need to take account of a unit
    if (unit.find("picoCoulomb") != std::string::npos) {
      /// Conversion factor between picoColumbs and microAmp*hours
      const double currentConversion = 1.e-6 / 3600.;
      total *= currentConversion;
    } else if (!unit.empty() && unit != "uAh") {
      g_log.warning(logname +
                    " log has units other than uAh or "
                    "picoCoulombs. The value of the total proton charge has "
                    "been left at the sum of the log values.");
    }
    const_cast<Run *>(this)->setProtonCharge(total);
  } else {
    g_log.warning(
        logname +
        " log was not a time series property. The value of the total proton "
        "charge has not been set");
  }
}

//-----------------------------------------------------------------------------------------------

/**
 * Store the given values as a set of energy bin boundaries. Throws
 *    - an invalid_argument if fewer than 2 values are given;
 *    - an out_of_range error if first value is greater of equal to the last
 * @param histoBins :: A vector of values that are interpreted as bin boundaries
 * from a histogram
 */
void Run::storeHistogramBinBoundaries(const std::vector<double> &histoBins) {
  if (histoBins.size() < 2) {
    std::ostringstream os;
    os << "Run::storeEnergyBinBoundaries - Fewer than 2 values given, size="
       << histoBins.size() << ". Cannot interpret values as bin boundaries.";
    throw std::invalid_argument(os.str());
  }
  if (histoBins.front() >= histoBins.back()) {
    std::ostringstream os;
    os << "Run::storeEnergyBinBoundaries - Inconsistent start & end values "
          "given, size="
       << histoBins.size() << ". Cannot interpret values as bin boundaries.";
    throw std::out_of_range(os.str());
  }
  m_histoBins = histoBins;
}

/**
 * Returns the energy bin boundaries for a given energy value if they have been
 * stored here. Throws a std::runtime_error
 * if the energy bins have not been set and a std::out_of_range error if the
 * input value is out of the stored range
 * @return The bin boundaries for the given energy value
 */
std::pair<double, double>
Run::histogramBinBoundaries(const double value) const {
  if (m_histoBins.empty()) {
    throw std::runtime_error("Run::histogramBoundaries - No energy bins have "
                             "been stored for this run");
  }

  if (value < m_histoBins.front()) {
    std::ostringstream os;
    os << "Run::histogramBinBoundaries- Value lower than first bin boundary. "
          "Value= "
       << value << ", first boundary=" << m_histoBins.front();
    throw std::out_of_range(os.str());
  }
  if (value > m_histoBins.back()) {
    std::ostringstream os;
    os << "Run::histogramBinBoundaries- Value greater than last bin boundary. "
          "Value= "
       << value << ", last boundary=" << m_histoBins.back();
    throw std::out_of_range(os.str());
  }
  const int index = VectorHelper::getBinIndex(m_histoBins, value);
  return std::make_pair(m_histoBins[index], m_histoBins[index + 1]);
}

/**
 * Returns the energy bin boundaries. Throws a std::runtime_error
 * if the energy bins have not been set.
 * @return The bin boundaries vector
 */
std::vector<double> Run::getBinBoundaries() const {
  if (m_histoBins.empty()) {
    throw std::runtime_error("Run::histogramBoundaries - No energy bins have "
                             "been stored for this run");
  }

  return m_histoBins;
}

//-----------------------------------------------------------------------------------------------
/** Return the total memory used by the run object, in bytes.
 */
size_t Run::getMemorySize() const {
  size_t total = LogManager::getMemorySize();
  total += sizeof(Geometry::Goniometer) * m_goniometers.size();
  total += m_histoBins.size() * sizeof(double);
  return total;
}

/** @return A reference to the const Goniometer object for this run */
const Geometry::Goniometer &Run::getGoniometer() const {
  return *m_goniometers[0];
}

/** @return A reference to the non-const Goniometer object for this run */
Geometry::Goniometer &Run::mutableGoniometer() { return *m_goniometers[0]; }

//-----------------------------------------------------------------------------------------------
/**
 * Set the gonoimeter & optionally read the average values from the logs
 * @param goniometer :: A reference to a goniometer
 * @param useLogValues :: If true, recalculate the goniometer using the log
 * values
 */
void Run::setGoniometer(const Geometry::Goniometer &goniometer,
                        const bool useLogValues) {
  auto old = std::move(m_goniometers);
  try {
    m_goniometers.emplace_back(
        std::make_unique<Geometry::Goniometer>(goniometer));
    if (useLogValues)
      calculateAverageGoniometerMatrix();
  } catch (std::runtime_error &) {
    m_goniometers = std::move(old);
    throw;
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Set the gonoimeter & read the individual values from the logs
 * @param goniometer :: A reference to a goniometer
 */
void Run::setGoniometers(const Geometry::Goniometer &goniometer) {
  auto old = std::move(m_goniometers);
  try {
    calculateGoniometerMatrices(goniometer);
  } catch (std::runtime_error &) {
    m_goniometers = std::move(old);
    throw;
  }
}

/** Get the gonimeter rotation matrix, calculated using the
 * previously set Goniometer object as well as the angles
 * loaded in the run (if any).
 *
 * @return 3x3 double rotation matrix
 */
const Mantid::Kernel::DblMatrix &Run::getGoniometerMatrix() const {
  return getGoniometerMatrix(0);
}

//-----------------------------------------------------------------------------------------------
/// @return the number of goniometers's in this Run
size_t Run::getNumGoniometers() const { return m_goniometers.size(); }

//-----------------------------------------------------------------------------------------------
/** Add a new Goniometer to this Run
 *
 * @param goniometer :: goniometer to add
 * @return the index at which it was added
 */
size_t Run::addGoniometer(const Geometry::Goniometer &goniometer) {
  m_goniometers.emplace_back(
      std::make_unique<Geometry::Goniometer>(goniometer));
  return m_goniometers.size() - 1;
}

//-----------------------------------------------------------------------------------------------
/// Remove all goniometers on the Run
void Run::clearGoniometers() { m_goniometers.clear(); }

//-----------------------------------------------------------------------------------------------
/** Get the Goniometer for the given run index
 *
 * @param index :: index of the run to get.
 * @return goniometer
 */
const Geometry::Goniometer &Run::getGoniometer(const size_t index) const {
  if (index >= m_goniometers.size())
    throw std::out_of_range(
        "Run::getGoniometer() const: index is out of range.");
  return *m_goniometers[index];
}

//-----------------------------------------------------------------------------------------------
/** Get the non-const Goniometer for the given run index
 *
 * @param index :: index of the run to get.
 * @return goniometer
 */
Geometry::Goniometer &Run::mutableGoniometer(const size_t index) {
  if (index >= m_goniometers.size())
    throw std::out_of_range(
        "Run::getGoniometer() const: index is out of range.");
  return *m_goniometers[index];
}

/** Get the gonoimeter rotation matrix, calculated using the
 * previously set Goniometer object as well as the angles
 * loaded in the run (if any).
 *
 * @param index :: index of the run to get.
 * @return 3x3 double rotation matrix
 */
const Mantid::Kernel::DblMatrix &
Run::getGoniometerMatrix(const size_t index) const {
  if (index >= m_goniometers.size())
    throw std::out_of_range(
        "Run::getGoniometer() const: index is out of range.");
  return m_goniometers[index]->getR();
}

/** Get a vector of all the gonoimeter rotation matries
 *
 * @return vector of 3x3 double rotation matrix
 */
const std::vector<Kernel::Matrix<double>> Run::getGoniometerMatrices() const {
  std::vector<Kernel::Matrix<double>> goniometers;
  goniometers.reserve(m_goniometers.size());
  for (auto it = m_goniometers.begin(); it != m_goniometers.end(); ++it) {
    goniometers.emplace_back((*it)->getR());
  }
  return goniometers;
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 * @param keepOpen :: If true, leave the file open after saving
 */
void Run::saveNexus(::NeXus::File *file, const std::string &group,
                    bool keepOpen) const {
  LogManager::saveNexus(file, group, true);

  // write the goniometer
  if (m_goniometers.size() == 1)
    m_goniometers[0]->saveNexus(file, GONIOMETER_LOG_NAME);
  else if (m_goniometers.size() > 1) {
    file->makeGroup(GONIOMETERS_LOG_NAME, "NXcollection", true);
    file->writeData("num_goniometer", int(m_goniometers.size()));
    for (size_t i = 0; i < m_goniometers.size(); i++) {
      m_goniometers[i]->saveNexus(file, "goniometer" + std::to_string(i));
    }
    file->closeGroup();
  }

  // write the histogram bins, if there are any
  if (!m_histoBins.empty()) {
    file->makeGroup(HISTO_BINS_LOG_NAME, "NXdata", true);
    file->writeData("value", m_histoBins);
    file->closeGroup();
  }
  if (this->hasProperty("PeakRadius")) {
    const std::vector<double> &values =
        this->getPropertyValueAsType<std::vector<double>>("PeakRadius");

    file->makeGroup(PEAK_RADIUS_GROUP, "NXdata", true);
    file->writeData("value", values);
    file->closeGroup();
  }
  if (this->hasProperty("BackgroundInnerRadius")) {
    file->makeGroup(INNER_BKG_RADIUS_GROUP, "NXdata", true);
    const std::vector<double> &values =
        this->getPropertyValueAsType<std::vector<double>>(
            "BackgroundInnerRadius");
    file->writeData("value", values);
    file->closeGroup();
  }
  if (this->hasProperty("BackgroundOuterRadius")) {
    file->makeGroup(OUTER_BKG_RADIUS_GROUP, "NXdata", true);
    const std::vector<double> &values =
        this->getPropertyValueAsType<std::vector<double>>(
            "BackgroundOuterRadius");
    file->writeData("value", values);
    file->closeGroup();
  }
  if (!keepOpen)
    file->closeGroup();
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open. Empty string to NOT open a group,
 * but
 * load any NXlog in the current open group.
 * @param keepOpen :: If true, then the file is left open after doing to load
 */
void Run::loadNexus(::NeXus::File *file, const std::string &group,
                    bool keepOpen) {

  if (!group.empty()) {
    file->openGroup(group, "NXgroup");
  }
  std::map<std::string, std::string> entries;
  file->getEntries(entries);
  LogManager::loadNexus(file, entries);
  for (const auto &name_class : entries) {
    if (name_class.first == GONIOMETER_LOG_NAME) {
      // Goniometer class
      m_goniometers[0]->loadNexus(file, name_class.first);
    } else if (name_class.first == GONIOMETERS_LOG_NAME) {
      file->openGroup(name_class.first, "NXcollection");
      int num_goniometer;
      file->readData("num_goniometer", num_goniometer);
      m_goniometers.clear();
      m_goniometers.reserve(num_goniometer);
      for (int i = 0; i < num_goniometer; i++) {
        m_goniometers.emplace_back(std::make_unique<Geometry::Goniometer>());
        m_goniometers[i]->loadNexus(file, "goniometer" + std::to_string(i));
      }
      file->closeGroup();
    } else if (name_class.first == HISTO_BINS_LOG_NAME) {
      file->openGroup(name_class.first, "NXdata");
      file->readData("value", m_histoBins);
      file->closeGroup();
    } else if (name_class.first == PEAK_RADIUS_GROUP) {
      file->openGroup(name_class.first, "NXdata");
      std::vector<double> values;
      file->readData("value", values);
      file->closeGroup();
      this->addProperty("PeakRadius", values, true);
    } else if (name_class.first == INNER_BKG_RADIUS_GROUP) {
      file->openGroup(name_class.first, "NXdata");
      std::vector<double> values;
      file->readData("value", values);
      file->closeGroup();
      this->addProperty("BackgroundInnerRadius", values, true);
    } else if (name_class.first == OUTER_BKG_RADIUS_GROUP) {
      file->openGroup(name_class.first, "NXdata");
      std::vector<double> values;
      file->readData("value", values);
      file->closeGroup();
      this->addProperty("BackgroundOuterRadius", values, true);
    } else if (name_class.first == "proton_charge" &&
               !this->hasProperty("proton_charge")) {
      // Old files may have a proton_charge field, single value (not even NXlog)
      double charge;
      file->readData("proton_charge", charge);
      this->setProtonCharge(charge);
    }
  }
  if (!(group.empty() || keepOpen))
    file->closeGroup();

  if (this->hasProperty("proton_charge")) {
    // Old files may have a proton_charge field, single value.
    // Modern files (e.g. SNS) have a proton_charge TimeSeriesProperty.
    PropertyWithValue<double> *charge_log =
        dynamic_cast<PropertyWithValue<double> *>(
            this->getProperty("proton_charge"));
    if (charge_log) {
      this->setProtonCharge(boost::lexical_cast<double>(charge_log->value()));
    }
  }
}

//-----------------------------------------------------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------------------------------------------------

/**
 * Calculate the average goniometer matrix
 */
void Run::calculateAverageGoniometerMatrix() {
  for (size_t i = 0; i < m_goniometers[0]->getNumberAxes(); ++i) {
    const std::string axisName = m_goniometers[0]->getAxis(i).name;
    const double minAngle =
        getLogAsSingleValue(axisName, Kernel::Math::Minimum);
    const double maxAngle =
        getLogAsSingleValue(axisName, Kernel::Math::Maximum);
    const double angle =
        getLogAsSingleValue(axisName, Kernel::Math::TimeAveragedMean);

    if (minAngle != maxAngle &&
        !(std::isnan(minAngle) && std::isnan(maxAngle))) {
      const double lastAngle =
          getLogAsSingleValue(axisName, Kernel::Math::LastValue);
      g_log.warning("Goniometer angle changed in " + axisName + " log from " +
                    boost::lexical_cast<std::string>(minAngle) + " to " +
                    boost::lexical_cast<std::string>(maxAngle) +
                    ".  Used time averaged value = " +
                    boost::lexical_cast<std::string>(angle) + ".");
      if (axisName == "omega") {
        g_log.warning("To set to last angle, replace omega with " +
                      boost::lexical_cast<std::string>(lastAngle) +
                      ": "
                      "SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,"
                      "1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
      } else if (axisName == "chi") {
        g_log.warning("To set to last angle, replace chi with " +
                      boost::lexical_cast<std::string>(lastAngle) +
                      ": "
                      "SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,"
                      "1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
      } else if (axisName == "phi") {
        g_log.warning("To set to last angle, replace phi with " +
                      boost::lexical_cast<std::string>(lastAngle) +
                      ": "
                      "SetGoniometer(Workspace=\'workspace\',Axis0=omega,0,1,0,"
                      "1\',Axis1='chi,0,0,1,1',Axis2='phi,0,1,0,1')");
      }
    }
    m_goniometers[0]->setRotationAngle(i, angle);
  }
}

/**
 * Calculate the goniometer matrixes from logs
 * @param goniometer goniometer with axes names to use
 */
void Run::calculateGoniometerMatrices(Geometry::Goniometer goniometer) {
  if (goniometer.getNumberAxes() == 0)
    throw std::runtime_error(
        "Run::calculateGoniometerMatrices must include axes for goniometer");

  const size_t num_log_values =
      getTimeSeriesProperty<double>(goniometer.getAxis(0).name)->size();

  m_goniometers.clear();
  m_goniometers.reserve(num_log_values);

  for (size_t i = 0; i < num_log_values; ++i)
    m_goniometers.emplace_back(
        std::make_unique<Geometry::Goniometer>(goniometer));

  for (size_t i = 0; i < goniometer.getNumberAxes(); ++i) {
    const auto angles =
        getTimeSeriesProperty<double>(goniometer.getAxis(i).name)
            ->valuesAsVector();
    if (angles.size() != num_log_values)
      throw std::runtime_error("Run::calculateGoniometerMatrices different "
                               "number of log entries between axes");

    for (size_t j = 0; j < num_log_values; ++j) {
      m_goniometers[j]->setRotationAngle(i, angles[j]);
    }
  }
}

/** Adds all the time series in the second property manager to those in the
 * first
 * @param sum the properties to add to
 * @param toAdd the properties to add
 */
void Run::mergeMergables(Mantid::Kernel::PropertyManager &sum,
                         const Mantid::Kernel::PropertyManager &toAdd) {
  // get pointers to all the properties on the right-handside and prepare to
  // loop through them
  const std::vector<Property *> &inc = toAdd.getProperties();
  for (auto ptr : inc) {
    const std::string &rhs_name = ptr->name();
    try {
      // now get pointers to the same properties on the left-handside
      Property *lhs_prop(sum.getProperty(rhs_name));
      lhs_prop->merge(ptr);
    } catch (Exception::NotFoundError &) {
      // copy any properties that aren't already on the left hand side
      auto copy = std::unique_ptr<Property>(ptr->clone());
      // And we add a copy of that property to *this
      sum.declareProperty(std::move(copy), "");
    }
  }
}

//-----------------------------------------------------------------------------------------------
/** Copy the goniometers from another
 * @param other :: other workspace to copy    */
void Run::copyGoniometers(const Run &other) {
  m_goniometers.clear();
  m_goniometers.reserve(other.m_goniometers.size());
  for (const auto &goniometer : other.m_goniometers) {
    auto new_goniometer = std::make_unique<Geometry::Goniometer>(*goniometer);
    m_goniometers.emplace_back(std::move(new_goniometer));
  }
}
} // namespace API
} // namespace Mantid
