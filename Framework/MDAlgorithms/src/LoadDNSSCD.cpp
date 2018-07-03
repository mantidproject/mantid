#include <map>
#include <iterator>
#include <iomanip>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include "MantidMDAlgorithms/LoadDNSSCD.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/MDWSTransform.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventInserter.h"

//========================
// helper functions
namespace {
void eraseSubStr(std::string &str, const std::string &toErase) {
  // Search for the substring in string
  size_t pos = str.find(toErase);
  if (pos != std::string::npos) {
    // If found then erase it from string
    str.erase(pos, toErase.length());
  }
}

std::string parseTime(std::string &str) {
  // remove unnecessary symbols
  eraseSubStr(str, "#");
  eraseSubStr(str, "start");
  eraseSubStr(str, "stopped");
  eraseSubStr(str, "at");
  auto it = std::find_if(str.begin(), str.end(), [](char ch) {
    return !std::isspace<char>(ch, std::locale::classic());
  });
  str.erase(str.begin(), it);
  using namespace boost::posix_time;
  // try to parse as a posix time
  try {
    auto time = time_from_string(str);
    return to_iso_extended_string(time);
  } catch (std::exception &) {
    int tzd;
    Poco::DateTime dt;
    bool ok = Poco::DateTimeParser::tryParse(str, dt, tzd);
    if (ok) {
      auto time = Poco::DateTimeFormatter::format(dt, "%Y-%m-%dT%H:%M:%S");
      return time;
    }
    std::string result("");
    return result;
  }
}

} // anonymous namespace
//============================

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

DECLARE_FILELOADER_ALGORITHM(LoadDNSSCD)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
LoadDNSSCD::LoadDNSSCD() : m_nDims(3) {}

/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadDNSSCD::confidence(Kernel::FileDescriptor &descriptor) const {
  // DNS data acquisition writes ascii files with .d_dat extension
  int confidence(0);
  if ((descriptor.extension() == ".d_dat") && descriptor.isAscii()) {
    confidence = 80;
  }
  return confidence;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadDNSSCD::init() {
  std::vector<std::string> exts(1, ".d_dat");
  declareProperty(Kernel::make_unique<MultipleFileProperty>("Filenames", exts),
                  "Select one or more DNS SCD .d_dat files to load."
                  "Files must be measured at the same conditions.");

  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output MDEventWorkspace.");

  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "NormalizationWorkspace", "", Direction::Output),
                  "An output normalization MDEventWorkspace.");

  const std::vector<std::string> normOptions = {"monitor", "time"};
  declareProperty("Normalization", "monitor",
                  boost::make_shared<StringListValidator>(normOptions),
                  "Algorithm will create a separate normalization workspace. "
                  "Choose whether it should contain monitor counts or time.");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  auto reasonableAngle = boost::make_shared<BoundedValidator<double>>();
  reasonableAngle->setLower(5.0);
  reasonableAngle->setUpper(175.0);
  // clang-format off
  auto mustBe3D = boost::make_shared<ArrayLengthValidator<double> >(3);
  auto mustBe2D = boost::make_shared<ArrayLengthValidator<double> >(2);
  // clang-format on
  std::vector<double> u0(3, 0), v0(3, 0);
  u0[0] = 1.;
  u0[1] = 1.;
  v0[2] = 1.;

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "a", 1.0, mustBePositive, Direction::Input),
                  "Lattice parameter a in Angstrom");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "b", 1.0, mustBePositive, Direction::Input),
                  "Lattice parameter b in Angstrom");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "c", 1.0, mustBePositive, Direction::Input),
                  "Lattice parameter c in Angstrom");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "alpha", 90.0, reasonableAngle, Direction::Input),
                  "Angle between b and c in degrees");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "beta", 90.0, reasonableAngle, Direction::Input),
                  "Angle between a and c in degrees");
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "gamma", 90.0, reasonableAngle, Direction::Input),
                  "Angle between a and b in degrees");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "OmegaOffset", 0.0,
                      boost::make_shared<BoundedValidator<double>>(),
                      Direction::Input),
                  "Angle in degrees between (HKL1) and the beam axis"
                  "if the goniometer is at zero.");
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("HKL1", u0, mustBe3D),
      "Indices of the vector in reciprocal space in the horizontal plane at "
      "angle Omegaoffset, "
      "if the goniometer is at zero.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("HKL2", v0, mustBe3D),
      "Indices of a second vector in reciprocal space in the horizontal plane "
      "not parallel to HKL1");

  std::vector<double> ttl(2, 0);
  ttl[1] = 180.0;
  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>("TwoThetaLimits", ttl,
                                                 mustBe2D),
      "Range (min, max) of scattering angles (2theta, in degrees) to consider. "
      "Everything out of this range will be cut.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "LoadHuberFrom", "", Direction::Input, PropertyMode::Optional),
      "A table workspace to load a list of raw sample rotation angles. "
      "Huber angles given in the data files will be ignored.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
          "SaveHuberTo", "", Direction::Output, PropertyMode::Optional),
      "A workspace name to save a list of raw sample rotation angles.");
}

//----------------------------------------------------------------------------------------------
/** Read Huber angles from a given table workspace.
 */

void LoadDNSSCD::loadHuber(ITableWorkspace_sptr tws) {
  ColumnVector<double> huber = tws->getVector("Huber(degrees)");
  // set huber[0] for each run in m_data
  for (auto &ds : m_data) {
    ds.huber = huber[0];
  }
  // dublicate runs for each huber in the table
  std::vector<ExpData> old(m_data);
  for (size_t i = 1; i < huber.size(); ++i) {
    for (auto &ds : old) {
      ds.huber = huber[i];
      m_data.push_back(ds);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Save Huber angles to a given table workspace.
 */
Mantid::API::ITableWorkspace_sptr LoadDNSSCD::saveHuber() {
  std::vector<double> huber;
  for (auto ds : m_data)
    huber.push_back(ds.huber);
  // remove dublicates
  std::sort(huber.begin(), huber.end());
  huber.erase(unique(huber.begin(), huber.end()), huber.end());

  Mantid::API::ITableWorkspace_sptr huberWS =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  huberWS->addColumn("double", "Huber(degrees)");
  for (size_t i = 0; i < huber.size(); i++) {
    huberWS->appendRow();
    huberWS->cell<double>(i, 0) = huber[i];
  }
  return huberWS;
}
//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadDNSSCD::exec() {
  MultipleFileProperty *multiFileProp =
      dynamic_cast<MultipleFileProperty *>(getPointerToProperty("Filenames"));
  if (!multiFileProp) {
    throw std::logic_error(
        "Filenames property must have MultipleFileProperty type.");
  }
  std::vector<std::string> filenames =
      VectorHelper::flattenVector(multiFileProp->operator()());
  if (filenames.empty())
    throw std::invalid_argument("Must specify at least one filename.");

  // set type of normalization
  std::string normtype = getProperty("Normalization");
  if (normtype == "monitor") {
    m_normtype = "Monitor";
    m_normfactor = 1.0;
  } else {
    m_normtype = "Timer";
    m_normfactor = 0.0; // error for time should be 0
  }

  g_log.notice() << "The normalization workspace will contain " << m_normtype
                 << ".\n";

  ExperimentInfo_sptr expinfo = boost::make_shared<ExperimentInfo>();
  API::Run &run = expinfo->mutableRun();
  for (auto fname : filenames) {
    std::map<std::string, std::string> str_metadata;
    std::map<std::string, double> num_metadata;
    try {
      read_data(fname, str_metadata, num_metadata);
      // if no stop_time, take file_save_time
      std::string time(str_metadata["stop_time"]);
      if (time.empty()) {
        g_log.warning()
            << "stop_time is empty! File save time will be used instead."
            << std::endl;
        time = str_metadata["file_save_time"];
      }
      updateProperties<std::string>(run, str_metadata, time);
      updateProperties<double>(run, num_metadata, time);
    } catch (...) {
      g_log.warning() << "Failed to read file " << fname;
      g_log.warning() << ". This file will be ignored. " << std::endl;
      g_log.debug() << boost::current_exception_diagnostic_information()
                    << std::endl;
    }
  }

  if (m_data.empty())
    throw std::runtime_error(
        "No valid DNS files have been provided. Nothing to load.");

  m_OutWS = MDEventFactory::CreateMDWorkspace(m_nDims, "MDEvent");

  m_OutWS->addExperimentInfo(expinfo);

  // load huber angles from a table workspace if given
  ITableWorkspace_sptr huberWS = getProperty("LoadHuberFrom");
  if (huberWS) {
    g_log.notice() << "Huber angles will be loaded from " << huberWS->getName()
                   << std::endl;
    loadHuber(huberWS);
  }

  // get wavelength
  TimeSeriesProperty<double> *wlprop =
      dynamic_cast<TimeSeriesProperty<double> *>(
          expinfo->run().getProperty("Lambda"));
  // assume, that lambda is in nm
  double wavelength =
      wlprop->minValue() * 10.0; // needed to estimate extents => minValue
  run.addProperty("wavelength", wavelength);
  run.getProperty("wavelength")->setUnits("Angstrom");

  fillOutputWorkspace(wavelength);

  std::string saveHuberTableWS = getProperty("SaveHuberTo");
  if (!saveHuberTableWS.empty()) {
    Mantid::API::ITableWorkspace_sptr huber_table = saveHuber();
    setProperty("SaveHuberTo", huber_table);
  }
  setProperty("OutputWorkspace", m_OutWS);
}

//----------------------------------------------------------------------------------------------

template <class T>
void LoadDNSSCD::updateProperties(API::Run &run,
                                  std::map<std::string, T> &metadata,
                                  std::string time) {
  typename std::map<std::string, T>::iterator it = metadata.begin();
  while (it != metadata.end()) {
    TimeSeriesProperty<T> *timeSeries(nullptr);
    std::string name(it->first);
    std::string units;
    // std::regex does not work for rhel7, thus boost
    boost::regex reg("([-_a-zA-Z]+)\\[(.*)]");
    boost::smatch match;
    if (boost::regex_search(name, match, reg) && match.size() > 2) {
      std::string new_name(match.str(1));
      units.assign(match.str(2));
      name = new_name;
    }
    if (run.hasProperty(name)) {
      timeSeries = dynamic_cast<TimeSeriesProperty<T> *>(run.getLogData(name));
      if (!timeSeries)
        throw std::invalid_argument(
            "Log '" + name +
            "' already exists but the values are a different type.");
    } else {
      timeSeries = new TimeSeriesProperty<T>(name);
      if (!units.empty())
        timeSeries->setUnits(units);
      run.addProperty(timeSeries);
    }
    timeSeries->addValue(time, it->second);
    ++it;
  }
}
//----------------------------------------------------------------------------------------------
/// Create output workspace
void LoadDNSSCD::fillOutputWorkspace(double wavelength) {

  // dimensions
  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "H";
  vec_ID[1] = "K";
  vec_ID[2] = "L";

  std::vector<std::string> dimensionNames(3);
  dimensionNames[0] = "H";
  dimensionNames[1] = "K";
  dimensionNames[2] = "L";

  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem =
      Mantid::Kernel::HKL;

  double a, b, c, alpha, beta, gamma;
  a = getProperty("a");
  b = getProperty("b");
  c = getProperty("c");
  alpha = getProperty("alpha");
  beta = getProperty("beta");
  gamma = getProperty("gamma");
  std::vector<double> u = getProperty("HKL1");
  std::vector<double> v = getProperty("HKL2");

  // estimate extents
  double qmax = 4.0 * M_PI / wavelength;
  std::vector<double> extentMins = {-qmax * a, -qmax * b, -qmax * c};
  std::vector<double> extentMaxs = {qmax * a, qmax * b, qmax * c};

  // Get MDFrame of HKL type with RLU
  auto unitFactory = makeMDUnitFactoryChain();
  auto unit = unitFactory->create(Units::Symbol::RLU.ascii());
  Mantid::Geometry::HKL frame(unit);

  // add dimensions
  for (size_t i = 0; i < m_nDims; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    m_OutWS->addDimension(
        Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
            id, name, frame, static_cast<coord_t>(extentMins[i]),
            static_cast<coord_t>(extentMaxs[i]), 5)));
  }

  // Set coordinate system
  m_OutWS->setCoordinateSystem(coordinateSystem);

  // calculate RUB matrix
  Mantid::Geometry::OrientedLattice o;
  o = Mantid::Geometry::OrientedLattice(a, b, c, alpha, beta, gamma);
  o.setUFromVectors(Mantid::Kernel::V3D(u[0], u[1], u[2]),
                    Mantid::Kernel::V3D(v[0], v[1], v[2]));

  double omega_offset = getProperty("OmegaOffset");
  omega_offset *= -1.0 * deg2rad;
  DblMatrix rotm(3, 3);
  rotm[0][0] = std::cos(omega_offset);
  rotm[0][1] = 0.0;
  rotm[0][2] = std::sin(omega_offset);
  rotm[1][0] = 0.0;
  rotm[1][1] = 1.0;
  rotm[1][2] = 0.0;
  rotm[2][0] = -std::sin(omega_offset);
  rotm[2][1] = 0.0;
  rotm[2][2] = std::cos(omega_offset);

  DblMatrix ub(o.getUB());
  ub = rotm * ub;
  o.setUB(ub);
  DblMatrix ub_inv(ub);
  // invert the UB matrix
  ub_inv.Invert();

  // Creates a new instance of the MDEventInserter to output workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr mdws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(m_OutWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  // create a normalization workspace
  IMDEventWorkspace_sptr normWS = m_OutWS->clone();

  // Creates a new instance of the MDEventInserter to norm workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr normws_mdevt_3 =
      boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(normWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> norm_inserter(
      normws_mdevt_3);

  // scattering angle limits
  std::vector<double> tth_limits = getProperty("TwoThetaLimits");
  double theta_min = tth_limits[0] * deg2rad / 2.0;
  double theta_max = tth_limits[1] * deg2rad / 2.0;

  // Go though each element of m_data to convert to MDEvent
  for (ExpData ds : m_data) {
    uint16_t runindex = 0;
    signal_t norm_signal(ds.norm);
    signal_t norm_error = std::sqrt(m_normfactor * norm_signal);
    double k = 2.0 / ds.wavelength;
    for (size_t i = 0; i < ds.detID.size(); i++) {
      signal_t signal(ds.signal[i]);
      signal_t error = std::sqrt(signal);
      detid_t detid(ds.detID[i]);
      double theta = 0.5 * (ds.detID[i] * 5.0 - ds.deterota) * deg2rad;
      if ((theta > theta_min) && (theta < theta_max)) {
        double omega = (ds.huber - ds.deterota) * deg2rad - theta;
        V3D uphi(-cos(omega), 0, -sin(omega));
        V3D hphi = uphi * k * sin(theta);
        V3D hkl = ub_inv * hphi;
        std::vector<Mantid::coord_t> millerindex(3);
        millerindex[0] = static_cast<float>(hkl.X());
        millerindex[1] = static_cast<float>(hkl.Y());
        millerindex[2] = static_cast<float>(hkl.Z());
        inserter.insertMDEvent(
            static_cast<float>(signal), static_cast<float>(error * error),
            static_cast<uint16_t>(runindex), detid, millerindex.data());

        norm_inserter.insertMDEvent(static_cast<float>(norm_signal),
                                    static_cast<float>(norm_error * norm_error),
                                    static_cast<uint16_t>(runindex), detid,
                                    millerindex.data());
      }
    }
  }
  setProperty("NormalizationWorkspace", normWS);
}

void LoadDNSSCD::read_data(const std::string fname,
                           std::map<std::string, std::string> &str_metadata,
                           std::map<std::string, double> &num_metadata) {
  std::ifstream file(fname);
  std::string line;
  std::string::size_type n;
  std::string s;
  boost::regex reg1("^#\\s+(\\w+):(.*)");
  boost::regex reg2("^#\\s+((\\w+\\s)+)\\s+(-?\\d+(,\\d+)*(\\.\\d+(e\\d+)?)?)");
  boost::smatch match;
  getline(file, line);
  n = line.find("DNS");
  if (n == std::string::npos) {
    throw std::invalid_argument("Not a DNS file");
  }
  // get file save time
  Poco::File pfile(fname);
  Poco::DateTime lastModified = pfile.getLastModified();
  std::string wtime(
      Poco::DateTimeFormatter::format(lastModified, "%Y-%m-%dT%H:%M:%S"));
  str_metadata.insert(std::make_pair("file_save_time", wtime));

  // get file basename
  Poco::Path p(fname);
  str_metadata.insert(std::make_pair("run_number", p.getBaseName()));

  // parse metadata
  while (getline(file, line)) {
    n = line.find("Lambda");
    if (n != std::string::npos) {
      boost::regex re("[\\s]+");
      s = line.substr(5);
      boost::sregex_token_iterator it(s.begin(), s.end(), re, -1);
      boost::sregex_token_iterator reg_end;
      getline(file, line);
      std::string s2 = line.substr(2);
      boost::sregex_token_iterator it2(s2.begin(), s2.end(), re, -1);
      for (; (it != reg_end) && (it2 != reg_end); ++it) {
        std::string token(it->str());
        if (token.find_first_not_of(' ') == std::string::npos) {
          ++it2;
          continue;
        }
        if (token == "Mono") {
          str_metadata.insert(std::make_pair(token, it2->str()));
        } else {
          num_metadata.insert(std::make_pair(token, std::stod(it2->str())));
        }
        ++it2;
      }
    }
    // parse start and stop time
    n = line.find("start");
    if (n != std::string::npos) {
      str_metadata.insert(std::make_pair("start_time", parseTime(line)));
      getline(file, line);
      str_metadata.insert(std::make_pair("stop_time", parseTime(line)));
      getline(file, line);
    }
    if (boost::regex_search(line, match, reg1) && match.size() > 2) {
      str_metadata.insert(std::make_pair(match.str(1), match.str(2)));
    }
    if (boost::regex_search(line, match, reg2) && match.size() > 2) {
      s = match.str(1);
      s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(),
              s.end());
      num_metadata.insert(std::make_pair(s, std::stod(match.str(3))));
    }
    n = line.find("DATA");
    if (n != std::string::npos) {
      break;
    }
  }

  // the algorithm does not work with TOF data for the moment
  std::map<std::string, double>::const_iterator m =
      num_metadata.lower_bound("TOF");
  g_log.debug() << "TOF Channels number: " << m->second << std::endl;
  if (m->second != 1)
    throw std::runtime_error(
        "Algorithm does not support TOF data. TOF Channels number must be 1.");

  ExpData ds;
  ds.deterota = num_metadata["DeteRota"];
  ds.huber = num_metadata["Huber"];
  ds.wavelength = 10.0 * num_metadata["Lambda[nm]"];
  ds.norm = num_metadata[m_normtype];

  // read data array
  getline(file, line);
  int d;
  double x;
  while (file) {
    file >> d >> x;
    ds.detID.push_back(d);
    ds.signal.push_back(x);
  }
  // DNS PA detector bank has only 24 detectors
  ds.detID.resize(24);
  ds.signal.resize(24);
  m_data.push_back(ds);
}

} // namespace MDAlgorithms
} // namespace Mantid
