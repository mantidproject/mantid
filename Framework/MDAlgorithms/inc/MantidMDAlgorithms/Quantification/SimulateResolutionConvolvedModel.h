#ifndef MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_
#define MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_

#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h"

namespace Mantid {
namespace API {
class IFunction;
class FunctionDomainMD;
class FunctionValues;
} // namespace API

namespace MDAlgorithms {

/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport SimulateResolutionConvolvedModel
    : public FitResolutionConvolvedModel {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Runs a simulation of a model with a selected resolution function";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"FitResolutionConvolvedModel"};
  }
  int version() const override;

private:
  /// Returns the number of iterations that should be performed
  int niterations() const override;
  void init() override;
  void exec() override;

  /// Create the MD function instance
  boost::shared_ptr<API::IFunction> createFunction() const;
  /// Create the input & output domains from the input workspace
  void createDomains();
  /// Generate the output MD workspace that is a result of the simulation
  void createOutputWorkspace();

  /// The input workspace
  API::IMDEventWorkspace_sptr m_inputWS;
  /// The input domain
  boost::shared_ptr<API::FunctionDomainMD> m_domain;
  /// The input domain
  boost::shared_ptr<API::FunctionValues> m_calculatedValues;
  /// The output workspace type
  using QOmegaWorkspace =
      DataObjects::MDEventWorkspace<DataObjects::MDEvent<4>, 4>;

  /// The output workspace
  boost::shared_ptr<QOmegaWorkspace> m_outputWS;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SIMULATERESOLUTIONCONVOLVEDMODEL_H_ */
