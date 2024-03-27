// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitData.h"
#include "FitOutput.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <boost/optional.hpp>
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL FitPlotModel {
public:
  FitPlotModel();
  ~FitPlotModel();

  Mantid::API::MatrixWorkspace_sptr getWorkspace() const;
  Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const;
  Mantid::API::MatrixWorkspace_sptr getGuessWorkspace() const;
  MantidWidgets::FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const;

  WorkspaceID getActiveWorkspaceID() const;
  WorkspaceIndex getActiveWorkspaceIndex() const;
  FitDomainIndex getActiveDomainIndex() const;
  WorkspaceID numberOfWorkspaces() const;
  std::pair<double, double> getRange() const;
  std::pair<double, double> getWorkspaceRange() const;
  std::pair<double, double> getResultRange() const;
  boost::optional<double> getFirstHWHM() const;
  boost::optional<double> getFirstPeakCentre() const;
  boost::optional<double> getFirstBackgroundLevel() const;
  double calculateHWHMMaximum(double minimum) const;
  double calculateHWHMMinimum(double maximum) const;
  bool canCalculateGuess() const;

  void setActiveIndex(WorkspaceID workspaceID);
  void setActiveSpectrum(WorkspaceIndex spectrum);

  void setFittingData(std::vector<FitData> *fittingData);
  void setFitOutput(IFitOutput *fitOutput);
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);

private:
  std::pair<double, double> getGuessRange() const;

  Mantid::API::IFunction_sptr getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  boost::optional<ResultLocationNew> getResultLocation(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  size_t numberOfSpectra(WorkspaceID workspaceID) const;

  Mantid::API::MatrixWorkspace_sptr createGuessWorkspace(const Mantid::API::MatrixWorkspace_sptr &inputWorkspace,
                                                         const Mantid::API::IFunction_const_sptr &func, double startX,
                                                         double endX) const;

  std::vector<double> computeOutput(const Mantid::API::IFunction_const_sptr &func,
                                    const std::vector<double> &dataX) const;

  Mantid::API::IAlgorithm_sptr createWorkspaceAlgorithm(std::size_t numberOfSpectra, const std::vector<double> &dataX,
                                                        const std::vector<double> &dataY) const;

  Mantid::API::MatrixWorkspace_sptr extractSpectra(const Mantid::API::MatrixWorkspace_sptr &inputWS, int startIndex,
                                                   int endIndex, double startX, double endX) const;

  Mantid::API::MatrixWorkspace_sptr appendSpectra(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                                                  const Mantid::API::MatrixWorkspace_sptr &spectraWS) const;

  Mantid::API::MatrixWorkspace_sptr cropWorkspace(const Mantid::API::MatrixWorkspace_sptr &inputWS, double startX,
                                                  double endX, int startIndex, int endIndex) const;

  void deleteWorkspace(const std::string &name) const;

  std::vector<FitData> *m_fittingData;
  IFitOutput *m_fitOutput;
  WorkspaceID m_activeWorkspaceID;
  WorkspaceIndex m_activeWorkspaceIndex;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
