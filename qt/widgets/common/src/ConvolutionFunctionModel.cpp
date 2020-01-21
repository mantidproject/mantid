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
#include <iostream>

namespace {
Mantid::Kernel::Logger g_log("ConvolutionFunctionModel");
}

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

namespace {

bool isConvolution(const IFunction *fun) {
  return fun->name() == "Convolution";
}

bool isResolution(const IFunction *fun) { return fun->name() == "Resolution"; }

bool isDeltaFunction(const IFunction *fun) {
  return fun->name() == "DeltaFunction";
}

bool isTempFunction(const IFunction *fun) {
  return fun->name() == "UserFunction";
}

bool isBackground(const IFunction *fun) {
  return static_cast<bool>(dynamic_cast<const IBackgroundFunction *>(fun));
}

bool isPeakFunction(const IFunction *fun) {
  if (dynamic_cast<const CompositeFunction *>(fun)) {
    return false;
  }
  return true;
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
  auto fitFunction = boost::make_shared<MultiDomainFunction>();
  auto const nf = m_numberDomains > 0 ? static_cast<int>(m_numberDomains) : 1;
  for (int i = 0; i < nf; ++i) {
    CompositeFunction_sptr domainFunction;
    auto qValue = qValues.empty() ? 0.0 : qValues[i];
    auto innerFunction = createInnerFunction(
        peaks, hasDeltaFunction, isQDependent, qValue, hasTempCorrection);
    auto workspace =
        resolutionWorkspaces.empty() ? "" : resolutionWorkspaces[i].first;
    auto workspaceIndex =
        resolutionWorkspaces.empty() ? 0 : resolutionWorkspaces[i].second;
    auto resolutionFunction =
        createResolutionFunction(workspace, workspaceIndex);
    auto convolutionFunction =
        createConvolutionFunction(resolutionFunction, innerFunction);
    domainFunction = addBackground(convolutionFunction, background);

    fitFunction->addFunction(domainFunction);
    fitFunction->setDomainIndex(i, i);
  }
  // The two clones here are needed as the clone value of IFunction goes through
  // a string serialisation and deserialisation. This can lead to the function
  // structure subtly changeing. For exmaple composite functions of only one
  // member are removed and uneeded brackets are removed from user defined
  // functions. As function cloning is used later on in the workflow it seems
  // safer to clone twice here to get the function in it's final state early on
  // rather than have it change during the workflow.
  setFunction(fitFunction->clone()->clone());
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

  if (hasTempCorrection) {
    innerFunction = addTempCorrection(innerFunction);
  }

  if (hasDeltaFunction) {
    auto deltaFunction =
        FunctionFactory::Instance().createFunction("DeltaFunction");
    if (!hasTempCorrection) {
      innerFunction->addFunction(deltaFunction);
    } else {
      CompositeFunction_sptr innerFunctionNew =
          boost::make_shared<CompositeFunction>();
      innerFunctionNew->addFunction(deltaFunction);
      innerFunctionNew->addFunction(innerFunction);
      return innerFunctionNew;
    }
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
  tempFunc->fixParameter("Temp", false);
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
  m_peakPrefixes = QStringList();
  m_resolutionWorkspace.clear();
  m_resolutionWorkspaceIndex = 0;

  auto function = getCurrentFunction();
  if (!function)
    return;
  iterateThroughFunction(function.get(), QString());

  if (m_peakPrefixes->isEmpty()) {
    m_peakPrefixes.reset();
  }
  if (!m_convolutionPrefix) {
    throw std::runtime_error("Model doesn't contain a convolution.");
  }
}

void ConvolutionFunctionModel::iterateThroughFunction(IFunction *func,
                                                      const QString &prefix) {
  auto numberOfSubFunction = func->nFunctions();

  setPrefix(func, prefix);
  if (numberOfSubFunction == 0) {
    return;
  }
  for (size_t k = 0; k < numberOfSubFunction; ++k) {
    iterateThroughFunction(func->getFunction(k).get(),
                           prefix + QString("f%1.").arg(k));
  }
}

void ConvolutionFunctionModel::setPrefix(IFunction *func,
                                         const QString &prefix) {
  if (isBackground(func)) {
    if (m_backgroundPrefix) {
      throw std::runtime_error("Model cannot have more than one background.");
    }
    m_backgroundPrefix = prefix;
  } else if (isConvolution(func)) {
    if (func->nFunctions() != 0 &&
        func->getFunction(0)->name() != "Resolution") {
      throw std::runtime_error(
          "Model's resolution function must have type Resolution.");
    } else if (func->nFunctions() == 0) {
      m_resolutionWorkspace = "";
      m_resolutionWorkspaceIndex = 0;
    }
    m_convolutionPrefix = prefix;
  } else if (isDeltaFunction(func)) {
    m_deltaFunctionPrefix = prefix;
  } else if (isTempFunction(func)) {
    m_tempFunctionPrefix = prefix;
  } else if (isResolution(func)) {
    m_resolutionWorkspace = func->getAttribute("Workspace").asString();
    m_resolutionWorkspaceIndex = func->getAttribute("WorkspaceIndex").asInt();
  } else if (isPeakFunction(func)) {
    m_peakPrefixes->append(prefix);
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
