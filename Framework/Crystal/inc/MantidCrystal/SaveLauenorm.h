// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a lauenorm format
 * http://www.ccp4.ac.uk/cvs/viewvc.cgi/laue/doc/lauenorm.ptx?diff_format=s&revision=1.1.1.1&view=markup
 *
 * @author Vickie Lynch, SNS
 * @date 2014-07-24
 */

class MANTID_CRYSTAL_DLL SaveLauenorm : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveLauenorm"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a PeaksWorkspace to a ASCII file for each detector."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  DataObjects::PeaksWorkspace_sptr ws;
  void sizeBanks(const std::string &bankName, int &nCols, int &nRows);

  const std::vector<std::string> m_typeList{"TRICLINIC", "MONOCLINIC",   "ORTHORHOMBIC", "TETRAGONAL",
                                            "HEXAGONAL", "RHOMBOHEDRAL", "CUBIC"};

  const std::vector<std::string> m_centeringList{"P", "A", "B", "C", "I", "F", "R"};
};

} // namespace Crystal
} // namespace Mantid
