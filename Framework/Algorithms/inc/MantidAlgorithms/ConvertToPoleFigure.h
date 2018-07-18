#ifndef MANTID_ALGORITHMS_ConvertToPoleFigure_H_
#define MANTID_ALGORITHMS_ConvertToPoleFigure_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** ConvertToPoleFigure : Calcualte Pole Figure for engineering material
 */
class DLLExport ConvertToPoleFigure : public API::Algorithm {
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
  void convertToPoleFigure();

  void rotateVectorQ(Kernel::V3D unitQ, const double &hrot, const double &omega,
                     double &r_td, double &r_nd);

  /// generatae output workspace and set output properties
  void generateOutputs();

  /// generate output MDEventWorkspace
  void generateMDEventWS();

  /// input workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// input counts
  API::MatrixWorkspace_const_sptr m_countWS;

  /// sample log name
  std::string m_nameHROT;
  std::string m_nameOmega;

  /// vector to record pole figure
  std::vector<double> m_poleFigureRTDVector;
  std::vector<double> m_poleFigureRNDVector;
  std::vector<double> m_poleFigurePeakIntensityVector;

  ///
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_ConvertToPoleFigure_H_ */
