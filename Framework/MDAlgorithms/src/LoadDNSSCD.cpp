// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/LoadDNSSCD.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/MDWSTransform.h"
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <map>

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
  auto it =
      std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
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

namespace Mantid::MDAlgorithms {

DECLARE_FILELOADER_ALGORITHM(LoadDNSSCD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadDNSSCD::LoadDNSSCD() : m_columnSep("\t, ;"), m_nDims(4), m_tof_max(20000.0) {}

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
  declareProperty(std::make_unique<MultipleFileProperty>("Filenames", exts),
                  "Select one or more DNS SCD .d_dat files to load."
                  "Files must be measured at the same conditions.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output MDEventWorkspace.");

  declareProperty(
      std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("NormalizationWorkspace", "", Direction::Output),
      "An output normalization MDEventWorkspace.");

  const std::vector<std::string> normOptions = {"monitor", "time"};
  declareProperty("Normalization", "monitor", std::make_shared<StringListValidator>(normOptions),
                  "Algorithm will create a separate normalization workspace. "
                  "Choose whether it should contain monitor counts or time.");

  const std::vector<std::string> wsOptions = {"raw", "HKL"};
  declareProperty("LoadAs", "HKL", std::make_shared<StringListValidator>(wsOptions),
                  "Choose whether the algorithm should load raw data"
                  "or convert to H,K,L,dE space");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  auto reasonableAngle = std::make_shared<BoundedValidator<double>>();
  reasonableAngle->setLower(5.0);
  reasonableAngle->setUpper(175.0);
  auto mustBe3D = std::make_shared<ArrayLengthValidator<double>>(3);
  auto mustBe2D = std::make_shared<ArrayLengthValidator<double>>(2);
  std::vector<double> u0(3, 0), v0(3, 0);
  u0[0] = 1.;
  u0[1] = 1.;
  v0[2] = 1.;

  declareProperty(std::make_unique<PropertyWithValue<double>>("a", 1.0, mustBePositive->clone(), Direction::Input),
                  "Lattice parameter a in Angstrom");
  declareProperty(std::make_unique<PropertyWithValue<double>>("b", 1.0, mustBePositive->clone(), Direction::Input),
                  "Lattice parameter b in Angstrom");
  declareProperty(std::make_unique<PropertyWithValue<double>>("c", 1.0, std::move(mustBePositive), Direction::Input),
                  "Lattice parameter c in Angstrom");
  declareProperty(
      std::make_unique<PropertyWithValue<double>>("alpha", 90.0, reasonableAngle->clone(), Direction::Input),
      "Angle between b and c in degrees");
  declareProperty(std::make_unique<PropertyWithValue<double>>("beta", 90.0, reasonableAngle->clone(), Direction::Input),
                  "Angle between a and c in degrees");
  declareProperty(
      std::make_unique<PropertyWithValue<double>>("gamma", 90.0, std::move(reasonableAngle), Direction::Input),
      "Angle between a and b in degrees");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "OmegaOffset", 0.0, std::make_shared<BoundedValidator<double>>(), Direction::Input),
                  "Angle in degrees between (HKL1) and the beam axis"
                  "if the goniometer is at zero.");
  declareProperty(std::make_unique<ArrayProperty<double>>("HKL1", std::move(u0), mustBe3D->clone()),
                  "Indices of the vector in reciprocal space in the horizontal plane at "
                  "angle Omegaoffset, "
                  "if the goniometer is at zero.");

  declareProperty(std::make_unique<ArrayProperty<double>>("HKL2", std::move(v0), std::move(mustBe3D)),
                  "Indices of a second vector in reciprocal space in the horizontal plane "
                  "not parallel to HKL1");

  std::vector<double> ttl(2, 0);
  ttl[1] = 180.0;
  declareProperty(std::make_unique<ArrayProperty<double>>("TwoThetaLimits", std::move(ttl), std::move(mustBe2D)),
                  "Range (min, max) of scattering angles (2theta, in degrees) to consider. "
                  "Everything out of this range will be cut.");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("LoadHuberFrom", "", Direction::Input,
                                                                            PropertyMode::Optional),
                  "A table workspace to load a list of raw sample rotation angles. "
                  "Huber angles given in the data files will be ignored.");

  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>("SaveHuberTo", "", Direction::Output,
                                                                            PropertyMode::Optional),
                  "A workspace name to save a list of raw sample rotation angles.");

  auto mustBeIntPositive = std::make_shared<BoundedValidator<int>>();
  mustBeIntPositive->setLower(0);
  declareProperty(
      std::make_unique<PropertyWithValue<int>>("ElasticChannel", 0, std::move(mustBeIntPositive), Direction::Input),
      "Elastic channel number. Only for TOF data.");

  auto mustBeNegative = std::make_shared<BoundedValidator<double>>();
  mustBeNegative->setUpper(0.0);
  declareProperty(
      std::make_unique<PropertyWithValue<double>>("DeltaEmin", -10.0, std::move(mustBeNegative), Direction::Input),
      "Minimal energy transfer to consider. Should be <=0. Only for TOF data.");
}

//----------------------------------------------------------------------------------------------
/** Read Huber angles from a given table workspace.
 */

void LoadDNSSCD::loadHuber(const ITableWorkspace_sptr &tws) {
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
      m_data.emplace_back(ds);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Save Huber angles to a given table workspace.
 */
Mantid::API::ITableWorkspace_sptr LoadDNSSCD::saveHuber() {
  std::vector<double> huber;
  huber.reserve(m_data.size());
  std::transform(m_data.cbegin(), m_data.cend(), std::back_inserter(huber), [](const auto &ds) { return ds.huber; });
  // remove dublicates
  std::sort(huber.begin(), huber.end());
  huber.erase(unique(huber.begin(), huber.end()), huber.end());

  Mantid::API::ITableWorkspace_sptr huberWS = WorkspaceFactory::Instance().createTable("TableWorkspace");
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
  MultipleFileProperty *multiFileProp = dynamic_cast<MultipleFileProperty *>(getPointerToProperty("Filenames"));
  if (!multiFileProp) {
    throw std::logic_error("Filenames property must have MultipleFileProperty type.");
  }
  std::vector<std::string> filenames = VectorHelper::flattenVector(multiFileProp->operator()());
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

  g_log.notice() << "The normalization workspace will contain " << m_normtype << ".\n";

  ExperimentInfo_sptr expinfo = std::make_shared<ExperimentInfo>();
  API::Run &run = expinfo->mutableRun();
  for (const auto &fname : filenames) {
    std::map<std::string, std::string> str_metadata;
    std::map<std::string, double> num_metadata;
    try {
      read_data(fname, str_metadata, num_metadata);
      // if no stop_time, take file_save_time
      std::string time(str_metadata["stop_time"]);
      if (time.empty()) {
        g_log.warning() << "stop_time is empty! File save time will be used instead." << std::endl;
        time = str_metadata["file_save_time"];
      }
      updateProperties<std::string>(run, str_metadata, time);
      updateProperties<double>(run, num_metadata, time);
    } catch (...) {
      g_log.warning() << "Failed to read file " << fname;
      g_log.warning() << ". This file will be ignored. " << std::endl;
      g_log.debug() << boost::current_exception_diagnostic_information() << std::endl;
    }
  }

  if (m_data.empty())
    throw std::runtime_error("No valid DNS files have been provided. Nothing to load.");

  // merge data with different time channel number is not allowed
  auto ch_n = m_data.front().nchannels;
  bool same_channel_number =
      std::all_of(m_data.cbegin(), m_data.cend(), [ch_n](const ExpData &d) { return (d.nchannels == ch_n); });
  if (!same_channel_number)
    throw std::runtime_error("Error: cannot merge data with different TOF channel numbers.");

  std::string load_as = getProperty("LoadAs");
  if (load_as == "raw")
    m_nDims = 3;

  m_OutWS = MDEventFactory::CreateMDWorkspace(m_nDims, "MDEvent");
  m_OutWS->addExperimentInfo(expinfo);

  // load huber angles from a table workspace if given
  ITableWorkspace_sptr huberWS = getProperty("LoadHuberFrom");
  if (huberWS) {
    g_log.notice() << "Huber angles will be loaded from " << huberWS->getName() << std::endl;
    loadHuber(huberWS);
  }

  // get wavelength
  TimeSeriesProperty<double> *wlprop = dynamic_cast<TimeSeriesProperty<double> *>(expinfo->run().getProperty("Lambda"));
  // assume, that lambda is in nm
  double wavelength = wlprop->minValue() * 10.0; // needed to estimate extents => minValue
  run.addProperty("wavelength", wavelength);
  run.getProperty("wavelength")->setUnits("Angstrom");

  if (load_as == "raw") {
    fillOutputWorkspaceRaw(wavelength);
  } else {
    fillOutputWorkspace(wavelength);
  }

  std::string saveHuberTableWS = getProperty("SaveHuberTo");
  if (!saveHuberTableWS.empty()) {
    Mantid::API::ITableWorkspace_sptr huber_table = saveHuber();
    setProperty("SaveHuberTo", huber_table);
  }
  setProperty("OutputWorkspace", m_OutWS);
}

int LoadDNSSCD::splitIntoColumns(std::list<std::string> &columns, std::string &str) {
  boost::split(columns, str, boost::is_any_of(m_columnSep), boost::token_compress_on);
  return static_cast<int>(columns.size());
}

//----------------------------------------------------------------------------------------------

template <class T>
void LoadDNSSCD::updateProperties(API::Run &run, std::map<std::string, T> &metadata, std::string time) {
  auto it = metadata.begin();
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
        throw std::invalid_argument("Log '" + name + "' already exists but the values are a different type.");
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
/// Fill output workspace with data converted to H, K, L, dE space
void LoadDNSSCD::fillOutputWorkspace(double wavelength) {

  // dimensions
  std::vector<std::string> vec_ID(4);
  vec_ID[0] = "H";
  vec_ID[1] = "K";
  vec_ID[2] = "L";
  vec_ID[3] = "DeltaE";

  std::vector<std::string> dimensionNames(4);
  dimensionNames[0] = "H";
  dimensionNames[1] = "K";
  dimensionNames[2] = "L";
  dimensionNames[3] = "DeltaE";

  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::HKL;

  double a, b, c, alpha, beta, gamma;
  a = getProperty("a");
  b = getProperty("b");
  c = getProperty("c");
  alpha = getProperty("alpha");
  beta = getProperty("beta");
  gamma = getProperty("gamma");
  std::vector<double> u = getProperty("HKL1");
  std::vector<double> v = getProperty("HKL2");

  // load empty DNS instrument to access L1 and L2
  auto loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  loadAlg->initialize();
  loadAlg->setProperty("InstrumentName", "DNS");
  loadAlg->setProperty("OutputWorkspace", "__DNS_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWS = loadAlg->getProperty("OutputWorkspace");
  const auto &instrument = instWS->getInstrument();
  const auto &samplePosition = instrument->getSample()->getPos();
  const auto &sourcePosition = instrument->getSource()->getPos();
  const auto beamVector = samplePosition - sourcePosition;
  const auto l1 = beamVector.norm();
  // calculate tof1
  auto velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass * wavelength * 1e-10); // m/s
  auto tof1 = 1e+06 * l1 / velocity;                                                            // microseconds
  g_log.debug() << "TOF1 = " << tof1 << std::endl;
  // calculate incident energy
  auto Ei = 0.5 * PhysicalConstants::NeutronMass * velocity * velocity / PhysicalConstants::meV;
  g_log.debug() << "Ei = " << Ei << std::endl;

  double dEmin = getProperty("DeltaEmin");
  // estimate extents
  double qmax = 4.0 * M_PI / wavelength;
  std::vector<double> extentMins = {-qmax * a, -qmax * b, -qmax * c, dEmin};
  std::vector<double> extentMaxs = {qmax * a, qmax * b, qmax * c, Ei};

  // Get MDFrame of HKL type with RLU
  auto unitFactory = makeMDUnitFactoryChain();
  auto unit = unitFactory->create(Units::Symbol::RLU.ascii());
  Mantid::Geometry::HKL frame(unit);

  // add dimensions
  for (size_t i = 0; i < m_nDims; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    m_OutWS->addDimension(Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
        id, name, frame, static_cast<coord_t>(extentMins[i]), static_cast<coord_t>(extentMaxs[i]), 5)));
  }

  // Set coordinate system
  m_OutWS->setCoordinateSystem(coordinateSystem);

  // calculate RUB matrix
  Mantid::Geometry::OrientedLattice o;
  o = Mantid::Geometry::OrientedLattice(a, b, c, alpha, beta, gamma);
  o.setUFromVectors(Mantid::Kernel::V3D(u[0], u[1], u[2]), Mantid::Kernel::V3D(v[0], v[1], v[2]));

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
  MDEventWorkspace<MDEvent<4>, 4>::sptr mdws_mdevt_4 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<4>, 4>>(m_OutWS);
  MDEventInserter<MDEventWorkspace<MDEvent<4>, 4>::sptr> inserter(mdws_mdevt_4);

  // create a normalization workspace
  IMDEventWorkspace_sptr normWS = m_OutWS->clone();

  // Creates a new instance of the MDEventInserter to norm workspace
  MDEventWorkspace<MDEvent<4>, 4>::sptr normws_mdevt_4 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<4>, 4>>(normWS);
  MDEventInserter<MDEventWorkspace<MDEvent<4>, 4>::sptr> norm_inserter(normws_mdevt_4);

  // scattering angle limits
  std::vector<double> tth_limits = getProperty("TwoThetaLimits");
  double theta_min = tth_limits[0] * deg2rad / 2.0;
  double theta_max = tth_limits[1] * deg2rad / 2.0;

  // get elastic channel from the user input
  int echannel_user = getProperty("ElasticChannel");

  // Go though each element of m_data to convert to MDEvent
  for (ExpData ds : m_data) {
    uint16_t expInfoIndex = 0;
    signal_t norm_signal(ds.norm);
    signal_t norm_error = std::sqrt(m_normfactor * norm_signal);
    double ki = 2.0 * M_PI / ds.wavelength;
    for (size_t i = 0; i < ds.detID.size(); i++) {
      const auto &detector = instWS->getDetector(i);
      const auto &detectorPosition = detector->getPos();
      const auto detectorVector = detectorPosition - samplePosition;
      const auto l2 = detectorVector.norm();
      auto tof2_elastic = 1e+06 * l2 / velocity;
      // geometric elastic channel
      auto echannel_geom = static_cast<int>(std::ceil(tof2_elastic / ds.chwidth));
      // rotate the signal array to get elastic peak at right position
      int ch_diff = echannel_geom - echannel_user;
      if ((echannel_user > 0) && (ch_diff < 0)) {
        std::rotate(ds.signal[i].begin(), ds.signal[i].begin() - ch_diff, ds.signal[i].end());
      } else if ((echannel_user > 0) && (ch_diff > 0)) {
        std::rotate(ds.signal[i].rbegin(), ds.signal[i].rbegin() + ch_diff, ds.signal[i].rend());
      }
      detid_t detid(ds.detID[i]);
      double theta = 0.5 * (ds.detID[i] * 5.0 - ds.deterota) * deg2rad;
      auto nchannels = static_cast<int64_t>(ds.signal[i].size());
      if ((theta > theta_min) && (theta < theta_max)) {
        PARALLEL_FOR_IF(Kernel::threadSafe(*m_OutWS, *normWS))
        for (int64_t channel = 0; channel < nchannels; channel++) {
          PARALLEL_START_INTERRUPT_REGION
          double signal = ds.signal[i][channel];
          signal_t error = std::sqrt(signal);
          double tof2 = static_cast<double>(channel) * ds.chwidth + 0.5 * ds.chwidth; // bin centers
          double dE = 0.0;
          if (nchannels > 1) {
            double v2 = 1e+06 * l2 / tof2;
            dE = Ei - 0.5 * PhysicalConstants::NeutronMass * v2 * v2 / PhysicalConstants::meV;
          }
          if (dE > dEmin) {
            double kf = std::sqrt(ki * ki - 2.0e-20 * PhysicalConstants::NeutronMass * dE * PhysicalConstants::meV /
                                                (PhysicalConstants::h_bar * PhysicalConstants::h_bar));
            double tlab = std::atan2(ki - kf * cos(2.0 * theta), kf * sin(2.0 * theta));
            double omega = (ds.huber - ds.deterota) * deg2rad - tlab;
            V3D uphi(-cos(omega), 0, -sin(omega));
            double qabs = 0.5 * std::sqrt(ki * ki + kf * kf - 2.0 * ki * kf * cos(2.0 * theta)) / M_PI;
            V3D hphi = uphi * qabs; // qabs = ki * sin(theta), for elastic case;
            V3D hkl = ub_inv * hphi;
            std::vector<Mantid::coord_t> millerindex(4);
            millerindex[0] = static_cast<float>(hkl.X());
            millerindex[1] = static_cast<float>(hkl.Y());
            millerindex[2] = static_cast<float>(hkl.Z());
            millerindex[3] = static_cast<float>(dE);
            PARALLEL_CRITICAL(addValues) {
              inserter.insertMDEvent(static_cast<float>(signal), static_cast<float>(error * error),
                                     static_cast<uint16_t>(expInfoIndex), 0, detid, millerindex.data());

              norm_inserter.insertMDEvent(static_cast<float>(norm_signal), static_cast<float>(norm_error * norm_error),
                                          static_cast<uint16_t>(expInfoIndex), 0, detid, millerindex.data());
            }
          }
          PARALLEL_END_INTERRUPT_REGION
        }
        PARALLEL_CHECK_INTERRUPT_REGION
      }
    }
  }
  setProperty("NormalizationWorkspace", normWS);
}

//----------------------------------------------------------------------------------------------
/// Fill output workspace with raw data theta, omega, tof space
///
void LoadDNSSCD::fillOutputWorkspaceRaw(double wavelength) {

  // dimensions
  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "Theta";
  vec_ID[1] = "Omega";
  vec_ID[2] = "TOF";

  std::vector<std::string> dimensionNames(3);
  dimensionNames[0] = "Scattering Angle";
  dimensionNames[1] = "Omega";
  dimensionNames[2] = "TOF";

  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::None;

  // load empty DNS instrument to access L1 and L2
  auto loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->setLogging(false);
  loadAlg->initialize();
  loadAlg->setProperty("InstrumentName", "DNS");
  loadAlg->setProperty("OutputWorkspace", "__DNS_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWS = loadAlg->getProperty("OutputWorkspace");
  const auto &instrument = instWS->getInstrument();
  const auto &samplePosition = instrument->getSample()->getPos();
  const auto &sourcePosition = instrument->getSource()->getPos();
  const auto beamVector = samplePosition - sourcePosition;
  const auto l1 = beamVector.norm();
  // calculate tof1
  auto velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass * wavelength * 1e-10); // m/s
  auto tof1 = 1e+06 * l1 / velocity;                                                            // microseconds
  g_log.debug() << "TOF1 = " << tof1 << std::endl;
  // calculate incident energy
  auto Ei = 0.5 * PhysicalConstants::NeutronMass * velocity * velocity / PhysicalConstants::meV;
  g_log.debug() << "Ei = " << Ei << std::endl;

  // estimate extents
  // scattering angle limits
  std::vector<double> tth_limits = getProperty("TwoThetaLimits");
  double theta_min = tth_limits[0] / 2.0;
  double theta_max = tth_limits[1] / 2.0;

  std::vector<double> extentMins = {theta_min, 0.0, tof1};
  std::vector<double> extentMaxs = {theta_max, 360.0, m_tof_max};

  // Get MDFrame of HKL type with RLU
  // auto unitFactory = makeMDUnitFactoryChain();
  // auto unit = unitFactory->create(Units::Symbol::RLU.ascii());
  // Mantid::Geometry::HKL frame(unit);
  const Kernel::UnitLabel unitLabel("Degrees");
  Mantid::Geometry::GeneralFrame frame("Scattering Angle", unitLabel);

  // add dimensions
  for (size_t i = 0; i < 3; ++i) {
    std::string id = vec_ID[i];
    std::string name = dimensionNames[i];
    m_OutWS->addDimension(Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
        name, id, frame, static_cast<coord_t>(extentMins[i]), static_cast<coord_t>(extentMaxs[i]), 5)));
  }

  // Set coordinate system
  m_OutWS->setCoordinateSystem(coordinateSystem);

  // Creates a new instance of the MDEventInserter to output workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr mdws_mdevt_3 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(m_OutWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

  // create a normalization workspace
  IMDEventWorkspace_sptr normWS = m_OutWS->clone();

  // Creates a new instance of the MDEventInserter to norm workspace
  MDEventWorkspace<MDEvent<3>, 3>::sptr normws_mdevt_3 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(normWS);
  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> norm_inserter(normws_mdevt_3);

  // get elastic channel from the user input
  int echannel_user = getProperty("ElasticChannel");

  // Go though each element of m_data to convert to MDEvent
  for (ExpData ds : m_data) {
    uint16_t expInfoIndex = 0;
    signal_t norm_signal(ds.norm);
    signal_t norm_error = std::sqrt(m_normfactor * norm_signal);
    for (size_t i = 0; i < ds.detID.size(); i++) {
      const auto &detector = instWS->getDetector(i);
      const auto &detectorPosition = detector->getPos();
      const auto detectorVector = detectorPosition - samplePosition;
      const auto l2 = detectorVector.norm();
      auto tof2_elastic = 1e+06 * l2 / velocity;
      // geometric elastic channel
      auto echannel_geom = static_cast<int>(std::ceil(tof2_elastic / ds.chwidth));
      // rotate the signal array to get elastic peak at right position
      int ch_diff = echannel_geom - echannel_user;
      if ((echannel_user > 0) && (ch_diff < 0)) {
        std::rotate(ds.signal[i].begin(), ds.signal[i].begin() - ch_diff, ds.signal[i].end());
      } else if ((echannel_user > 0) && (ch_diff > 0)) {
        std::rotate(ds.signal[i].rbegin(), ds.signal[i].rbegin() + ch_diff, ds.signal[i].rend());
      }

      detid_t detid(ds.detID[i]);
      double theta = 0.5 * (ds.detID[i] * 5.0 - ds.deterota);
      auto nchannels = static_cast<int64_t>(ds.signal[i].size());
      if ((theta > theta_min) && (theta < theta_max)) {
        PARALLEL_FOR_IF(Kernel::threadSafe(*m_OutWS, *normWS))
        for (int64_t channel = 0; channel < nchannels; channel++) {
          PARALLEL_START_INTERRUPT_REGION
          double signal = ds.signal[i][channel];
          signal_t error = std::sqrt(signal);
          double tof2(tof2_elastic);
          if (nchannels > 1) {
            tof2 = static_cast<double>(channel) * ds.chwidth + 0.5 * ds.chwidth; // bin centers
          }
          double omega = (ds.huber - ds.deterota);

          std::vector<Mantid::coord_t> datapoint(3);
          datapoint[0] = static_cast<float>(theta);
          datapoint[1] = static_cast<float>(omega);
          datapoint[2] = static_cast<float>(tof1 + tof2);
          PARALLEL_CRITICAL(addValues) {
            inserter.insertMDEvent(static_cast<float>(signal), static_cast<float>(error * error),
                                   static_cast<uint16_t>(expInfoIndex), 0, detid, datapoint.data());

            norm_inserter.insertMDEvent(static_cast<float>(norm_signal), static_cast<float>(norm_error * norm_error),
                                        static_cast<uint16_t>(expInfoIndex), 0, detid, datapoint.data());
          }
          PARALLEL_END_INTERRUPT_REGION
        }
        PARALLEL_CHECK_INTERRUPT_REGION
      }
    }
  }
  setProperty("NormalizationWorkspace", normWS);
}

void LoadDNSSCD::read_data(const std::string &fname, std::map<std::string, std::string> &str_metadata,
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
  std::string wtime(Poco::DateTimeFormatter::format(lastModified, "%Y-%m-%dT%H:%M:%S"));
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
      s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
      num_metadata.insert(std::make_pair(s, std::stod(match.str(3))));
    }
    n = line.find("DATA");
    if (n != std::string::npos) {
      break;
    }
  }

  std::map<std::string, double>::const_iterator m = num_metadata.lower_bound("TOF");
  g_log.debug() << "TOF Channels number: " << m->second << std::endl;
  std::map<std::string, double>::const_iterator w = num_metadata.lower_bound("Time");
  g_log.debug() << "Channel width: " << w->second << std::endl;

  ExpData ds;
  ds.deterota = num_metadata["DeteRota"];
  ds.huber = num_metadata["Huber"];
  ds.wavelength = 10.0 * num_metadata["Lambda[nm]"];
  ds.norm = num_metadata[m_normtype];
  ds.chwidth = w->second;
  ds.nchannels = static_cast<size_t>(std::ceil(m->second));

  // read data array
  getline(file, line);

  std::list<std::string> columns;
  while (getline(file, line)) {
    boost::trim(line);
    const int cols = splitIntoColumns(columns, line);
    if (cols > 0) {
      ds.detID.emplace_back(std::stoi(columns.front()));
      columns.pop_front();
      std::vector<double> signal;
      std::transform(columns.begin(), columns.end(), std::back_inserter(signal),
                     [](const std::string &s) { return std::stod(s); });
      ds.signal.emplace_back(signal);
    }
  }
  // DNS PA detector bank has only 24 detectors
  ds.detID.resize(24);
  ds.signal.resize(24);
  m_data.emplace_back(ds);
}

} // namespace Mantid::MDAlgorithms
