// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SAVEISAWUB_H_
#define MANTID_CRYSTAL_SAVEISAWUB_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Algorithm to save  a UB matrix and lattice parameters to an ISAW-style
 * ASCII file.
 *
 * @author Ruth Mikkelson
 * @date 2011-08-10
 *
 *
 *
 */

class DLLExport SaveIsawUB : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawUB"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a UB matrix and lattice parameters from a workspace to an "
           "ISAW-style ASCII file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadIsawUB"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  // Calculates the error in the volume
  double getErrorVolume(const Geometry::OrientedLattice &lattice);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_LOADISAWUB_H_ */
