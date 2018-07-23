/**
 * This is a flexible NeXus file loader. It takes as input a filename
 * and a dictionary file and loads the data from the file into a
 * suitable workspace. The dictionary can contain to types of lines:
 * Lines starting with # are ignored and treated as comments
 * Then there are lines: property=value These define how the workspace
 * is constructed. Value can either be an value or a path into the NeXus
 * file. Data will be loaded from that path then. For property there are
 * some special values:
 *
 * data defines the path to the main data item. This will determine the
 *       dimensionality and type of the resulting workspace.
 * x,y,z-axis  is the path to the data for the appropriate axis
 * x,y,z-axis-name is the name of the axis
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 * Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 * This file is part of Mantid.

 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

#ifndef FLEXINEXUSLOADER_H_
#define FLEXINEXUSLOADER_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include "MantidDataObjects/Workspace2D.h"
#include <boost/shared_array.hpp>
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

class MANTID_SINQ_DLL LoadFlexiNexus : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadFlexiNexus"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a NeXus file directed by a dictionary file";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Nexus"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  // A dictionary
  std::map<std::string, std::string> dictionary;

  void loadDictionary(std::string dictionaryFile);

  void load2DWorkspace(NeXus::File *fin);

  void loadMD(NeXus::File *fin);

  void readData(NeXus::File *fin);

  /// Personal wrapper for sqrt to allow msvs to compile
  static double dblSqrt(double in);

  Mantid::Geometry::MDHistoDimension_sptr makeDimension(NeXus::File *fin,
                                                        int index, int length);

  std::unordered_set<std::string> populateSpecialMap();

  void addMetaData(NeXus::File *fin, Mantid::API::Workspace_sptr ws,
                   Mantid::API::ExperimentInfo_sptr info);

  int safeOpenpath(NeXus::File *fin, std::string path);
  int calculateCAddress(int *pos, int *dim, int rank);
  int calculateF77Address(int *pos, int rank);
};

#endif /*FLEXINEXUSLOADER_H_*/
