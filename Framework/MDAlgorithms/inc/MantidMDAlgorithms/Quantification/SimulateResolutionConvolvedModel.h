// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
