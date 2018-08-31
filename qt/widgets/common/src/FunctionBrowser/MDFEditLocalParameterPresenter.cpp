#include "MDFEditLocalParameterPresenter.h"

#include <iterator>
#include <regex>

namespace {

using MantidQt::CustomInterfaces::QENS::EditLocalParameterDialog;
using MantidQt::CustomInterfaces::MDF::EditLocalParameterModel;

std::vector<std::string> splitOnRegex(std::string const &str,
                                      std::regex const &re) {
  std::vector<std::string> items;
  std::copy(std::sregex_token_iterator(str.begin(), str.end(), re, -1),
            std::sregex_token_iterator(), std::back_inserter(items));
  return items;
}

std::vector<std::string> splitOnRegex(std::string const &str,
                                      std::string const &re) {
  return splitOnRegex(str, std::regex(re));
}

bool toDouble(std::string const &str) {
  try {
    return std::stod(str);
  } catch (std::invalid_argument const &) {
    return 0;
  } catch (std::out_of_range const &) {
    return 0;
  }
}

std::vector<double> getParametersFromClipboardText(std::string const &text) {
  auto const valueStrings = splitOnRegex(text, "\\s|,");

  std::vector<double> values;
  values.reserve(valueStrings.size());
  std::transform(valueStrings.begin(), valueStrings.end(),
                 std::back_inserter(values), toDouble);
  return values;
}

auto getDatasetNameCreator(EditLocalParameterModel const &model) {
  auto const &names = model.getWorkspaceNames();
  auto const &indices = model.getWorkspaceIndices();
  return [&](std::size_t i) {
    return names[i] + " (" + std::to_string(indices[i]) + ")";
  };
}

void addParameterToDialog(EditLocalParameterModel const &model,
                          EditLocalParameterDialog &dialog,
                          std::string const &dataset, std::size_t index) {
  if (model.isFixed(index))
    dialog.addFixedParameter(dataset, model.getParameterValue(index));
  else if (model.isTied(index))
    dialog.addTiedParameter(dataset, model.getParameterValue(index),
                            model.getTie(index));
  else
    dialog.addFittedParameter(dataset, model.getParameterValue(index));
}

void addParametersToDialog(EditLocalParameterModel const &model,
                           EditLocalParameterDialog &dialog) {
  auto const getDatasetName = getDatasetNameCreator(model);
  for (auto i = 0u; i < model.numberOfParameters(); ++i)
    addParameterToDialog(model, dialog, getDatasetName(i), i);
}

std::string createParameterNameTitle(EditLocalParameterModel const &model) {
  return "Parameter: " + model.getParameterName();
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

EditLocalParameterPresenter::EditLocalParameterPresenter(
    EditLocalParameterModel model, QWidget *dialogParent)
    : m_model(std::move(model)), m_dialog(dialogParent) {
  m_dialog.subscribe(this);
  m_dialog.setParameterNameTitle(createParameterNameTitle(model));
  m_dialog.addLogsToMenu(m_model.getLogNames());
  addParametersToDialog(m_model, m_dialog);
}

bool EditLocalParameterPresenter::executeDialog(
    MantidWidgets::MultiDomainFunctionModel &modelToUpdate) {
  if (m_dialog.exec() == QDialog::Accepted) {
    m_model.updateFunctionModel(modelToUpdate);
    return true;
  }
  return false;
}

void EditLocalParameterPresenter::setParameters(double value) {
  m_model.setParameters(value);
  m_dialog.setParameterValues(value);
}

void EditLocalParameterPresenter::setFixed(bool fixed) {
  m_model.setFixed(fixed);
  updateDialogParameterRoles();
}

void EditLocalParameterPresenter::setTies(std::string const &tie) {
  m_model.setTies(tie);
  m_dialog.setTies(tie);
  updateDialogParameterRoles();
}

void EditLocalParameterPresenter::setParameter(double value, int index) {
  m_model.setParameter(value, static_cast<std::size_t>(index));
  m_dialog.setParameterValue(value, index);
}

void EditLocalParameterPresenter::fixParameter(bool fixed, int index) {
  m_model.fixParameter(fixed, index);
  updateDialogParameterRole(index);
}

void EditLocalParameterPresenter::setTie(std::string const &tie, int index) {
  m_model.setTie(tie, index);
  updateDialogParameterRole(index);
}

void EditLocalParameterPresenter::copyValuesToClipboard() {
  m_dialog.copyToClipboard(m_model.getDelimitedParameters("\n"));
}

void EditLocalParameterPresenter::pasteValuesFromClipboard(
    std::string const &text) {
  auto const parameterValues = getParametersFromClipboardText(text);
  auto const n = std::min(parameterValues.size(), m_model.numberOfParameters());
  for (auto i = 0u; i < n; ++i)
    setParameter(parameterValues[i], i);
}

void EditLocalParameterPresenter::setValuesToLog(std::string const &logName,
                                                 std::string const &function) {
  m_model.setValuesToLog(logName, function);
  updateDialogParameterValues();
}

void EditLocalParameterPresenter::setValueToLog(std::string const &logName,
                                                std::string const &function,
                                                int row) {
  auto const index = static_cast<std::size_t>(row);
  m_model.setValueToLog(logName, function, static_cast<std::size_t>(index));
  m_dialog.setParameterValue(m_model.getParameterValue(index), row);
}

void EditLocalParameterPresenter::updateDialogParameterValues() {
  for (auto i = 0u; i < m_model.numberOfParameters(); ++i)
    return m_dialog.setParameterValue(m_model.getParameterValue(i), i);
}

void EditLocalParameterPresenter::updateDialogParameterRoles() {
  for (auto i = 0u; i < m_model.numberOfParameters(); ++i)
    updateDialogParameterRole(i);
}

void EditLocalParameterPresenter::updateDialogParameterRole(int index) {
  if (m_model.isFixed(static_cast<std::size_t>(index)))
    m_dialog.setParameterToFixed(index);
  else if (m_model.isTied(static_cast<std::size_t>(index)))
    m_dialog.setParameterToTied(index);
  else
    m_dialog.setParameterToFitted(index);
}

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt