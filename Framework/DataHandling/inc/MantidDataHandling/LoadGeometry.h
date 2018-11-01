// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADGEOMETRY_H_
#define MANTID_DATAHANDLING_LOADGEOMETRY_H_

#include <string>

namespace Mantid {
namespace DataHandling {
/**
Loads an instrument definition file into a workspace, with the purpose of being
able to visualise an instrument without requiring to read in a ISIS raw datafile
first.
The name of the algorithm refers to the fact that an instrument
is loaded into a workspace but without any real data - hence the reason for
referring to
it as an 'empty' instrument.

Required Properties:
<UL>
<LI> Filename - The name of an instrument definition file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
instrument</LI>
</UL>

Optional Properties: (note that these options are not available if reading a
multiperiod file)
<UL>
<LI> detector_value  - This value affect the colour of the detectorss in the
instrument display window</LI>
<LI> monitor_value  - This value affect the colour of the monitors in the
instrument display window</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 31/10/2008
*/
namespace LoadGeometry {

bool isIDF(const std::string &filename, const std::string &instrumentname);
bool isNexus(const std::string &filename);

} // namespace LoadGeometry

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADGEOMETRY_H_*/
