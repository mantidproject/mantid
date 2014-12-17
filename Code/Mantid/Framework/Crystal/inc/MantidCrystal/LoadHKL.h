#ifndef MANTID_CRYSTAL_LOADHKL_H_
#define MANTID_CRYSTAL_LOADHKL_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
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
  LoadHKL();
  ~LoadHKL();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadHKL"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads an ASCII .hkl file to a PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_LOADHKL_H_ */
