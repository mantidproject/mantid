// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadISISNexusHelper.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
namespace {
static constexpr size_t RUN_TIME_STRING_LENGTH = 19;
} // namespace

namespace Mantid {

namespace DataHandling {

namespace LoadISISNexusHelper {

using namespace Kernel;
using namespace API;
using namespace NeXus;
using std::size_t;

/**
 * Find total number of spectra in the nexus file
 *   @param entry :: The Nexus entry
 *   @param hasVMSBlock :: Whether the current nexus entry has a vms_compat
 * block
 *   @return Total number of spectra
 */
int64_t findNumberOfSpectra(const NXEntry &entry, const bool hasVMSBlock) {

  if (hasVMSBlock) {
    NXInt nsp1 = entry.openNXInt("isis_vms_compat/NSP1");
    nsp1.load();
    return nsp1[0];
  } else {
    NXClass det_class = entry.openNXGroup("detector_1");
    NXInt spectrum_index = det_class.openNXInt("spectrum_index");
    // Not clear what this variable is, number of detectors or number of
    // detectors + monitors if it is number of detectors, that is just the
    // length of the spectra
    spectrum_index.load();
    return spectrum_index.dim0();
  }
}

/**
 * Find total number of spectra in the nexus file
 *   @param entry :: The Nexus entry
 *   @param hasVMSBlock :: Whether the current nexus entry has a vms_compat
 * block
 *   @return Returns a pair of Detector IDs and Spectrum numbers
 */
std::tuple<NXInt, NXInt>
findDetectorIDsAndSpectrumNumber(const NXEntry &entry, const bool hasVMSBlock) {
  if (hasVMSBlock) {
    NXInt udet = entry.openNXInt("isis_vms_compat/UDET");
    udet.load();
    NXInt spec = entry.openNXInt("isis_vms_compat/SPEC");
    spec.load();
    return std::make_tuple(udet, spec);
  } else {
    NXClass det_class = entry.openNXGroup("detector_1");
    NXInt spectrum_index = det_class.openNXInt("spectrum_index");
    spectrum_index.load();
    // Currently there is no UDET variable anywhere, so
    // setting it to spectrum index assumes a 1 to 1 correspondence between
    // detector id and spectrum i.e detector 1 maps to spectrum 1
    NXInt udet = spectrum_index;
    NXInt spec = spectrum_index;
    return std::make_tuple(udet, spec);
  }
}

/**
 * Load geometrical data about the sample from the nexus entry into a workspace
 *   @param Sample :: The sample which the geometrical data will be saved into
 *   @param entry :: The Nexus entry
 *   @param hasVMSBlock :: Whether the current nexus entry has a vms_compat
 * block
 */
void loadSampleGeometry(Sample &sample, const NXEntry &entry,
                        const bool hasVMSBlock) {

  if (hasVMSBlock) {
    NXInt spb = entry.openNXInt("isis_vms_compat/SPB");
    // Just load the index we need, not the whole block. The flag is the
    // third value in the array
    spb.load(1, 2);
    int geom_id = spb[0];
    sample.setGeometryFlag(geom_id);

    NXFloat rspb = entry.openNXFloat("isis_vms_compat/RSPB");
    // Just load the indices we need, not the whole block. The values start
    // from the 4th onward
    rspb.load(3, 3);
    double thick(rspb[0]);
    double height(rspb[1]);
    double width(rspb[2]);
    sample.setThickness(thick);
    sample.setHeight(height);
    sample.setWidth(width);

  } else {
    NXClass sampleEntry = entry.openNXGroup("sample");
    std::string id = sampleEntry.getString("id");
    int geom_id(0);
    if (!id.empty()) {
      geom_id = std::stoi(id);
    }
    sample.setGeometryFlag(geom_id);

    double thick = sampleEntry.getFloat("thickness");
    double height = sampleEntry.getFloat("height");
    double width = sampleEntry.getFloat("width");
    sample.setThickness(thick);
    sample.setHeight(height);
    sample.setWidth(width);
  }
}

/**
 * Load data about the run
 *   @param Run :: The run where the information will be stored
 *   @param entry :: The Nexus entry
 */
void loadRunDetails(API::Run &runDetails, const NXEntry &entry,
                    const bool hasVMSBlock) {

  // Charge is stored as a float
  double proton_charge = static_cast<double>(entry.getFloat("proton_charge"));
  runDetails.setProtonCharge(proton_charge);

  std::string run_num = std::to_string(entry.getInt("run_number"));
  runDetails.addProperty("run_number", run_num);

  // End date and time is stored separately in ISO format in the
  // "raw_data1/endtime" class
  NXChar char_data = entry.openNXChar("end_time");
  char_data.load();

  std::string end_time_iso = std::string(char_data(), RUN_TIME_STRING_LENGTH);
  runDetails.addProperty("run_end", end_time_iso);

  char_data = entry.openNXChar("start_time");
  char_data.load();

  std::string start_time_iso = std::string(char_data(), RUN_TIME_STRING_LENGTH);

  runDetails.addProperty("run_start", start_time_iso);

  // Some details are only stored in the VMS comparability block so we'll
  // everything from there
  // for consistency
  if (hasVMSBlock) {

    NXClass vms_compat = entry.openNXGroup("isis_vms_compat");

    // RPB struct info
    NXInt rpb_int = vms_compat.openNXInt("IRPB");
    rpb_int.load();
    runDetails.addProperty(
        "freq",
        rpb_int[6]); // 2**k where source frequency = 50 / 2**k
    // Now double data
    NXFloat rpb_dbl = vms_compat.openNXFloat("RRPB");
    rpb_dbl.load();
    runDetails.addProperty(
        "gd_prtn_chrg",
        static_cast<double>(rpb_dbl[7])); // good proton charge (uA.hour)
    runDetails.addProperty(
        "tot_prtn_chrg",
        static_cast<double>(rpb_dbl[8])); // total proton charge (uA.hour)
    runDetails.addProperty("goodfrm", rpb_int[9]);      // good frames
    runDetails.addProperty("rawfrm", rpb_int[10]);      // raw frames
    runDetails.addProperty("rb_proposal", rpb_int[21]); // RB (proposal)
    vms_compat.close();
  } else {

    NXFloat floatData = entry.openNXFloat("duration");
    floatData.load();
    runDetails.addProperty("dur", static_cast<double>(floatData[0]),
                           floatData.attributes("units"), true);

    // These variables have changed, gd_prtn_chrg is now proton_charge
    floatData = entry.openNXFloat("proton_charge");
    floatData.load();
    runDetails.addProperty("gd_prtn_chrg", static_cast<double>(floatData[0]),
                           floatData.attributes("units"), true);
    // Total_proton_charge is now proton_charge_raw
    floatData = entry.openNXFloat("proton_charge_raw");
    floatData.load();
    runDetails.addProperty("tot_prtn_chrg", static_cast<double>(floatData[0]),
                           floatData.attributes("units"), true);

    if (entry.containsGroup("instrument/source/frequency")) {
      runDetails.addProperty("freq",
                             entry.getFloat("instrument/source/frequency"));
    } else {
      // If no entry, assume 50hz source (suggested by Freddie Akeroyd)
      runDetails.addProperty("freq", 50, "Hz", true);
    }

    runDetails.addProperty("goodfrm", entry.getInt("good_frames"));
    runDetails.addProperty("rawfrm", entry.getInt("raw_frames"));
    runDetails.addProperty(
        "rb_proposal",
        entry.getString("experiment_identifier")); // RB (proposal)
  }
}
} // namespace LoadISISNexusHelper
} // namespace DataHandling
} // namespace Mantid