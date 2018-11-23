// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADGEOMETRY_H_
#define MANTID_DATAHANDLING_LOADGEOMETRY_H_

#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {
/**
Common methods for LoadInstrument.cpp and LoadEmptyInstrument.cpp

@author Neil Vaytet, ESS
@date 01/11/2018
*/
namespace LoadGeometry {

/// Determine if the Geometry file type is IDF
bool isIDF(const std::string &filename);
/// Determine if the Geometry file type is Nexus
bool isNexus(const std::string &filename);
/// List allowed file extensions for geometry
const std::vector<std::string> validExtensions();

} // namespace LoadGeometry

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADGEOMETRY_H_*/
