// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IINDIRECTFITTINGMODEL_H_

#include "IndirectFitData.h"
#include "IndirectFitOutput.h"

#include "DllConfig.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };

struct PrivateFittingData;

class MANTIDQT_INDIRECT_DLL IIndirectFittingModel {
public:
  virtual Mantid::API::MatrixWorkspace_sptr
  getWorkspace(std::size_t index) const = 0;
  virtual Spectra getSpectra(std::size_t index) const = 0;
  virtual std::pair<double, double>
  getFittingRange(std::size_t dataIndex, std::size_t spectrum) const = 0;
  virtual std::string getExcludeRegion(std::size_t dataIndex,
                                       std::size_t index) const = 0;
  virtual std::string createDisplayName(const std::string &formatString,
                                        const std::string &rangeDelimiter,
                                        std::size_t dataIndex) const = 0;
  virtual std::string createOutputName(const std::string &formatString,
                                       const std::string &rangeDelimiter,
                                       std::size_t dataIndex) const = 0;
  virtual bool isMultiFit() const = 0;
  virtual bool isPreviouslyFit(std::size_t dataIndex,
                               std::size_t spectrum) const = 0;
  virtual bool hasZeroSpectra(std::size_t dataIndex) const = 0;
  virtual boost::optional<std::string> isInvalidFunction() const = 0;
  virtual std::size_t numberOfWorkspaces() const = 0;
  virtual std::size_t getNumberOfSpectra(std::size_t index) const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::IFunction_sptr getFittingFunction() const = 0;

  virtual std::vector<std::string> getSpectrumDependentAttributes() const = 0;

  virtual void setFittingData(PrivateFittingData &&fittingData) = 0;
  virtual void setSpectra(const std::string &spectra,
                          std::size_t dataIndex) = 0;
  virtual void setSpectra(Spectra &&spectra, std::size_t dataIndex) = 0;
  virtual void setSpectra(const Spectra &spectra, std::size_t dataIndex) = 0;
  virtual void setStartX(double startX, std::size_t dataIndex,
                         std::size_t spectrum) = 0;
  virtual void setEndX(double endX, std::size_t dataIndex,
                       std::size_t spectrum) = 0;
  virtual void setExcludeRegion(const std::string &exclude,
                                std::size_t dataIndex,
                                std::size_t spectrum) = 0;

  virtual void addWorkspace(const std::string &workspaceName) = 0;
  virtual void addWorkspace(const std::string &workspaceName,
                            const std::string &spectra) = 0;
  virtual void addWorkspace(const std::string &workspaceName,
                            const Spectra &spectra) = 0;
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra) = 0;
  virtual void removeWorkspace(std::size_t index) = 0;
  virtual PrivateFittingData clearWorkspaces() = 0;
  virtual void setFittingMode(FittingMode mode) = 0;
  virtual void setFitFunction(Mantid::API::IFunction_sptr function) = 0;
  virtual void setDefaultParameterValue(const std::string &name, double value,
                                        std::size_t dataIndex) = 0;
  virtual void addSingleFitOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                                  std::size_t index) = 0;
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) = 0;

  virtual FittingMode getFittingMode() const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  getParameterValues(std::size_t dataIndex, std::size_t spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  getFitParameters(std::size_t dataIndex, std::size_t spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t dataIndex) const = 0;
  virtual boost::optional<ResultLocation>
  getResultLocation(std::size_t dataIndex, std::size_t spectrum) const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultGroup() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const = 0;
  virtual Mantid::API::IAlgorithm_sptr
  getSingleFit(std::size_t dataIndex, std::size_t spectrum) const = 0;

  virtual void saveResult() const = 0;
  virtual void
  cleanFailedRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm) = 0;
  virtual void
  cleanFailedSingleRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm,
                       std::size_t index) = 0;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
