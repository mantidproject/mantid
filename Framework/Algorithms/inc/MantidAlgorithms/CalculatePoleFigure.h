#ifndef MANTID_ALGORITHMS_CalculatePoleFigure_H_
#define MANTID_ALGORITHMS_CalculatePoleFigure_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePoleFigure : Calcualte Pole Figure for engineering material
 */
class DLLExport CalculatePoleFigure : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "function (PDF). G(r) will be stored in another named workspace.";
  }

  /// Algorithm's version for identification
  int version() const override;
  /// Algorithm's category for identification
  const std::string category() const override;

private:
  /// Initialize the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// process inputs
  void processInputs();

  /// calculate pole figure
  void calculatePoleFigure();
  double calculatePeakIntensitySimple(size_t iws, double dmin, double dmax);

  /// input workspace
  API::MatrixWorkspace_const_sptr m_inputWS;

  /// sample log name
  std::string m_nameHROT;
  std::string m_nameOmega;
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_CalculatePoleFigure_H_ */
