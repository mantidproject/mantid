// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NEXUSGEOMETRY_CONSTANTS_H_
#define MANTID_NEXUSGEOMETRY_CONSTANTS_H_

#include "MantidNexusGeometry/DllConfig.h"
#include <string>

namespace Mantid {
namespace NexusGeometry {
namespace Constants {
const std::string NEXUS_STRUCTURE = "nexus_structure";
const std::string NX_CLASS = "NX_class";
const std::string NX_ENTRY = "NXentry";
const std::string NX_INSTRUMENT = "NXinstrument";
const std::string NX_DETECTOR = "NXdetector";
const std::string NX_DISK_CHOPPER = "NXdisk_chopper";
const std::string NX_MONITOR = "NXmonitor";
const std::string NX_SAMPLE = "NXsample";
const std::string NX_SOURCE = "NXsource";
const std::string NX_TRANSFORMATIONS = "NXtransformations";
const std::string DETECTOR_IDS = "detector_number";
const std::string DETECTOR_ID = "detector_id";
const std::string X_PIXEL_OFFSET = "x_pixel_offset";
const std::string Y_PIXEL_OFFSET = "y_pixel_offset";
const std::string Z_PIXEL_OFFSET = "z_pixel_offset";
const std::string DEPENDS_ON = "depends_on";
const std::string NO_DEPENDENCY = ".";
const std::string PIXEL_SHAPE = "pixel_shape";
const std::string DETECTOR_SHAPE = "detector_shape";
const std::string SHAPE = "shape";
// Transformation types
const std::string TRANSFORMATION_TYPE = "transformation_type";
const std::string TRANSLATION = "translation";
const std::string ROTATION = "rotation";
const std::string VECTOR = "vector";
const std::string UNITS = "units";
// Radians and degrees
constexpr double PI = 3.1415926535;
constexpr double DEGREES_IN_SEMICIRCLE = 180;
// Nexus shape types
const std::string NX_CYLINDER = "NXcylindrical_geometry";
const std::string NX_OFF = "NXoff_geometry";
const std::string BANK_NAME = "local_name";
} // namespace Constants

} // namespace NexusGeometry
} // namespace Mantid

#endif /* MANTID_NEXUSGEOMETRY_CONSTANTS_H_ */
