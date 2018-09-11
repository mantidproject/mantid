#ifndef MANTID_CRYSTAL_LOADHKL_H_
#define MANTID_CRYSTAL_LOADHKL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_poly.h>

namespace Mantid {
namespace Crystal {

/** LoadHKL : Load an ISAW-style .hkl file
 * into a PeaksWorkspace
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-25
 */
class DLLExport LoadHKL : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadHKL"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an ASCII .hkl file to a PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"SaveHKL"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_LOADHKL_H_ */
