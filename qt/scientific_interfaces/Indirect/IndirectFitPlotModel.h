// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTMODEL_H_

#include "IndirectFittingModel.h"

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitPlotModel {
public:
  IndirectFitPlotModel(IndirectFittingModel *fittingModel);
  ~IndirectFitPlotModel();

  Mantid::API::MatrixWorkspace_sptr getWorkspace() const;
  Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const;
  Mantid::API::MatrixWorkspace_sptr getGuessWorkspace() const;
  Spectra getSpectra() const;

  Mantid::API::MatrixWorkspace_sptr
  appendGuessToInput(Mantid::API::MatrixWorkspace_sptr guessWorkspace) const;

  std::size_t getActiveDataIndex() const;
  std::size_t getActiveSpectrum() const;
  std::size_t numberOfWorkspaces() const;
  std::string getFitDataName(std::size_t index) const;
  std::string getFitDataName() const;
  std::string getLastFitDataName() const;
  std::pair<double, double> getRange() const;
  std::pair<double, double> getWorkspaceRange() const;
  std::pair<double, double> getResultRange() const;
  boost::optional<double> getFirstHWHM() const;
  boost::optional<double> getFirstPeakCentre() const;
  boost::optional<double> getFirstBackgroundLevel() const;
  double calculateHWHMMaximum(double minimum) const;
  double calculateHWHMMinimum(double maximum) const;
  bool canCalculateGuess() const;

  void setActiveIndex(std::size_t index);
  void setActiveSpectrum(std::size_t spectrum);
  void setStartX(double startX);
  void setEndX(double endX);
  void setFWHM(double fwhm);
  void setBackground(double background);

  void deleteExternalGuessWorkspace();

private:
  Mantid::API::MatrixWorkspace_sptr
  createInputAndGuessWorkspace(Mantid::API::MatrixWorkspace_sptr inputWS,
                               Mantid::API::MatrixWorkspace_sptr guessWorkspace,
                               int spectrum, double startX, double endX) const;

  Mantid::API::MatrixWorkspace_sptr
  createGuessWorkspace(Mantid::API::MatrixWorkspace_sptr inputWorkspace,
                       Mantid::API::IFunction_const_sptr func,
                       int workspaceIndex, double startX, double endX) const;

  std::vector<double> computeOutput(Mantid::API::IFunction_const_sptr func,
                                    const std::vector<double> &dataX) const;

  Mantid::API::IAlgorithm_sptr
  createWorkspaceAlgorithm(std::size_t numberOfSpectra,
                           const std::vector<double> &dataX,
                           const std::vector<double> &dataY) const;

  Mantid::API::MatrixWorkspace_sptr
  extractSpectra(Mantid::API::MatrixWorkspace_sptr inputWS, int startIndex,
                 int endIndex, double startX, double endX) const;

  Mantid::API::MatrixWorkspace_sptr
  appendSpectra(Mantid::API::MatrixWorkspace_sptr inputWS,
                Mantid::API::MatrixWorkspace_sptr spectraWS) const;

  Mantid::API::MatrixWorkspace_sptr
  cropWorkspace(Mantid::API::MatrixWorkspace_sptr inputWS, double startX,
                double endX, int startIndex, int endIndex) const;

  void deleteWorkspace(const std::string &name) const;

  IndirectFittingModel *m_fittingModel;
  std::size_t m_activeIndex;
  std::size_t m_activeSpectrum;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTMODEL_H_ */
