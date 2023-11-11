// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFunctionModel.h"

#include "Analysis/FitTabConstants.h"
#include "Analysis/IDAFunctionParameterEstimation.h"
#include "MantidKernel/PhysicalConstants.h"

#include <memory>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

constexpr double HBAR = Mantid::PhysicalConstants::h_bar / Mantid::PhysicalConstants::meV * 1e12;

auto const chudleyElliot = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  double L = 1.5;
  return std::unordered_map<std::string, double>{{"L", L}, {"Tau", (HBAR / y[1]) * (1 - sin(x[1] * L) / (L * x[1]))}};
};

auto const hallRoss = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  double L = 0.2;
  return std::unordered_map<std::string, double>{{"L", L},
                                                 {"Tau", (HBAR / y[1]) * (1 - exp((-x[1] * x[1] * L * L) / 2))}};
};

auto const teixeiraWater = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  double L = 1.5;
  double QL = x[1] * L;
  return std::unordered_map<std::string, double>{{"L", L}, {"Tau", (HBAR / y[1]) * ((QL * QL) / (6 + QL * QL))}};
};

auto const fickDiffusion = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  return std::unordered_map<std::string, double>{{"D", y[1] / (x[1] * x[1])}};
};

auto const estimators = std::unordered_map<std::string, IDAFunctionParameterEstimation::ParameterEstimator>{
    {"ChudleyElliot", chudleyElliot},
    {"HallRoss", hallRoss},
    {"TeixeiraWater", teixeiraWater},
    {"FickDiffusion", fickDiffusion}};

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

FqFunctionModel::FqFunctionModel()
    : SingleFunctionTemplateModel(std::make_unique<IDAFunctionParameterEstimation>(estimators)) {
  updateAvailableFunctions(FqFit::ALL_FITS);
}

} // namespace MantidQt::CustomInterfaces::IDA