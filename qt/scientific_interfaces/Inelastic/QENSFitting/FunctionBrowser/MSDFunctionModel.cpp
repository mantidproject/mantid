// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFunctionModel.h"

#include "../FitTabConstants.h"
#include "../ParameterEstimation.h"

#include <memory>

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

auto const msd = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  double Msd = 6 * log(y[0] / y[1]) / (x[1] * x[1]);
  // If MSD less than zero, reject the estimate and set to default value of
  // 0.05, which leads to a (roughly) flat line
  Msd = Msd > 0 ? Msd : 0.05;
  return std::unordered_map<std::string, double>{{"Msd", Msd}, {"Height", y[0]}};
};

auto const estimators = std::unordered_map<std::string, FunctionParameterEstimation::ParameterEstimator>{
    {"MsdGauss", msd}, {"MsdPeters", msd}, {"MsdYi", msd}};

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

MSDFunctionModel::MSDFunctionModel()
    : SingleFunctionTemplateModel(std::make_unique<FunctionParameterEstimation>(estimators)) {
  updateAvailableFunctions(MSD::ALL_FITS);
}

} // namespace MantidQt::CustomInterfaces::Inelastic