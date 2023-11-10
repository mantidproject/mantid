// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDTemplateBrowser.h"

#include "Analysis/FitTabConstants.h"
#include "Analysis/IDAFunctionParameterEstimation.h"
#include "MantidAPI/IFunction.h"

#include <memory>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

IDAFunctionParameterEstimation createParameterEstimation() {
  auto estimateMsd = [](::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {
    auto y = estimationData.y;
    auto x = estimationData.x;
    if (x.size() != 2 || y.size() != 2) {
      return;
    }
    double Msd = 6 * log(y[0] / y[1]) / (x[1] * x[1]);
    // If MSD less than zero, reject the estimate and set to default value of
    // 0.05, which leads to a (roughly) flat line
    Msd = Msd > 0 ? Msd : 0.05;
    function->setParameter("Msd", Msd);
    function->setParameter("Height", y[0]);
  };
  IDAFunctionParameterEstimation parameterEstimation;
  parameterEstimation.addParameterEstimationFunction("MsdGauss", estimateMsd);
  parameterEstimation.addParameterEstimationFunction("MsdPeters", estimateMsd);
  parameterEstimation.addParameterEstimationFunction("MsdYi", estimateMsd);

  return parameterEstimation;
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

MSDTemplateBrowser::MSDTemplateBrowser()
    : SingleFunctionTemplateBrowser(MSDFit::ALL_FITS,
                                    std::make_unique<IDAFunctionParameterEstimation>(createParameterEstimation())) {}

} // namespace MantidQt::CustomInterfaces::IDA