// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqTemplateBrowser.h"

#include "Analysis/FQFitConstants.h"
#include "Analysis/IDAFunctionParameterEstimation.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/PhysicalConstants.h"

#include <memory>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

constexpr double HBAR = Mantid::PhysicalConstants::h / Mantid::PhysicalConstants::meV * 1e12 / (2 * M_PI);

void estimateChudleyElliot(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 1.5;
  double tau = (HBAR / y[1]) * (1 - sin(x[1] * L) / (L * x[1]));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateHallRoss(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 0.2;
  double tau = (HBAR / y[1]) * (1 - exp((-x[1] * x[1] * L * L) / 2));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateTeixeiraWater(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {

  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  double L = 1.5;
  double QL = x[1] * L;
  double tau = (HBAR / y[1]) * ((QL * QL) / (6 + QL * QL));

  function->setParameter("L", L);
  function->setParameter("Tau", tau);
}
void estimateFickDiffusion(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData) {
  auto y = estimationData.y;
  auto x = estimationData.x;
  if (x.size() != 2 || y.size() != 2) {
    return;
  }

  function->setParameter("D", y[1] / (x[1] * x[1]));
}

IDAFunctionParameterEstimation createParameterEstimation() {

  IDAFunctionParameterEstimation parameterEstimation;
  parameterEstimation.addParameterEstimationFunction("ChudleyElliot", estimateChudleyElliot);
  parameterEstimation.addParameterEstimationFunction("HallRoss", estimateHallRoss);
  parameterEstimation.addParameterEstimationFunction("TeixeiraWater", estimateTeixeiraWater);
  parameterEstimation.addParameterEstimationFunction("FickDiffusion", estimateFickDiffusion);

  return parameterEstimation;
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

FqTemplateBrowser::FqTemplateBrowser()
    : SingleFunctionTemplateBrowser(widthFits,
                                    std::make_unique<IDAFunctionParameterEstimation>(createParameterEstimation())) {}

} // namespace MantidQt::CustomInterfaces::IDA