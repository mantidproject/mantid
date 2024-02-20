// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionTemplatePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FunctionTemplatePresenter::FunctionTemplatePresenter(FunctionTemplateBrowser *view,
                                                     std::unique_ptr<MantidQt::MantidWidgets::IFunctionModel> model)
    : m_view(view), m_model(std::move(model)) {}

void FunctionTemplatePresenter::init() {}

void FunctionTemplatePresenter::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  (void)functionInitialisationStrings;
}

void FunctionTemplatePresenter::setFitType(std::string const &name) { (void)name; }

void FunctionTemplatePresenter::updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) { (void)table; }

void FunctionTemplatePresenter::setNumberOfExponentials(int nExponentials) { (void)nExponentials; }

void FunctionTemplatePresenter::setStretchExponential(bool on) { (void)on; }

void FunctionTemplatePresenter::setBackground(std::string const &name) { (void)name; }

void FunctionTemplatePresenter::tieIntensities(bool on) { (void)on; }

bool FunctionTemplatePresenter::canTieIntensities() const { return true; }

void FunctionTemplatePresenter::setSubType(std::size_t subTypeIndex, int typeIndex) {
  (void)subTypeIndex;
  (void)typeIndex;
}

void FunctionTemplatePresenter::setDeltaFunction(bool on) { (void)on; }

void FunctionTemplatePresenter::setTempCorrection(bool on) { (void)on; }

void FunctionTemplatePresenter::setBackgroundA0(double value) { (void)value; }

void FunctionTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  (void)fitResolutions;
}

void FunctionTemplatePresenter::setQValues(const std::vector<double> &qValues) { (void)qValues; }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt