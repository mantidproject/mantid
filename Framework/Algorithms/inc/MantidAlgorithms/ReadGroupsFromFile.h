// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"
#include <unordered_map>

namespace Mantid {
namespace Algorithms {
/** Read a diffraction calibration file (*.cal) and an instrument name, and
 output a 2D workspace
 * containing on the Y-axis the values of the Group each detector belongs to.
 * This is used to visualise the grouping scheme for powder diffractometers,
 where a large number of detectors
 * are grouped together. The output 2D workspace can be visualize using the show
 instrument method.
 * The format of the *.cal file is as follows:
 *
 *   # Format:
 *   number   UDET offset  select  group
 *   0        611  0.0000000  1    0
 *   1        612  0.0000000  1    0
 *   2        601  0.0000000  0    0
 *   3        602  0.0000000  0    0
 *   4        621  0.0000000  1    0
 *   The first column is simply an index, the second is a UDET identifier for
 the detector,
 *   the third column corresponds to an offset in Deltad/d (not applied, this can be interpreted by the ApplyDiffCal
 *   algorithm). The forth column is a flag to indicate
 whether the detector
 *   is selected. The fifth column indicates the group this detector belongs to
 (number >=1),
 *   zero is not considered as a group.
 *   Required Properties:
 *   <UL>
 *   <LI> InstrumentName   - The name of the instrument. Needs to be present in
 the store </LI>
 *   <LI> GroupingFilename - The name of the output file (*.cal extension) </LI>
 *   <LI> ShowUnselected   - Option (true or false) to consider unselected
 detectors in the cal file </LI>
 *   <LI> OuputWorkspace   - The name of the output 2D Workspace containing the
 group information </LI>
 *   </UL>

    @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
    @date 09/03/2009
*/
class MANTID_ALGORITHMS_DLL ReadGroupsFromFile final : public API::Algorithm {
public:
  /// (Empty) Constructor
  ReadGroupsFromFile();
  /// Algorithm's name
  const std::string name() const override { return "ReadGroupsFromFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Read a diffraction calibration file (*.cal) or an XML grouping "
           "file (*.xml) and an instrument name, and output a 2D workspace "
           "containing on the Y-axis the values of the Group each detector "
           "belongs to.  This is used to visualise the grouping scheme for "
           "powder diffractometers, where a large number of detectors are "
           "grouped together. The output 2D workspace can be visualize using "
           "the show instrument method.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CreateDummyCalFile", "CreateCalFileByNames", "DiffractionFocussing",
            "LoadCalFile",        "SaveCalFile",          "MergeCalFiles"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling\\CalFiles"; }

private:
  /// Map containing the detector entries found in the *.cal file. The key is
  /// the udet number, the value of is a pair of <group,selected>.
  using calmap = std::unordered_map<int, std::pair<int, int>>;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Read a grouping file and construct the calibration map
  void readGroupingFile(const std::string &filename);
  /// Read an XML Grouping File
  void readXMLGroupingFile(const std::string &filename);
  /// Child Algorithm to Load the associated empty instrument
  /// @param instrument_xml_name :: The instrument xml name including
  /// extension(.xml or .XML) but no path
  /// this is determine by the mantid instrument.directory
  /// @return Shared pointer to the 2D workspace
  DataObjects::Workspace2D_sptr loadEmptyInstrument(const std::string &instrument_xml_name);
  /// Calibration map containing the detector entries found in the *.cal file.
  /// The key is the udet number, the value of is a pair of <group,selected>.
  calmap calibration;
};

} // namespace Algorithms
} // namespace Mantid
