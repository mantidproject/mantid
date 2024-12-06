// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreateCalFileByNames.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"

#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <fstream>
#include <queue>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CreateCalFileByNames)

using namespace Kernel;
using API::FileProperty;
using API::Progress;
using Geometry::Instrument_const_sptr;

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CreateCalFileByNames::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InstrumentWorkspace", "", Direction::Input,
                                                        std::make_shared<InstrumentValidator>()),
                  "A workspace that contains a reference to the instrument of interest. "
                  "You can use LoadEmptyInstrument to create such a workspace.");
  declareProperty(std::make_unique<FileProperty>("GroupingFileName", "", FileProperty::Save, ".cal"),
                  "The name of the output CalFile");
  declareProperty("GroupNames", "",
                  "A string of the instrument component names to use as separate groups. "
                  "/ or , can be used to separate multiple groups.");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read
 *successfully
 *  @throw runtime_error If unable to run one of the Child Algorithms
 *successfully
 */

void CreateCalFileByNames::exec() {
  MatrixWorkspace_const_sptr ws = getProperty("InstrumentWorkspace");

  // Get the instrument.
  Instrument_const_sptr inst = ws->getInstrument();

  // Get the names of groups
  std::string groupsname = getProperty("GroupNames");
  groups = groupsname;

  // Split the names of the group and insert in a vector, throw if group empty
  std::vector<std::string> vgroups;
  boost::split(vgroups, groupsname, boost::algorithm::detail::is_any_ofF<char>(",/*"));
  if (vgroups.empty()) {
    g_log.error("Could not determine group names. Group names should be "
                "separated by / or ,");
    throw std::runtime_error("Could not determine group names. Group names "
                             "should be separated by / or ,");
  }

  // Assign incremental number to each group
  std::map<std::string, int> group_map;
  int index = 0;
  for (auto &vgroup : vgroups) {
    boost::trim(vgroup);
    group_map[vgroup] = ++index;
  }

  // Not needed anymore
  vgroups.clear();

  // Find Detectors that belong to groups
  using sptr_ICompAss = std::shared_ptr<const Geometry::ICompAssembly>;
  using sptr_IComp = std::shared_ptr<const Geometry::IComponent>;
  using sptr_IDet = std::shared_ptr<const Geometry::IDetector>;
  std::queue<std::pair<sptr_ICompAss, int>> assemblies;
  sptr_ICompAss current = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(inst);
  sptr_IDet currentDet;
  sptr_IComp currentIComp;
  sptr_ICompAss currentchild;

  int top_group, child_group;

  if (current.get()) {
    top_group = group_map[current->getName()]; // Return 0 if not in map
    assemblies.emplace(current, top_group);
  }

  std::string filename = getProperty("GroupingFilename");

  // Check if a template cal file is given
  bool overwrite = groupingFileDoesExist(filename);

  int number = 0;
  Progress prog(this, 0.0, 0.8, assemblies.size());
  while (!assemblies.empty()) // Travel the tree from the instrument point
  {
    current = assemblies.front().first;
    top_group = assemblies.front().second;
    assemblies.pop();
    int nchilds = current->nelements();
    if (nchilds != 0) {
      for (int i = 0; i < nchilds; ++i) {
        currentIComp = (*(current.get()))[i]; // Get child
        currentDet = std::dynamic_pointer_cast<const Geometry::IDetector>(currentIComp);
        if (currentDet.get()) // Is detector
        {
          if (overwrite) // Map will contains udet as the key
            instrcalib[currentDet->getID()] = std::make_pair(number++, top_group);
          else // Map will contains the entry number as the key
            instrcalib[number++] = std::make_pair(currentDet->getID(), top_group);
        } else // Is an assembly, push in the queue
        {
          currentchild = std::dynamic_pointer_cast<const Geometry::ICompAssembly>(currentIComp);
          if (currentchild.get()) {
            child_group = group_map[currentchild->getName()];
            if (child_group == 0)
              child_group = top_group;
            assemblies.emplace(currentchild, child_group);
          }
        }
      }
    }
    prog.report();
  }
  // Write the results in a file
  saveGroupingFile(filename, overwrite);
  progress(0.2);
}

bool CreateCalFileByNames::groupingFileDoesExist(const std::string &filename) const {
  std::ifstream file(filename.c_str());
  // Check if the file already exists
  if (!file)
    return false;
  file.close();
  std::ostringstream mess;
  mess << "Calibration file " << filename << " already exist. Only grouping will be modified";
  g_log.information(mess.str());
  return true;
}

/// Creates and saves the output file
void CreateCalFileByNames::saveGroupingFile(const std::string &filename, bool overwrite) const {
  std::ostringstream message;
  std::fstream outfile;
  std::fstream infile;
  if (!overwrite) // Open the file directly
  {
    outfile.open(filename.c_str(), std::ios::out);
    if (!outfile.is_open()) {
      message << "Can't open Calibration File: " << filename;
      g_log.error(message.str());
      throw std::runtime_error(message.str());
    }
  } else {
    infile.open(filename.c_str(), std::ios::in);
    std::string newfilename = filename + "2";
    outfile.open(newfilename.c_str(), std::ios::out);
    if (!infile.is_open()) {
      message << "Can't open input Calibration File: " << filename;
      g_log.error(message.str());
      throw std::runtime_error(message.str());
    }
    if (!outfile.is_open()) {
      message << "Can't open new Calibration File: " << newfilename;
      g_log.error(message.str());
      throw std::runtime_error(message.str());
    }
  }

  // Write the headers
  writeHeaders(outfile, filename, overwrite);

  if (overwrite) {
    int number, udet, select, group;
    double offset;

    std::string str;
    while (getline(infile, str)) {
      if (str.empty() || str[0] == '#') // Skip the headers
        continue;
      std::istringstream istr(str);
      istr >> number >> udet >> offset >> select >> group;
      auto it = instrcalib.find(udet);
      if (it == instrcalib.end()) // Not found, don't assign a group
        group = 0;
      else
        group = ((*it).second).second; // If found then assign new group
      // write out
      writeCalEntry(outfile, number, udet, offset, select, group);
    }
  } else //
  {
    for (const auto &value : instrcalib)
      writeCalEntry(outfile, value.first, (value.second).first, 0.0, 1, (value.second).second);
  }

  // Closing
  outfile.close();
  if (overwrite)
    infile.close();
}

/// Writes a single calibration line to the output file
void CreateCalFileByNames::writeCalEntry(std::ostream &os, int number, int udet, double offset, int select, int group) {
  os << std::fixed << std::setw(9) << number << std::fixed << std::setw(15) << udet << std::fixed
     << std::setprecision(7) << std::setw(15) << offset << std::fixed << std::setw(8) << select << std::fixed
     << std::setw(8) << group << "\n";
}

/// Writes out the header to the output file
void CreateCalFileByNames::writeHeaders(std::ostream &os, const std::string &filename, bool overwrite) const {
  os << "# Diffraction focusing calibration file created by Mantid"
     << "\n";
  os << "# Detectors have been grouped using assembly names:" << groups << "\n";
  if (overwrite) {
    os << "# Template file " << filename << " has been used"
       << "\n";
    os << "# Only grouping has been changed, offset from template file have "
          "been copied"
       << "\n";
  } else
    os << "# No template file, all offsets set to 0.0 and select to 1"
       << "\n";

  os << "#  Number           UDET         offset      select  group"
     << "\n";
}

} // namespace Mantid::Algorithms
