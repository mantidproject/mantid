// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCPeakFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <Poco/ActiveResult.h>
#include <QApplication>
#include <utility>

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr extractSpectrum(const MatrixWorkspace_sptr &inputWorkspace,
                                     const int workspaceIndex) {
  auto extracter = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extracter->setAlwaysStoreInADS(false);
  extracter->setProperty("InputWorkspace", inputWorkspace);
  extracter->setProperty("WorkspaceIndex", workspaceIndex);
  extracter->setPropertyValue("OutputWorkspace", "__NotUsed__");
  extracter->execute();
  MatrixWorkspace_sptr output = extracter->getProperty("OutputWorkspace");
  return output;
}

MatrixWorkspace_sptr
evaluateFunction(const IFunction_const_sptr &function,
                 const MatrixWorkspace_sptr &inputWorkspace) {
  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setProperty("Function", function->asString());
  fit->setProperty("InputWorkspace", inputWorkspace);
  fit->setProperty("MaxIterations", 0);
  fit->setProperty("CreateOutput", true);
  fit->execute();
  MatrixWorkspace_sptr output = fit->getProperty("OutputWorkspace");
  return output;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

void ALCPeakFittingModel::setData(MatrixWorkspace_sptr newData) {
  m_data = std::move(newData);
  emit dataChanged();
}

MatrixWorkspace_sptr ALCPeakFittingModel::exportWorkspace() {
  if (m_data && m_data->getNumberHistograms() > 2) {

    return std::const_pointer_cast<MatrixWorkspace>(m_data);

  } else {

    return MatrixWorkspace_sptr();
  }
}

ITableWorkspace_sptr ALCPeakFittingModel::exportFittedPeaks() {
  if (m_parameterTable) {

    return m_parameterTable;

  } else {

    return ITableWorkspace_sptr();
  }
}

void ALCPeakFittingModel::setFittedPeaks(IFunction_const_sptr fittedPeaks) {
  m_fittedPeaks = std::move(fittedPeaks);
  emit fittedPeaksChanged();
}

void ALCPeakFittingModel::fitPeaks(IFunction_const_sptr peaks) {
  IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
  fit->setAlwaysStoreInADS(false);
  fit->setProperty("Function", peaks->asString());
  fit->setProperty("InputWorkspace",
                   std::const_pointer_cast<MatrixWorkspace>(m_data));
  fit->setProperty("CreateOutput", true);
  fit->setProperty("OutputCompositeMembers", true);

  // Execute async so we can show progress bar
  Poco::ActiveResult<bool> result(fit->executeAsync());
  while (!result.available()) {
    QCoreApplication::processEvents();
  }
  if (!result.error().empty()) {
    QString msg =
        "Fit algorithm failed.\n\n" + QString(result.error().c_str()) + "\n";
    emit errorInModel(msg);
  }

  m_data = fit->getProperty("OutputWorkspace");
  m_parameterTable = fit->getProperty("OutputParameters");
  setFittedPeaks(static_cast<IFunction_sptr>(fit->getProperty("Function")));
}

MatrixWorkspace_sptr
ALCPeakFittingModel::guessData(IFunction_const_sptr function,
                               const std::vector<double> &xValues) {
  const auto inputWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, xValues.size(),
                                          xValues.size()));
  inputWorkspace->mutableX(0) = xValues;
  return extractSpectrum(evaluateFunction(function, inputWorkspace), 1);
}

} // namespace CustomInterfaces
} // namespace MantidQt
