// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFunctionModel.h"

#include "Analysis/FitTabConstants.h"
#include "Analysis/ParameterEstimation.h"

#include <memory>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

auto const msd = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  double Msd = 6 * log(y[0] / y[1]) / (x[1] * x[1]);
  // If MSD less than zero, reject the estimate and set to default value of
  // 0.05, which leads to a (roughly) flat line
  Msd = Msd > 0 ? Msd : 0.05;
  return std::unordered_map<std::string, double>{{"Msd", Msd}, {"Height", y[0]}};
};

auto const estimators = std::unordered_map<std::string, IDAFunctionParameterEstimation::ParameterEstimator>{
    {"MsdGauss", msd}, {"MsdPeters", msd}, {"MsdYi", msd}};

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

MSDFunctionModel::MSDFunctionModel()
    : SingleFunctionTemplateModel(std::make_unique<IDAFunctionParameterEstimation>(estimators)) {
  updateAvailableFunctions(MSDFit::ALL_FITS);
}

} // namespace MantidQt::CustomInterfaces::IDA