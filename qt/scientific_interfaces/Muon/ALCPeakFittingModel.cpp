// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr extractSpectrum(MatrixWorkspace_sptr inputWorkspace,
                                     const int workspaceIndex) {
  auto extracter = AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extracter->setChild(true);
  extracter->setProperty("InputWorkspace", inputWorkspace);
  extracter->setProperty("WorkspaceIndex", workspaceIndex);
  extracter->setPropertyValue("OutputWorkspace", "__NotUsed__");
  extracter->execute();
  MatrixWorkspace_sptr output = extracter->getProperty("OutputWorkspace");
  return output;
}

MatrixWorkspace_sptr evaluateFunction(IFunction_const_sptr function,
                                      MatrixWorkspace_sptr inputWorkspace) {
  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->setChild(true);
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
  m_data = newData;
  emit dataChanged();
}

MatrixWorkspace_sptr ALCPeakFittingModel::exportWorkspace() {
  if (m_data && m_data->getNumberHistograms() > 2) {

    return boost::const_pointer_cast<MatrixWorkspace>(m_data);

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
  m_fittedPeaks = fittedPeaks;
  emit fittedPeaksChanged();
}

void ALCPeakFittingModel::fitPeaks(IFunction_const_sptr peaks) {
  IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
  fit->setChild(true);
  fit->setProperty("Function", peaks->asString());
  fit->setProperty("InputWorkspace",
                   boost::const_pointer_cast<MatrixWorkspace>(m_data));
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
  const auto inputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, xValues.size(),
                                          xValues.size()));
  inputWorkspace->mutableX(0) = xValues;
  return extractSpectrum(evaluateFunction(function, inputWorkspace), 1);
}

} // namespace CustomInterfaces
} // namespace MantidQt
