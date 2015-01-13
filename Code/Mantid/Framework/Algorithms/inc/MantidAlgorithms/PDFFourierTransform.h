#ifndef MANTID_ALGORITHMS_PDFFourierTransform_H_
#define MANTID_ALGORITHMS_PDFFourierTransform_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** PDFFourierTransform : TODO: DESCRIPTION
 */
class DLLExport PDFFourierTransform : public API::Algorithm {
public:
  PDFFourierTransform();
  ~PDFFourierTransform();

  /// Algorithm's name for identification
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Fourier transform from S(Q) to G(r), which is paired distribution "
           "function (PDF). G(r) will be stored in another named workspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const;
  /// Algorithm's category for identification
  virtual const std::string
  category() const; // category better be in diffraction than general
  /// @copydoc Algorithm::validateInputs()
  virtual std::map<std::string, std::string> validateInputs();

private:
  /// Initialize the properties
  void init();
  /// Run the algorithm
  void exec();
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_PDFFourierTransform_H_ */
