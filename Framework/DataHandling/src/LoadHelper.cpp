// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 * Helper file to gather common routines to the Loaders
 * */

#include "MantidDataHandling/LoadHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/PhysicalConstants.h"

#include <nexus/napi.h>

#include <boost/algorithm/string/predicate.hpp> //assert(boost::algorithm::ends_with("mystring", "ing"));

namespace Mantid {
namespace DataHandling {

namespace {
/// static logger
Kernel::Logger g_log("LoadHelper");
} // namespace

using namespace Kernel;
using namespace API;

/**
 * Finds the path for the instrument name in the nexus file
 * Usually of the form: entry0/\<NXinstrument class\>/name
 */
std::string
LoadHelper::findInstrumentNexusPath(const Mantid::NeXus::NXEntry &firstEntry) {
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

std::string
LoadHelper::getStringFromNexusPath(const Mantid::NeXus::NXEntry &firstEntry,
                                   const std::string &nexusPath) {
  return firstEntry.getString(nexusPath);
}

double
LoadHelper::getDoubleFromNexusPath(const Mantid::NeXus::NXEntry &firstEntry,
                                   const std::string &nexusPath) {
  return firstEntry.getFloat(nexusPath);
}

/**
 * Gets the time binning from a Nexus float array
 * Adds an extra bin at the end
 */
std::vector<double> LoadHelper::getTimeBinningFromNexusPath(
    const Mantid::NeXus::NXEntry &firstEntry, const std::string &nexusPath) {

  Mantid::NeXus::NXFloat timeBinningNexus = firstEntry.openNXFloat(nexusPath);
  timeBinningNexus.load();

  size_t numberOfBins =
      static_cast<size_t>(timeBinningNexus.dim0()) + 1; // boundaries

  float *timeBinning_p = &timeBinningNexus[0];
  std::vector<double> timeBinning(numberOfBins);
  std::copy(timeBinning_p, timeBinning_p + timeBinningNexus.dim0(),
            std::begin(timeBinning));
  // calculate the extra bin at the end
  timeBinning[numberOfBins - 1] =
      timeBinning[numberOfBins - 2] + timeBinning[1] - timeBinning[0];

  return timeBinning;
}
/**
 * Calculate Neutron Energy from wavelength: \f$ E = h^2 / 2m\lambda ^2 \f$
 *  @param wavelength :: wavelength in \f$ \mbox{\AA} \f$
 *  @return tof in seconds
 */
double LoadHelper::calculateEnergy(double wavelength) {
  double e =
      (PhysicalConstants::h * PhysicalConstants::h) /
      (2 * PhysicalConstants::NeutronMass * wavelength * wavelength * 1e-20) /
      PhysicalConstants::meV;
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

  double velocity = PhysicalConstants::h / (PhysicalConstants::NeutronMass *
                                            wavelength * 1e-10); // m/s

  return distance / velocity;
}

/*
 * Get instrument property as double
 * @s - input property name
 *
 */
double
LoadHelper::getInstrumentProperty(const API::MatrixWorkspace_sptr &workspace,
                                  std::string s) {
  std::vector<double> prop = workspace->getInstrument()->getNumberParameter(s);
  if (prop.empty()) {
    g_log.debug("Property <" + s + "> doesn't exist!");
    return EMPTY_DBL();
  } else {
    g_log.debug() << "Property <" + s + "> = " << prop[0] << '\n';
    return prop[0];
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
 *
 */
void LoadHelper::addNexusFieldsToWsRun(NXhandle nxfileID,
                                       API::Run &runDetails) {
  std::string emptyStr; // needed for first call
  int datatype;
  char nxname[NX_MAXNAMELEN], nxclass[NX_MAXNAMELEN];

  // As a workaround against some "not so good" old ILL nexus files
  // (ILLIN5_Vana_095893.nxs for example)
  // we begin the parse on the first entry (entry0). This allow to avoid the
  // bogus entries that follows.

  NXstatus getnextentry_status =
      NXgetnextentry(nxfileID, nxname, nxclass, &datatype);
  if (getnextentry_status == NX_OK) {
    if ((NXopengroup(nxfileID, nxname, nxclass)) == NX_OK) {
      if (std::string(nxname) == "entry0") {
        recurseAndAddNexusFieldsToWsRun(nxfileID, runDetails, emptyStr,
                                        emptyStr, 1 /* level */);
      } else {
        g_log.debug() << "Unexpected group name in nexus file : " << nxname
                      << '\n';
      }
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
 *
 */
void LoadHelper::recurseAndAddNexusFieldsToWsRun(NXhandle nxfileID,
                                                 API::Run &runDetails,
                                                 std::string &parent_name,
                                                 std::string &parent_class,
                                                 int level) {

  std::string indent_str(level * 2, ' '); // Two space by indent level

  // Link ?

  // Attributes ?

  // Classes
  NXstatus getnextentry_status; ///< return status
  int datatype; ///< NX data type if a dataset, e.g. NX_CHAR, NX_FLOAT32, see
  /// napi.h
  char nxname[NX_MAXNAMELEN], nxclass[NX_MAXNAMELEN];
  nxname[0] = '0';
  nxclass[0] = '0';

  bool has_entry = true; // follows getnextentry_status
  while (has_entry) {
    getnextentry_status = NXgetnextentry(nxfileID, nxname, nxclass, &datatype);

    if (getnextentry_status == NX_OK) {
      g_log.debug() << indent_str << parent_name << "." << nxname << " ; "
                    << nxclass << '\n';

      NXstatus opengroup_status;
      NXstatus opendata_status;
      NXstatus getinfo_status;

      if ((opengroup_status = NXopengroup(nxfileID, nxname, nxclass)) ==
          NX_OK) {

        if (std::string(nxclass) != "ILL_data_scan_vars") {

          // Go down to one level, if the group is known to nexus
          std::string p_nxname(
              nxname); // current names can be useful for next level
          std::string p_nxclass(nxclass);

          recurseAndAddNexusFieldsToWsRun(nxfileID, runDetails, p_nxname,
                                          p_nxclass, level + 1);
        }

        NXclosegroup(nxfileID);
      } // if(NXopengroup
      else if ((opendata_status = NXopendata(nxfileID, nxname)) == NX_OK) {
        // dump_attributes(nxfileID, indent_str);
        g_log.debug() << indent_str << nxname << " opened.\n";

        if (parent_class == "NXData" || parent_class == "NXMonitor" ||
            std::string(nxname) == "data") {
          g_log.debug() << indent_str << "skipping " << parent_class << " ("
                        << nxname << ")\n";
          /* nothing */
        } else { // create a property
          int rank = 0;
          int dims[4] = {0, 0, 0, 0};
          int type;

          std::string property_name =
              (parent_name.empty() ? nxname : parent_name + "." + nxname);

          g_log.debug() << indent_str << "considering property "
                        << property_name << '\n';

          // Get the value
          if ((getinfo_status = NXgetinfo(nxfileID, &rank, dims, &type)) ==
              NX_OK) {

            g_log.debug() << indent_str << "Rank of " << property_name << " is "
                          << rank << "\n"
                          << indent_str << "Dimensions are " << dims[0] << ", "
                          << dims[1] << ", " << dims[2] << ", " << dims[3]
                          << "\n";

            // Note, we choose to only build properties on small float arrays
            // filter logic is below
            bool build_small_float_array = false; // default
            bool read_property = true;

            if ((type == NX_FLOAT32) || (type == NX_FLOAT64)) {
              if ((rank == 1) && (dims[0] <= 9)) {
                build_small_float_array = true;
              } else {
                g_log.debug() << indent_str
                              << "ignored multi dimensional number "
                                 "data with more than 10 elements "
                              << property_name << '\n';
                read_property = false;
              }
            } else if (type != NX_CHAR) {
              if ((rank > 1) || (dims[0] > 1) || (dims[1] > 1) ||
                  (dims[2] > 1) || (dims[3] > 1)) {
                g_log.debug()
                    << indent_str << "ignored non-scalar numeric data on "
                    << property_name << '\n';
                read_property = false;
              }
            } else {
              if ((rank > 1) || (dims[1] > 1) || (dims[2] > 1) ||
                  (dims[3] > 1)) {
                g_log.debug() << indent_str << "ignored string array data on "
                              << property_name << '\n';
                read_property = false;
              }
            }

            if (read_property) {

              void *dataBuffer;
              NXmalloc(&dataBuffer, rank, dims, type);

              if (NXgetdata(nxfileID, dataBuffer) == NX_OK) {

                if (type == NX_CHAR) {
                  std::string property_value(
                      reinterpret_cast<const char *>(dataBuffer));
                  if (boost::algorithm::ends_with(property_name, "_time")) {
                    // That's a time value! Convert to Mantid standard
                    property_value = dateTimeInIsoFormat(property_value);
                  }
                  runDetails.addProperty(property_name, property_value);

                } else if ((type == NX_FLOAT32) || (type == NX_FLOAT64) ||
                           (type == NX_INT16) || (type == NX_INT32) ||
                           (type == NX_UINT16)) {

                  // Look for "units"
                  NXstatus units_status;
                  char units_sbuf[NX_MAXNAMELEN];
                  int units_len = NX_MAXNAMELEN;
                  int units_type = NX_CHAR;

                  char unitsAttrName[] = "units";
                  units_status = NXgetattr(nxfileID, unitsAttrName, units_sbuf,
                                           &units_len, &units_type);
                  if (units_status != NX_ERROR) {
                    g_log.debug() << indent_str << "[ " << property_name
                                  << " has unit " << units_sbuf << " ]\n";
                  }

                  if ((type == NX_FLOAT32) || (type == NX_FLOAT64)) {
                    // Mantid numerical properties are double only.
                    double property_double_value = 0.0;

                    // Simple case, one value
                    if (dims[0] == 1) {
                      if (type == NX_FLOAT32) {
                        property_double_value =
                            *(reinterpret_cast<float *>(dataBuffer));
                      } else if (type == NX_FLOAT64) {
                        property_double_value =
                            *(reinterpret_cast<double *>(dataBuffer));
                      }
                      if (units_status != NX_ERROR)
                        runDetails.addProperty(property_name,
                                               property_double_value,
                                               std::string(units_sbuf));
                      else
                        runDetails.addProperty(property_name,
                                               property_double_value);
                    } else if (build_small_float_array) {
                      // An array, converted to "name_index", with index < 10
                      // (see
                      // test above)
                      for (int dim_index = 0; dim_index < dims[0];
                           dim_index++) {
                        if (type == NX_FLOAT32) {
                          property_double_value = (reinterpret_cast<float *>(
                              dataBuffer))[dim_index];
                        } else if (type == NX_FLOAT64) {
                          property_double_value = (reinterpret_cast<double *>(
                              dataBuffer))[dim_index];
                        }
                        std::string indexed_property_name =
                            property_name + std::string("_") +
                            std::to_string(dim_index);
                        if (units_status != NX_ERROR)
                          runDetails.addProperty(indexed_property_name,
                                                 property_double_value,
                                                 std::string(units_sbuf));
                        else
                          runDetails.addProperty(indexed_property_name,
                                                 property_double_value);
                      }
                    }

                  } else {
                    // int case
                    int property_int_value = 0;
                    if (type == NX_INT16) {
                      property_int_value =
                          *(reinterpret_cast<short int *>(dataBuffer));
                    } else if (type == NX_INT32) {
                      property_int_value =
                          *(reinterpret_cast<int *>(dataBuffer));
                    } else if (type == NX_UINT16) {
                      property_int_value =
                          *(reinterpret_cast<short unsigned int *>(dataBuffer));
                    }

                    if (units_status != NX_ERROR)
                      runDetails.addProperty(property_name, property_int_value,
                                             std::string(units_sbuf));
                    else
                      runDetails.addProperty(property_name, property_int_value);

                  } // if (type==...

                } else {
                  g_log.debug() << indent_str << "unexpected data on "
                                << property_name << '\n';
                } // test on nxdata type

              } else {
                g_log.debug() << indent_str << "could not read the value of "
                              << property_name << '\n';
              }

              NXfree(&dataBuffer);
              dataBuffer = nullptr;
            }

          } // if NXgetinfo OK
          else {
            g_log.debug() << indent_str << "unexpected status ("
                          << getinfo_status << ") on " << nxname << '\n';
          }

        } // if (parent_class == "NXData" || parent_class == "NXMonitor") else

        NXclosedata(nxfileID);
      } else {
        g_log.debug() << indent_str << "unexpected status (" << opendata_status
                      << ") on " << nxname << '\n';
      }

    } else if (getnextentry_status == NX_EOD) {
      g_log.debug() << indent_str << "End of Dir\n";
      has_entry = false; // end of loop
    } else {
      g_log.debug() << indent_str << "unexpected status ("
                    << getnextentry_status << ")\n";
      has_entry = false; // end of loop
    }

  } // while

} // recurseAndAddNexusFieldsToWsRun

/**
 * Show attributes attached to the current Nexus entry
 *
 * @param nxfileID The Nexus entry
 * @param indentStr Indent spaces do display nexus entries as a tree
 *
 */
void LoadHelper::dumpNexusAttributes(NXhandle nxfileID,
                                     std::string &indentStr) {
  // Attributes
  NXname pName;
  int iLength, iType;
#ifndef NEXUS43
  int rank;
  int dims[4];
#endif
  int nbuff = 127;
  boost::shared_array<char> buff(new char[nbuff + 1]);

#ifdef NEXUS43
  while (NXgetnextattr(nxfileID, pName, &iLength, &iType) != NX_EOD) {
#else
  while (NXgetnextattra(nxfileID, pName, &rank, dims, &iType) != NX_EOD) {
    g_log.debug() << indentStr << '@' << pName << " = ";
    if (rank > 1) { // mantid only supports single value attributes
      throw std::runtime_error(
          "Encountered attribute with multi-dimensional array value");
    }
    iLength = dims[0]; // to clarify things
    if (iType != NX_CHAR && iLength != 1) {
      throw std::runtime_error("Encountered attribute with array value");
    }
#endif

    switch (iType) {
    case NX_CHAR: {
      if (iLength > nbuff + 1) {
        nbuff = iLength;
        buff.reset(new char[nbuff + 1]);
      }
      int nz = iLength + 1;
      NXgetattr(nxfileID, pName, buff.get(), &nz, &iType);
      g_log.debug() << indentStr << buff.get() << '\n';
      break;
    }
    case NX_INT16: {
      short int value;
      NXgetattr(nxfileID, pName, &value, &iLength, &iType);
      g_log.debug() << indentStr << value << '\n';
      break;
    }
    case NX_INT32: {
      int value;
      NXgetattr(nxfileID, pName, &value, &iLength, &iType);
      g_log.debug() << indentStr << value << '\n';
      break;
    }
    case NX_UINT16: {
      short unsigned int value;
      NXgetattr(nxfileID, pName, &value, &iLength, &iType);
      g_log.debug() << indentStr << value << '\n';
      break;
    }
    } // switch
  }   // while
}
/**
 * Parses the date as formatted at the ILL:
 * 29-Jun-12 11:27:26
 * and converts it to the ISO format used in Mantid:
 * ISO8601 format string: "yyyy-mm-ddThh:mm:ss[Z+-]tz:tz"
 *
 *  @param dateToParse :: date as string
 *  @return date as required in Mantid
 */
std::string LoadHelper::dateTimeInIsoFormat(std::string dateToParse) {
  namespace bt = boost::posix_time;
  // parsing format
  const std::locale format = std::locale(
      std::locale::classic(), new bt::time_input_facet("%d-%b-%y %H:%M:%S"));

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
void LoadHelper::moveComponent(API::MatrixWorkspace_sptr ws,
                               const std::string &componentName,
                               const V3D &newPos) {
  Geometry::IComponent_const_sptr component =
      ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName +
                                " not found");
  }
  auto &componentInfo = ws->mutableComponentInfo();
  const auto componentIndex =
      componentInfo.indexOf(component->getComponentID());
  componentInfo.setPosition(componentIndex, newPos);
}

/**
 * @brief LoadHelper::rotateComponent
 * @param ws A MantrixWorkspace
 * @param componentName The Name of the component of the instrument
 * @param rot Rotations defined by setting a quaternion from an angle in degrees
 * and an axis
 */
void LoadHelper::rotateComponent(API::MatrixWorkspace_sptr ws,
                                 const std::string &componentName,
                                 const Kernel::Quat &rot) {
  Geometry::IComponent_const_sptr component =
      ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName +
                                " not found");
  }
  auto &componentInfo = ws->mutableComponentInfo();
  const auto componentIndex =
      componentInfo.indexOf(component->getComponentID());
  componentInfo.setRotation(componentIndex, rot);
}

/**
 * @brief LoadHelper::getComponentPosition
 * @param ws A MatrixWorkspace
 * @param componentName The Name of the component of the instrument
 * @return The position of the component
 */
V3D LoadHelper::getComponentPosition(API::MatrixWorkspace_sptr ws,
                                     const std::string &componentName) {
  Geometry::IComponent_const_sptr component =
      ws->getInstrument()->getComponentByName(componentName);
  if (!component) {
    throw std::invalid_argument("Instrument component " + componentName +
                                " not found");
  }
  V3D pos = component->getPos();
  return pos;
}

} // namespace DataHandling
} // namespace Mantid
