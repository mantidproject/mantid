// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * Helper file to gather common routines to the Loaders
 * */

#include "MantidDataHandling/LoadHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PhysicalConstants.h"

#include <nexus/napi.h>

#include <boost/algorithm/string/predicate.hpp> //assert(boost::algorithm::ends_with("mystring", "ing"));

namespace Mantid {

namespace {
/// static logger
Kernel::Logger g_log("LoadHelper");
} // namespace

namespace DataHandling {
using namespace Kernel;
using namespace API;

/**
 * Finds the path for the instrument name in the nexus file
 * Usually of the form: entry0/\<NXinstrument class\>/name
 */
std::string LoadHelper::findInstrumentNexusPath(const Mantid::NeXus::NXEntry &firstEntry) {
  std::string insNamePath;
  std::vector<Mantid::NeXus::NXClassInfo> v = firstEntry.groups();
  for (auto it = v.begin(); it < v.end(); it++) {
    if (it->nxclass == "NXinstrument") {
      insNamePath = it->nxname;
      break;
    }
  }
  return insNamePath;
}

std::string LoadHelper::getStringFromNexusPath(const Mantid::NeXus::NXEntry &firstEntry, const std::string &nexusPath) {
  return firstEntry.getString(nexusPath);
}

double LoadHelper::getDoubleFromNexusPath(const Mantid::NeXus::NXEntry &firstEntry, const std::string &nexusPath) {
  return firstEntry.getFloat(nexusPath);
}

/**
 * Gets the time binning from a Nexus float array
 * Adds an extra bin at the end
 */
std::vector<double> LoadHelper::getTimeBinningFromNexusPath(const Mantid::NeXus::NXEntry &firstEntry,
                                                            const std::string &nexusPath) {

  Mantid::NeXus::NXFloat timeBinningNexus = firstEntry.openNXFloat(nexusPath);
  timeBinningNexus.load();

  size_t numberOfBins = static_cast<size_t>(timeBinningNexus.dim0()) + 1; // boundaries

  float *timeBinning_p = &timeBinningNexus[0];
  std::vector<double> timeBinning(numberOfBins);
  std::copy(timeBinning_p, timeBinning_p + timeBinningNexus.dim0(), std::begin(timeBinning));
  // calculate the extra bin at the end
  timeBinning[numberOfBins - 1] = timeBinning[numberOfBins - 2] + timeBinning[1] - timeBinning[0];

  return timeBinning;
}
/**
 * Calculate Neutron Energy from wavelength: \f$ E = h^2 / 2m\lambda ^2 \f$
 *  @param wavelength :: wavelength in \f$ \mbox{\AA} \f$
 *  @return tof in seconds
 */
double LoadHelper::calculateEnergy(double wavelength) {
  double e = (PhysicalConstants::h * PhysicalConstants::h) /
             (2 * PhysicalConstants::NeutronMass * wavelength * wavelength * 1e-20) / PhysicalConstants::meV;
  return e;
}

/**
 * Calculate TOF from distance
 *  @param distance :: distance in meters
 *  @param wavelength :: wavelength to calculate TOF from
 *  @return tof in seconds
 */
double LoadHelper::calculateTOF(double distance, double wavelength) {
  if (wavelength <= 0) {
    throw std::runtime_error("Wavelenght is <= 0");
  }

  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass * wavelength * 1e-10); // m/s

  return distance / velocity;
}

/*
 * Get instrument property as double
 * @s - input property name
 *
 */
double LoadHelper::getInstrumentProperty(const API::MatrixWorkspace_sptr &workspace, const std::string &s) {
  std::vector<std::string> prop = workspace->getInstrument()->getStringParameter(s);
  if (prop.empty()) {
    return EMPTY_DBL();
  } else {
    return boost::lexical_cast<double>(prop[0]);
  }
}

/**
 * Add properties from a nexus file to
 * the workspace run.
 * API entry for recursive routine below
 *
 *
 * @param nxfileID    :: Nexus file handle to be parsed, just after an
 *NXopengroup
 * @param runDetails  :: where to add properties
 * @param entryName :: entry name to load properties from
 * @param useFullPath :: use full path to entry in nexus tree to generate the log entry name in Mantid
 *
 */
void LoadHelper::addNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails, const std::string &entryName,
                                       bool useFullPath) {
  std::string emptyStr; // needed for first call
  int datatype;
  char nxname[NX_MAXNAMELEN], nxclass[NX_MAXNAMELEN];
  if (!entryName.empty()) {
    strcpy(nxname, entryName.c_str());
  }

  // As a workaround against some "not so good" old ILL nexus files
  // (ILLIN5_Vana_095893.nxs for example)
  // by default we begin the parse on the first entry (entry0),
  // or from a chosen entryName. This allow to avoid the
  // bogus entries that follows.

  NXstatus getnextentry_status = NXgetnextentry(nxfileID, nxname, nxclass, &datatype);
  if (getnextentry_status == NX_OK) {
    if ((NXopengroup(nxfileID, nxname, nxclass)) == NX_OK) {
      recurseAndAddNexusFieldsToWsRun(nxfileID, runDetails, emptyStr, emptyStr, 1 /* level */, useFullPath);
      NXclosegroup(nxfileID);
    }
  }
}

/**
 * Recursively add properties from a nexus file to
 * the workspace run.
 *
 * @param nxfileID    :: Nexus file handle to be parsed, just after an
 *NXopengroup
 * @param runDetails  :: where to add properties
 * @param parent_name :: nexus caller name
 * @param parent_class :: nexus caller class
 * @param level       :: current level in nexus tree
 * @param useFullPath :: use full path to entry in nexus tree to generate the log entry name in Mantid
 *
 */
void LoadHelper::recurseAndAddNexusFieldsToWsRun(NXhandle nxfileID, API::Run &runDetails, std::string &parent_name,
                                                 std::string &parent_class, int level, bool useFullPath) {
  // Classes
  NXstatus getnextentry_status; ///< return status
  int datatype;                 ///< NX data type if a dataset, e.g. NX_CHAR, NX_FLOAT32, see
  /// napi.h
  char nxname[NX_MAXNAMELEN], nxclass[NX_MAXNAMELEN];
  nxname[0] = '0';
  nxclass[0] = '0';

  bool has_entry = true; // follows getnextentry_status
  while (has_entry) {
    getnextentry_status = NXgetnextentry(nxfileID, nxname, nxclass, &datatype);

    if (getnextentry_status == NX_OK) {
      NXstatus opengroup_status;
      NXstatus opendata_status;
      NXstatus getinfo_status;

      std::string property_name = (parent_name.empty() ? nxname : parent_name + "." + nxname);
      if ((opengroup_status = NXopengroup(nxfileID, nxname, nxclass)) == NX_OK) {
        if (std::string(nxclass) != "ILL_data_scan_vars" && std::string(nxclass) != "NXill_data_scan_vars") {
          // Go down to one level, if the group is known to nexus
          std::string p_nxname = useFullPath ? property_name : nxname; // current names can be useful for next level
          std::string p_nxclass(nxclass);

          recurseAndAddNexusFieldsToWsRun(nxfileID, runDetails, p_nxname, p_nxclass, level + 1, useFullPath);
        }

        NXclosegroup(nxfileID);
      } // if(NXopengroup
      else if ((opendata_status = NXopendata(nxfileID, nxname)) == NX_OK) {
        if (parent_class != "NXData" && parent_class != "NXMonitor" &&
            std::string(nxname) != "data") { // create a property
          int rank = 0;
          int dims[4] = {0, 0, 0, 0};
          int type;

          // Get the value
          if ((getinfo_status = NXgetinfo(nxfileID, &rank, dims, &type)) == NX_OK) {
            // Note, we choose to only build properties on small float arrays
            // filter logic is below
            bool build_small_float_array = false; // default
            bool read_property = true;

            if ((type == NX_FLOAT32) || (type == NX_FLOAT64)) {
              if ((rank == 1) && (dims[0] <= 9)) {
                build_small_float_array = true;
              } else {
                read_property = false;
              }
            } else if (type != NX_CHAR) {
              if ((rank > 1) || (dims[0] > 1) || (dims[1] > 1) || (dims[2] > 1) || (dims[3] > 1)) {
                read_property = false;
              }
            } else {
              if ((rank > 1) || (dims[1] > 1) || (dims[2] > 1) || (dims[3] > 1)) {
                read_property = false;
              }
            }

            if (read_property) {

              void *dataBuffer;
              NXmalloc(&dataBuffer, rank, dims, type);

              if (NXgetdata(nxfileID, dataBuffer) == NX_OK) {

                if (type == NX_CHAR) {
                  std::string property_value(reinterpret_cast<const char *>(dataBuffer));
                  if (boost::algorithm::ends_with(property_name, "_time")) {
                    // That's a time value! Convert to Mantid standard
                    property_value = dateTimeInIsoFormat(property_value);
                    if (runDetails.hasProperty(property_name))
                      runDetails.getProperty(property_name)->setValue(property_value);
                    else
                      runDetails.addProperty(property_name, property_value);
                  } else {
                    if (!runDetails.hasProperty(property_name)) {
                      runDetails.addProperty(property_name, property_value);
                    } else {
                      g_log.warning() << "Property " << property_name
                                      << " was set twice. Please check the Nexus file and your inputs." << std::endl;
                    }
                  }

                } else if ((type == NX_FLOAT32) || (type == NX_FLOAT64) || (type == NX_INT16) || (type == NX_INT32) ||
                           (type == NX_UINT16)) {

                  // Look for "units"
                  NXstatus units_status;
                  char units_sbuf[NX_MAXNAMELEN];
                  int units_len = NX_MAXNAMELEN;
                  int units_type = NX_CHAR;

                  char unitsAttrName[] = "units";
                  units_status = NXgetattr(nxfileID, unitsAttrName, units_sbuf, &units_len, &units_type);
                  if ((type == NX_FLOAT32) || (type == NX_FLOAT64)) {
                    // Mantid numerical properties are double only.
                    double property_double_value = 0.0;

                    // Simple case, one value
                    if (dims[0] == 1) {
                      if (type == NX_FLOAT32) {
                        property_double_value = *(reinterpret_cast<float *>(dataBuffer));
                      } else if (type == NX_FLOAT64) {
                        property_double_value = *(reinterpret_cast<double *>(dataBuffer));
                      }
                      if (!runDetails.hasProperty(property_name)) {

                        if (units_status != NX_ERROR)
                          runDetails.addProperty(property_name, property_double_value, std::string(units_sbuf));
                        else
                          runDetails.addProperty(property_name, property_double_value);
                      } else {
                        g_log.warning() << "Property " << property_name
                                        << " was set twice. Please check the Nexus file and your inputs." << std::endl;
                      }
                    } else if (build_small_float_array) {
                      // An array, converted to "name_index", with index < 10
                      // (see
                      // test above)
                      for (int dim_index = 0; dim_index < dims[0]; dim_index++) {
                        if (type == NX_FLOAT32) {
                          property_double_value = (reinterpret_cast<float *>(dataBuffer))[dim_index];
                        } else if (type == NX_FLOAT64) {
                          property_double_value = (reinterpret_cast<double *>(dataBuffer))[dim_index];
                        }
                        std::string indexed_property_name =
                            property_name + std::string("_") + std::to_string(dim_index);

                        if (!runDetails.hasProperty(indexed_property_name)) {
                          if (units_status != NX_ERROR)
                            runDetails.addProperty(indexed_property_name, property_double_value,
                                                   std::string(units_sbuf));
                          else
                            runDetails.addProperty(indexed_property_name, property_double_value);
                        } else {
                          g_log.warning()
                              << "Property " << property_name
                              << " was set twice. Please check the Nexus file and your inputs." << std::endl;
                        }
                      }
                    }

                  } else {
                    // int case
                    int property_int_value = 0;
                    if (type == NX_INT16) {
                      property_int_value = *(reinterpret_cast<short int *>(dataBuffer));
                    } else if (type == NX_INT32) {
                      property_int_value = *(reinterpret_cast<int *>(dataBuffer));
                    } else if (type == NX_UINT16) {
                      property_int_value = *(reinterpret_cast<short unsigned int *>(dataBuffer));
                    }

                    if (!runDetails.hasProperty(property_name)) {
                      if (units_status != NX_ERROR)
                        runDetails.addProperty(property_name, property_int_value, std::string(units_sbuf));
                      else
                        runDetails.addProperty(property_name, property_int_value);
                    } else {
                      g_log.warning() << "Property " << property_name
                                      << " was set twice. Please check the Nexus file and your inputs." << std::endl;
                    }

                  } // if (type==...
                }
              }
              NXfree(&dataBuffer);
              dataBuffer = nullptr;
            }

          } // if NXgetinfo OK
        }   // if (parent_class == "NXData" || parent_class == "NXMonitor") else

        NXclosedata(nxfileID);
      }
    } else if (getnextentry_status == NX_EOD) {
      has_entry = false; // end of loop
    } else {
      has_entry = false; // end of loop
    }

  } // while

} // recurseAndAddNexusFieldsToWsRun

/**
 * Parses the date as formatted at the ILL:
 * 29-Jun-12 11:27:26
 * and converts it to the ISO format used in Mantid:
 * ISO8601 format string: "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"
 *
 *  @param dateToParse :: date as string
 *  @return date as required in Mantid
 */
std::string LoadHelper::dateTimeInIsoFormat(const std::string &dateToParse) {
  namespace bt = boost::posix_time;
  try {
    // Try to convert to a boost date as an iso string. If it works, we are already in ISO and no conversion is
    // requiered.
    bt::from_iso_extended_string(dateToParse);
    return dateToParse;

  } catch (...) {
  }
  // parsing format
  const std::locale format = std::locale(std::locale::classic(), new bt::time_input_facet("%d-%b-%y %H:%M:%S"));

  bt::ptime pt;
  std::istringstream is(dateToParse);
  is.imbue(format);
  is >> pt;

  if (pt != bt::ptime()) {
    // Converts to ISO
    std::string s = bt::to_iso_extended_string(pt);
    return s;
  } else {
    return "";
  }
}

/**
 * @brief LoadHelper::moveComponent
 * @param ws A MatrixWorkspace
 * @param componentName The name of the component of the instrument
 * @param newPos New position of the component
 */
void LoadHelper::moveComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName,
                               const V3D &newPos) {
  Geometry::IComponent_const_sptr component = ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName + " not found");
  }
  auto &componentInfo = ws->mutableComponentInfo();
  const auto componentIndex = componentInfo.indexOf(component->getComponentID());
  componentInfo.setPosition(componentIndex, newPos);
}

/**
 * @brief LoadHelper::rotateComponent
 * @param ws A MantrixWorkspace
 * @param componentName The Name of the component of the instrument
 * @param rot Rotations defined by setting a quaternion from an angle in degrees
 * and an axis
 */
void LoadHelper::rotateComponent(const API::MatrixWorkspace_sptr &ws, const std::string &componentName,
                                 const Kernel::Quat &rot) {
  Geometry::IComponent_const_sptr component = ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName + " not found");
  }
  auto &componentInfo = ws->mutableComponentInfo();
  const auto componentIndex = componentInfo.indexOf(component->getComponentID());
  componentInfo.setRotation(componentIndex, rot);
}

/**
 * @brief LoadHelper::getComponentPosition
 * @param ws A MatrixWorkspace
 * @param componentName The Name of the component of the instrument
 * @return The position of the component
 */
V3D LoadHelper::getComponentPosition(const API::MatrixWorkspace_sptr &ws, const std::string &componentName) {
  Geometry::IComponent_const_sptr component = ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName + " not found");
  }
  V3D pos = component->getPos();
  return pos;
}

/**
 * Loads empty instrument of chosen name into a provided workspace
 * @param ws A MatrixWorkspace
 * @param instrumentName Name of the instrument to be loaded
 * @param instrumentPath Path to the instrument definition file, optional
 */
void LoadHelper::loadEmptyInstrument(const API::MatrixWorkspace_sptr &ws, const std::string &instrumentName,
                                     const std::string &instrumentPath) {
  auto loadInst = AlgorithmManager::Instance().create("LoadInstrument");
  loadInst->initialize();
  loadInst->setChild(true);
  loadInst->setPropertyValue("InstrumentName", instrumentName);
  if (!instrumentPath.empty())
    loadInst->setPropertyValue("Filename", instrumentPath);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", ws);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

/**
 * Fills workspace with histogram data from provided data structure
 * @param ws A MatrixWorkspace to be filled with data
 * @param data Data object to extract counts from
 * @param xAxis X axis values to be assigned to each spectrum
 * @param initialSpectrum Initial spectrum number, optional and defaults to 0
 * @param pointData Switch to decide whether the data is going to be a histogram or point data, defaults to false
 * (histogram)
 * @param detectorIDs Vector of detector IDs to override the default spectrum number, defaults to empty (IDs equal to
 * index)
 * @param acceptedDetectorIDs Set of accepted detector IDs, defaults to empty (all accepted)
 * @param axisOrder Tuple containing information at which position in data one can find tubes, pixels, and channels
 * (scans), defaults to 0,1,2 meaning default order of tube-pixel-channel
 */
void LoadHelper::fillStaticWorkspace(const API::MatrixWorkspace_sptr &ws, const Mantid::NeXus::NXInt &data,
                                     const std::vector<double> &xAxis, int initialSpectrum, bool pointData,
                                     const std::vector<int> &detectorIDs, const std::set<int> &acceptedDetectorIDs,
                                     const std::tuple<short, short, short> &axisOrder) {

  const bool customDetectorIDs = detectorIDs.size() != 0;
  const bool excludeDetectorIDs = acceptedDetectorIDs.size() != 0;

  std::array dims = {data.dim0(), data.dim1(), data.dim2()};
  const auto nTubes = dims[std::get<0>(axisOrder)];
  const auto nPixels = dims[std::get<1>(axisOrder)];
  const auto nChannels = dims[std::get<2>(axisOrder)];

  int loadOrder[3] = {0, 1, 2};
  loadingOrder(axisOrder, loadOrder);

  HistogramData::Points histoPoints;
  HistogramData::BinEdges binEdges;
  if (pointData)
    histoPoints = HistogramData::Points(xAxis);
  else
    binEdges = HistogramData::BinEdges(xAxis);
  int nSkipped = 0;

#pragma omp parallel for if (!excludeDetectorIDs && Kernel::threadSafe(*ws))
  for (int tube_no = 0; tube_no < nTubes; ++tube_no) {
    for (int pixel_no = 0; pixel_no < nPixels; ++pixel_no) {
      auto currentSpectrum = initialSpectrum + tube_no * nPixels + pixel_no;
      if (excludeDetectorIDs != 0 && std::find(acceptedDetectorIDs.cbegin(), acceptedDetectorIDs.cend(),
                                               currentSpectrum) == acceptedDetectorIDs.end()) {
        nSkipped++;
        continue;
      }
      currentSpectrum -= nSkipped;

      std::vector<int> spectrum(nChannels);
      for (auto channel_no = 0; channel_no < nChannels; ++channel_no) {
        const int dataIndices[3] = {tube_no, pixel_no, channel_no};
        spectrum[channel_no] = data(dataIndices[loadOrder[0]], dataIndices[loadOrder[1]], dataIndices[loadOrder[2]]);
      }
      const HistogramData::Counts counts(spectrum.begin(), spectrum.end());
      const HistogramData::CountVariances countVariances(spectrum.begin(), spectrum.end());
      if (pointData) {
        ws->setCounts(currentSpectrum, counts);
        ws->setCountVariances(currentSpectrum, countVariances);
        ws->setPoints(currentSpectrum, histoPoints);
      } else {
        ws->setHistogram(currentSpectrum, binEdges, counts);
      }
      const auto detectorID = customDetectorIDs ? detectorIDs[currentSpectrum] : currentSpectrum;
      ws->getSpectrum(currentSpectrum).setSpectrumNo(detectorID);
    }
  }
}

/**
 * Handles non-standard loading order of the provided data, based on the provided data dimension order.
 * @param dataOrder tuple containing where tubes, pixels, and channels (scans) can be found in data
 * @param dataIndices pointer to the array containing data indices to be set
 */
void LoadHelper::loadingOrder(const std::tuple<short, short, short> &dataOrder, int *dataIndices) {
  if (std::get<0>(dataOrder) != 0)
    dataIndices[0] = std::get<1>(dataOrder) == 0 ? 1 : 2;
  if (std::get<1>(dataOrder) != 1)
    dataIndices[1] = std::get<0>(dataOrder) == 1 ? 0 : 2;
  if (std::get<2>(dataOrder) != 2)
    dataIndices[2] = std::get<1>(dataOrder) == 2 ? 1 : 0;
}

/**
 * Fills workspace with histogram data from provided data structure
 * @param ws A MatrixWorkspace to be filled with data
 * @param data Data object to extract counts from
 * @param xAxis X axis values to be assigned to each spectrum
 * @param initialSpectrum Initial spectrum number, optional and defaults to 0
 * @param acceptedDetectorIDs Set of accepted detector IDs, defaults to empty (all accepted)
 * @param customDetectorIDs Vector of custom detector IDs to replace the default detectorID calculation, defaults to
 * empty (default calculation)
 * @param axisOrder Tuple containing description of axis order of 3D Nexus data, defaults to 0,1,2 meaning
 * tube-pixel-channel order
 */
void LoadHelper::fillMovingWorkspace(const API::MatrixWorkspace_sptr &ws, const Mantid::NeXus::NXInt &data,
                                     const std::vector<double> &xAxis, int initialSpectrum,
                                     const std::set<int> &acceptedDetectorIDs,
                                     const std::vector<int> &customDetectorIDs,
                                     const std::tuple<short, short, short> &axisOrder) {

  const auto useCustomSpectraMap = customDetectorIDs.size() != 0;
  const auto useAcceptedDetectorIDs = acceptedDetectorIDs.size() != 0;

  std::array dims = {data.dim0(), data.dim1(), data.dim2()};
  const auto nTubes = dims[std::get<0>(axisOrder)];
  const auto nPixels = dims[std::get<1>(axisOrder)];
  const auto nScans = dims[std::get<2>(axisOrder)];

  int nSkipped = 0;
#pragma omp parallel for if (Kernel::threadSafe(*ws))
  for (int tube_no = 0; tube_no < nTubes; ++tube_no) {
    for (int pixel_no = 0; pixel_no < nPixels; ++pixel_no) {
      auto currentDetector = initialSpectrum + tube_no * nPixels + pixel_no;
      if (useAcceptedDetectorIDs && std::find(acceptedDetectorIDs.cbegin(), acceptedDetectorIDs.cend(),
                                              currentDetector) == acceptedDetectorIDs.end()) {
        nSkipped++;
        continue;
      }
      currentDetector -= nSkipped;
      int currentSpectrum;
      if (useCustomSpectraMap)
        currentSpectrum = customDetectorIDs[currentDetector - initialSpectrum];
      else
        currentSpectrum = currentDetector;
      currentSpectrum *= nScans;
      for (auto channel_no = 0; channel_no < nScans; ++channel_no) {
        auto spectrumValue = data(channel_no, tube_no, pixel_no);
        ws->mutableY(currentSpectrum) = spectrumValue;
        ws->mutableE(currentSpectrum) = sqrt(spectrumValue);
        ws->mutableX(currentSpectrum) = xAxis;
        currentSpectrum++;
      }
    }
  }
}

/**
 * Replaces errors of bins with zero counts with provided value
 * @param ws MatrixWorkspace to have its zero errors replaced
 * @param zeroCountsError Value to replace default error of square root of counts
 */
void LoadHelper::replaceZeroErrors(const API::MatrixWorkspace_sptr &ws, double zeroCountsError) {
  for (size_t spectrum_no = 0; spectrum_no < ws->getNumberHistograms(); ++spectrum_no) {
    auto &errorAxis = ws->mutableE(spectrum_no);
    std::transform(errorAxis.begin(), errorAxis.end(), errorAxis.begin(),
                   [zeroCountsError](const auto &error) { return error == 0 ? zeroCountsError : error; });
  }
}

/**
 * Fetches NXInt data from the requested group name in the entry provided.
 * @param entry NXEntry where desired data can be found
 * @param groupName Full name of the data group
 * @return NXInt data object
 */
NeXus::NXInt LoadHelper::getIntDataset(const NeXus::NXEntry &entry, const std::string &groupName) {
  auto dataGroup = entry.openNXData(groupName);
  return dataGroup.openIntData();
}

/**
 * Fetches NXDouble data from the requested group name in the entry provided.
 * @param entry NXEntry where desired data can be found
 * @param groupName Full name of the data group
 * @return NXDouble data object
 */
NeXus::NXDouble LoadHelper::getDoubleDataset(const NeXus::NXEntry &entry, const std::string &groupName) {
  auto dataGroup = entry.openNXData(groupName);
  return dataGroup.openDoubleData();
}

} // namespace DataHandling
} // namespace Mantid
