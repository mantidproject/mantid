// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "PlotFitAnalysisPaneModel.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::API;
namespace MantidQt {
namespace CustomInterfaces {

IFunction_sptr
PlotFitAnalysisPaneModel::doFit(const std::string &wsName,
                                const std::pair<double, double> &range,
                                IFunction_sptr func) {

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
  alg->initialize();
  alg->setProperty("Function", func);
  alg->setProperty("InputWorkspace", wsName);
  alg->setProperty("Output", wsName + "_fits");
  alg->setProperty("StartX", range.first);
  alg->setProperty("EndX", range.second);
  alg->execute();
  return alg->getProperty("Function");
}

} // namespace CustomInterfaces
} // namespace MantidQt
