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

bool isConvolution(const IFunction_sptr& fun) {
  return fun->name() == "Convolution";
}

bool isResolution(const IFunction_sptr& fun) {
  return fun->name() == "Resolution";
}

bool isDeltaFunction(const IFunction_sptr& fun) {
  return fun->name() == "DeltaFunction";
}

bool isBackground(const IFunction_sptr& fun) {
  return static_cast<bool>(boost::dynamic_pointer_cast<const IBackgroundFunction>(fun));
}

}

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
  auto const function = background.empty() ? convolution : background + ";(" + convolution + ")";
  setFunction(FunctionFactory::Instance().createInitialized(function));
}

void ConvolutionFunctionModel::findComponentPrefixes() {
  m_backgroundPrefix.reset();
  m_convolutionPrefix.reset();
  m_deltaFunctionPrefix.reset();
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
        throw std::runtime_error("Model cannot have more than one convolution.");
      }
      m_convolutionPrefix = QString("f%1.").arg(i);
      findConvolutionPrefixes(fun);
    }
  }
  if (!m_convolutionPrefix) {
    throw std::runtime_error("Model doesn't contain a convolution.");
  }
}

void ConvolutionFunctionModel::findConvolutionPrefixes(const IFunction_sptr &fun) {
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
  auto const model = fun->getFunction(1);
  auto const nm = model->nFunctions();
  if (nm == 0) {
    auto const prefix = *m_convolutionPrefix + "f1.";
    if (isDeltaFunction(model)) {
      m_deltaFunctionPrefix = prefix;
    } else {
      m_peakPrefixes = QStringList(prefix);
    }
  } else {
    m_peakPrefixes = QStringList();
    for (size_t j = 0; j < model->nFunctions(); ++j) {
      auto const prefix = *m_convolutionPrefix + QString("f1.f%1.").arg(j);
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
