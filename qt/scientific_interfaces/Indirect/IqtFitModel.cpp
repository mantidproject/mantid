#include "IqtFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"

using namespace Mantid::API;

namespace {
MatrixWorkspace_sptr replaceInfinityAndNaN(MatrixWorkspace_sptr inputWS) {
  auto replaceAlg = AlgorithmManager::Instance().create("ReplaceSpecialValues");
  replaceAlg->setChild(true);
  replaceAlg->initialize();
  replaceAlg->setProperty("InputWorkspace", inputWS);
  replaceAlg->setProperty("NaNValue", 0.0);
  replaceAlg->setProperty("InfinityError", 0.0);
  replaceAlg->setProperty("OutputWorkspace", inputWS->getName() + "_nospecial");
  replaceAlg->execute();
  return replaceAlg->getProperty("OutputWorkspace");
}

IFunction_sptr getIndexOfFirstInCategory(CompositeFunction_sptr function,
                                         const std::string &category) {
  for (auto i = 0u; i < function->nFunctions(); ++i) {
    if (function->getFunction(i)->category() == category)
      return function->getFunction(i);
  }
  return nullptr;
}

IFunction_sptr getFirstInCategory(IFunction_sptr function,
                                  const std::string &category) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return getFirstInCategory(composite, category);
  else if (function->category() == category)
    return function;
  return nullptr;
}

std::vector<std::string> getParameters(IFunction_sptr function,
                                       const std::string &shortParameterName) {
  std::vector<std::string> parameters;

  for (const auto &longName : function->getParameterNames()) {
    if (boost::algorithm::ends_with(longName, shortParameterName))
      parameters.push_back(longName);
  }
  return parameters;
}
}; // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

void IqtFitModel::addWorkspace(MatrixWorkspace_sptr workspace,
                                       const Spectra &spectra) {
  IndirectFittingModel::addWorkspace(replaceInfinityAndNaN(workspace), spectra);
}

IAlgorithm_sptr IqtFitModel::sequentialFitAlgorithm() const {
  return AlgorithmManager::Instance().create("IqtFitSequential");
}

IAlgorithm_sptr IqtFitModel::simultaneousFitAlgorithm() const {
  return AlgorithmManager::Instance().create("IqtFitSimultaneous");
}

std::string IqtFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiIqtFit_" + m_fitType;
  return createOutputName("%1%_IqtFit_" + m_fitType + "_s%2%", "_to_", 0);
}

std::string IqtFitModel::simultaneousFitOutputName() const {
  if (isMultiFit())
    return "MultiSimultaneousIqtFit_" + m_fitType;
  return createOutputName("%1%_IqtFit_mult" + m_fitType + "_s%2%", "_to_", 0);
}

void IqtFitModel::setFitTypeString(const std::string &fitType) {
  m_fitType = fitType;
}

std::string IqtFitModel::createIntensityTie() const {
  const auto intensityParameters =
      getParameters(getFittingFunction(), "Height");
  const auto backgroundParameters = getParameters(getFittingFunction(), "A0");

  if (intensityParameters.size() + backgroundParameters.size() < 2 ||
      intensityParameters.empty())
    return "";

  std::stringstream tieString;

  for (const auto &parameter : backgroundParameters)
    tieString << "-" << parameter;

  for (auto i = 1u; i < intensityParameters.size(); ++i)
    tieString << "-" << intensityParameters[i];

  return intensityParameters[0] + "=" + tieString.str();
}

std::unordered_map<std::string, ParameterValue>
IqtFitModel::getDefaultParameters(std::size_t index) const {
  std::unordered_map<std::string, ParameterValue> parameters;
  auto background = getFirstInCategory(getFittingFunction(), "Background");

  double height = 1.0;
  if (background && background->hasParameter("A0"))
    height -= background->getParameter("A0");
  parameters["Height"] = height;

  auto inputWs = getWorkspace(index);
  double tau = 0;

  if (inputWs) {
    auto x = inputWs->x(0);
    auto y = inputWs->y(0);

    if (x.size() > 4) {
      tau = -x[4] / log(y[4]);
    }
  }

  parameters["Lifetime"] = tau;
  parameters["Stretching"] = 1.0;
  parameters["A0"] = 0.0;
  return parameters;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
