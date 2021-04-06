// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <H5Cpp.h>
#include <string>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

const H5G_obj_t GROUP_TYPE = static_cast<H5G_obj_t>(0);
const H5G_obj_t DATASET_TYPE = static_cast<H5G_obj_t>(1);
const std::vector<std::string> nexus_geometry_extensions = {".nxs", ".hdf5"};

const H5std_string NX_CLASS = "NX_class";
const H5std_string NX_SAMPLE = "NXsample";
const H5std_string NX_DETECTOR = "NXdetector";
const H5std_string NX_MONITOR = "NXmonitor";
const H5std_string NX_DISK_CHOPPER = "NXdisk_chopper";
const H5std_string NX_ENTRY = "NXentry";
const H5std_string NX_CYLINDER = "NXcylindrical_geometry";
const H5std_string NX_OFF = "NXoff_geometry";
const H5std_string NX_INSTRUMENT = "NXinstrument";
const H5std_string NX_SOURCE = "NXsource";
const H5std_string NX_TRANSFORMATIONS = "NXtransformations";

const H5std_string SHORT_NAME = "short_name";
const H5std_string TRANSFORMATIONS = "transformations";
const H5std_string TRANSLATION = "translation";
const H5std_string ROTATION = "rotation";
const H5std_string NO_DEPENDENCY = ".";
const H5std_string LOCATION = "location";
const H5std_string ORIENTATION = "orientation";
const H5std_string DEPENDS_ON = "depends_on";
const H5std_string X_PIXEL_OFFSET = "x_pixel_offset";
const H5std_string Y_PIXEL_OFFSET = "y_pixel_offset";
const H5std_string Z_PIXEL_OFFSET = "z_pixel_offset";
const H5std_string BANK_NAME = "local_name";
const H5std_string PIXEL_SHAPE = "pixel_shape";
const H5std_string DETECTOR_SHAPE = "detector_shape";
const H5std_string SHAPE = "shape";

// these strings belong to DataSets which are duplicates of each other. written
// to NXmonitor group to handle the naming inconsistency. probably temporary.
const H5std_string DETECTOR_IDS = "detector_number";
const H5std_string DETECTOR_ID = "detector_id";

// Processed Nexus definitions
const H5std_string SPECTRA_COUNTS = "detector_count"; // Number of detectors contributing to each spectra. NOT
                                                      // Nexus compliant
const H5std_string SPECTRA_NUMBERS = "spectra";       // NOT Nexus compliant
const H5std_string DETECTOR_LIST = "detector_list";   // Note this is identical to
                                                      // DETECTOR_ID, but is NOT
                                                      // Nexus compliant
const H5std_string DETECTOR_INDEX = "detector_index"; // indices of detectors contributing to spectra. NOT Nexus
                                                      // compliant

const H5std_string TRANSFORMATION_TYPE = "transformation_type";

const H5std_string METRES = "m";
const H5std_string DEGREES = "degrees";
const H5std_string NAME = "name";
const H5std_string UNITS = "units";
const H5std_string VECTOR = "vector";

const double DEGREES_IN_SEMICIRCLE = 180.0;
const double PRECISION = 1e-5;

const H5std_string DEFAULT_ROOT_ENTRY_NAME = "raw_data_1";
const H5::DataSpace SCALAR(H5S_SCALAR);

// JSON PARSER SPECIFICS
const std::string NEXUS_STRUCTURE = "nexus_structure";

} // namespace NexusGeometry
} // namespace Mantid
