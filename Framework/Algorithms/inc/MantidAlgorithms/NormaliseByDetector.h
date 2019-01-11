// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_
#define MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
/// Forward declaration for MatrixWorkspace.
class MatrixWorkspace;
class Progress;
} // namespace API
namespace Algorithms {
/** NormaliseByDetector : Normalises a workspace with respect to the detector
  efficiency function stored against components in the instrument parameters.
  See wiki for more details.
  Detector efficiency functions are calculated using the wavelengths in the
  input workspace.

  @date 2012-07-17
*/
class DLLExport NormaliseByDetector : public API::Algorithm {
public:
  NormaliseByDetector(bool parallelExecution = true);

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Normalise the input workspace by the detector efficiency.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Divide"}; }
  const std::string category() const override;

private:
  /// Flag to indicate that the histograms should be processed in parallel.
  const bool m_parallelExecution;
  /// Try to parse a function parameter and extract the correctly typed
  /// parameter.
  const Mantid::Geometry::FitParameter
  tryParseFunctionParameter(Mantid::Geometry::Parameter_sptr parameter,
                            const Geometry::IDetector &det);
  /// Block to process histograms.
  boost::shared_ptr<Mantid::API::MatrixWorkspace>
  processHistograms(boost::shared_ptr<Mantid::API::MatrixWorkspace> inWS);
  /// Process indivdual histogram.
  void processHistogram(
      size_t wsIndex,
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> inWS,
      boost::shared_ptr<Mantid::API::MatrixWorkspace> denominatorWS,
      Mantid::API::Progress &prog);

  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_NORMALISEBYDETECTOR_H_ */
