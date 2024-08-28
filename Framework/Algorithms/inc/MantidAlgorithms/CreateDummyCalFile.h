// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// To be compatible with MSVC++ Express Edition that does not have TR1 headers
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include <map>

namespace Mantid {
namespace Algorithms {
/** Create a calibration file for diffraction focussing (*.cal old Ariel format)
 *  based on list of names of the instrument tree.
 *  The offsets are all sets to zero and all detectors are selected. Detectors
 not assigned
 *  to any group will appear as group 0, i.e. not included when using
 AlignDetector or
 *  DiffractionFocussing algorithms.
 *  The group number is assigned based on a descent in the instrument tree
 assembly.
 *  If two assemblies are parented, say Bank1 and module1, and both assembly
 names
 *  are given in the GroupNames, they will get assigned different grouping
 numbers.
 *  This allows to isolate a particular sub-assembly of a particular leaf of the
 tree

    Required Properties:
    <UL>
    <LI> InstrumentName   - The name of the instrument. Needs to be present in
 the store</LI>
    <LI> GroupingFilename - The name of the output file (*.cal extension) .</LI>
    <LI> GroupNames       - Name of assemblies to consider (names separated by
 "/" or "," or "*"</LI>
    </UL>

    @author Vickie Lynch, SNS, ORNL
    @date 12/01/2010
*/
class MANTID_ALGORITHMS_DLL CreateDummyCalFile final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CreateDummyCalFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a calibration file (extension .cal) from a workspace by "
           "harvesting the detector ids from the instrument. All of the "
           "offsets will be zero, and the pixels will be all grouped into "
           "group one and the final column should be one. This will allow "
           "generating powder patterns from instruments that have not done a "
           "proper calibration.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ReadGroupsFromFile", "CreateCalFileByNames", "DiffractionFocussing",
            "LoadCalFile",        "SaveCalFile",          "MergeCalFiles"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling\\CalFiles"; }

private:
  /// Calibration entries map
  using instrcalmap = std::map<int, std::pair<int, int>>;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// The name and path of the input file
  std::string m_filename;
  /// Determine whether the grouping file already exists.
  /// @param filename :: GroupingFilename (extension .cal)
  /// @return true if the grouping file exists
  bool groupingFileDoesExist(const std::string &filename) const;
  void saveGroupingFile(const std::string &, bool overwrite) const;
  static void writeCalEntry(std::ostream &os, int number, int udet, double offset, int select, int group);
  void writeHeaders(std::ostream &os, const std::string &filename, bool overwrite) const;
  /// The names of the groups
  std::string groups;
  /// Calibration map used if the *.cal file exist. All entries in the *.cal
  /// file are registered with the udet number as the key and the
  /// <Number,Offset,Select,Group> as the tuple value.
  instrcalmap instrcalib;
};

} // namespace Algorithms
} // namespace Mantid
