// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ConvolutionFunctionModel.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFunctionModel");
}

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

namespace {

bool isConvolution(const IFunction_sptr &fun) {
  return fun->name() == "Convolution";
}

bool isResolution(const IFunction_sptr &fun) {
  return fun->name() == "Resolution";
}

bool isDeltaFunction(const IFunction_sptr &fun) {
  return fun->name() == "DeltaFunction";
}

bool isProductFunction(const IFunction_sptr &fun) {
  return fun->name() == "ProductFunction";
}

bool isBackground(const IFunction_sptr &fun) {
  return static_cast<bool>(
      boost::dynamic_pointer_cast<const IBackgroundFunction>(fun));
}

} // namespace

void ConvolutionFunctionModel::setFunction(IFunction_sptr fun) {
  FunctionModel::setFunction(fun);
  findComponentPrefixes();
}

void ConvolutionFunctionModel::setModel(const std::string &background,
                                        const std::string &workspace,
                                        int workspaceIndex,
                                        const std::string &peaks,
                                        bool hasDeltaFunction) {
  auto const resolution =
      workspace.empty()
          ? "name=Resolution"
          : "name=Resolution,Workspace=\"" + workspace +
                "\",WorkspaceIndex=" + std::to_string(workspaceIndex);
  auto const model = hasDeltaFunction ? "name=DeltaFunction;" + peaks : peaks;
  auto const convolution = "composite=Convolution;" + resolution + ";" + model;
  auto const function =
      background.empty() ? convolution : background + ";(" + convolution + ")";
  setFunction(FunctionFactory::Instance().createInitialized(function));
}

void ConvolutionFunctionModel::setModel(
    const std::string &background,
    const std::vector<std::pair<std::string, int>> &resolutionWorkspaces,
    const std::string &peaks, bool hasDeltaFunction,
    const std::vector<double> &qValues, const bool isQDependent,
    bool hasTempCorrection) {

  std::string resolution, convolution, function, modifiedPeaks;
  auto fitFunction = boost::make_shared<MultiDomainFunction>();

  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    CompositeFunction_sptr domainFunction;
    auto qValue = qValues.empty() ? 0.0 : qValues[i];
    domainFunction = createInnerFunction(peaks, hasDeltaFunction, isQDependent,
                                         qValue, hasTempCorrection);
    auto workspace =
        resolutionWorkspaces.empty() ? "" : resolutionWorkspaces[i].first;
    auto resolutionFunction =
        createResolutionFunction(workspace, resolutionWorkspaces[i].second);
    domainFunction =
        createConvolutionFunction(resolutionFunction, domainFunction);
    domainFunction = addBackground(domainFunction, background);
    fitFunction->addFunction(domainFunction);
    fitFunction->setDomainIndex(i, i);
  }
  setFunction(fitFunction);
}

CompositeFunction_sptr
ConvolutionFunctionModel::addBackground(CompositeFunction_sptr domainFunction,
                                        std::string background) {
  if (background.empty())
    return domainFunction;

  auto backgroundFunction =
      FunctionFactory::Instance().createInitialized(background);
  auto functionWithBackground = boost::make_shared<CompositeFunction>();
  functionWithBackground->addFunction(backgroundFunction);
  functionWithBackground->addFunction(domainFunction);

  return functionWithBackground;
}

CompositeFunction_sptr ConvolutionFunctionModel::createInnerFunction(
    std::string peaksFunction, bool hasDeltaFunction, bool isQDependent,
    double qValue, bool hasTempCorrection) {
  auto functionSpecified = !peaksFunction.empty();
  CompositeFunction_sptr innerFunction =
      boost::make_shared<CompositeFunction>();
  if (functionSpecified) {
    auto peakFunction =
        FunctionFactory::Instance().createInitialized(peaksFunction);
    auto peakFunctionComposite =
        boost::dynamic_pointer_cast<CompositeFunction>(peakFunction);
    if (peakFunctionComposite) {
      innerFunction = peakFunctionComposite;
    } else {
      innerFunction->addFunction(peakFunction);
    }
    if (isQDependent) {
      IFunction::Attribute attr(qValue);
      innerFunction->setAttribute("Q", attr);
    }
  }
  if (hasDeltaFunction) {
    auto deltaFunction =
        FunctionFactory::Instance().createFunction("DeltaFunction");
    innerFunction->addFunction(deltaFunction);
  }
  if (hasTempCorrection && (functionSpecified || hasDeltaFunction)) {
    innerFunction = addTempCorrection(innerFunction);
  }

  return innerFunction;
}

CompositeFunction_sptr ConvolutionFunctionModel::addTempCorrection(
    CompositeFunction_sptr peaksFunction) {
  CompositeFunction_sptr productFunction =
      boost::dynamic_pointer_cast<CompositeFunction>(
          FunctionFactory::Instance().createFunction("ProductFunction"));
  auto tempFunction = createTemperatureCorrection(100.0);
  productFunction->addFunction(tempFunction);
  productFunction->addFunction(peaksFunction);
  return productFunction;
}

IFunction_sptr
ConvolutionFunctionModel::createTemperatureCorrection(double correction) {
  // create user function for the exponential correction
  // (x*temp) / 1-exp(-(x*temp))
  auto tempFunc = FunctionFactory::Instance().createFunction("UserFunction");
  // 11.606 is the conversion factor from meV to K
  std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
  IFunction::Attribute att(formula);
  tempFunc->setAttribute("Formula", att);
  tempFunc->setParameter("Temp", correction);
  tempFunc->fixParameter("Temp", true);
  return tempFunc;
}

CompositeFunction_sptr ConvolutionFunctionModel::createConvolutionFunction(
    IFunction_sptr resolutionFunction, IFunction_sptr innerFunction) {
  CompositeFunction_sptr convolution =
      boost::dynamic_pointer_cast<CompositeFunction>(
          FunctionFactory::Instance().createFunction("Convolution"));
  convolution->addFunction(resolutionFunction);

  if (innerFunction->nFunctions() > 0)
    convolution->addFunction(innerFunction);

  return convolution;
}

IFunction_sptr
ConvolutionFunctionModel::createResolutionFunction(std::string workspaceName,
                                                   int workspaceIndex) {
  std::string resolution =
      workspaceName.empty()
          ? "name=Resolution"
          : "name=Resolution,Workspace=\"" + workspaceName +
                "\",WorkspaceIndex=" + std::to_string(workspaceIndex);
  return FunctionFactory::Instance().createInitialized(resolution);
}

void ConvolutionFunctionModel::findComponentPrefixes() {
  m_backgroundPrefix.reset();
  m_convolutionPrefix.reset();
  m_deltaFunctionPrefix.reset();
  m_tempFunctionPrefix.reset();
  m_peakPrefixes.reset();
  m_resolutionWorkspace.clear();
  m_resolutionWorkspaceIndex = 0;
  auto function = getCurrentFunction();
  if (!function)
    return;
  if (function->nFunctions() == 0) {
    if (!isConvolution(function)) {
      throw std::runtime_error("Model doesn't contain a convolution.");
    }
    m_convolutionPrefix = "";
    return;
  }
  if (isConvolution(function)) {
    m_convolutionPrefix = "";
    findConvolutionPrefixes(function);
    return;
  }
  for (size_t i = 0; i < function->nFunctions(); ++i) {
    auto const fun = function->getFunction(i);
    if (isBackground(fun)) {
      if (m_backgroundPrefix) {
        throw std::runtime_error("Model cannot have more than one background.");
      }
      m_backgroundPrefix = QString("f%1.").arg(i);
    } else if (isConvolution(fun)) {
      if (m_convolutionPrefix) {
        throw std::runtime_error(
            "Model cannot have more than one convolution.");
      }
      m_convolutionPrefix = QString("f%1.").arg(i);
      findConvolutionPrefixes(fun);
    }
  }
  if (!m_convolutionPrefix) {
    throw std::runtime_error("Model doesn't contain a convolution.");
  }
}

void ConvolutionFunctionModel::findConvolutionPrefixes(
    const IFunction_sptr &fun) {
  auto const nf = fun->nFunctions();
  if (nf == 0)
    return;
  auto const resolution = fun->getFunction(0);
  if (!isResolution(resolution)) {
    throw std::runtime_error(
        "Model's resolution function must have type Resolution.");
  }
  m_resolutionWorkspace = resolution->getAttribute("Workspace").asString();
  m_resolutionWorkspaceIndex =
      resolution->getAttribute("WorkspaceIndex").asInt();
  if (nf == 1)
    return;
  auto model = fun->getFunction(1);
  QString convolutionPrefix = "f1.";
  if (isProductFunction(model)) {
    model = model->getFunction(1);
    convolutionPrefix = "f1.f1.";
    m_tempFunctionPrefix = *m_convolutionPrefix + "f1.f0.";
  }
  auto const nm = model->nFunctions();
  if (nm == 0) {
    auto const prefix = *m_convolutionPrefix + convolutionPrefix;
    if (isDeltaFunction(model)) {
      m_deltaFunctionPrefix = prefix;
    } else {
      m_peakPrefixes = QStringList(prefix);
    }
  } else {
    m_peakPrefixes = QStringList();
    for (size_t j = 0; j < model->nFunctions(); ++j) {
      auto const prefix =
          *m_convolutionPrefix + convolutionPrefix + QString("f%1.").arg(j);
      if (isDeltaFunction(model->getFunction(j))) {
        m_deltaFunctionPrefix = prefix;
      } else {
        m_peakPrefixes->append(prefix);
      }
    }
    if (m_peakPrefixes->isEmpty()) {
      m_peakPrefixes.reset();
    }
  }
  if (!m_convolutionPrefix) {
    throw std::runtime_error("Model doesn't contain a convolution.");
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
