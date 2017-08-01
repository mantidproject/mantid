// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidQtCustomInterfaces/DynamicPDF/DPDFInputDataControl.h"
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/AnalysisDataService.h"
// 3rd party library headers
// System #includes

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/*              **********************
 *              **  Public Members  **
 *              **********************/

/**
 * @brief Constructor.
 */
InputDataControl::InputDataControl()
    : m_workspace(), m_selectedWorkspaceIndex{0}, m_domain() {
  this->observePreDelete(true); // Subscribe to notifications
}

/**
 * @brief default destructor.
 */
InputDataControl::~InputDataControl() {
  m_workspace.reset();
  this->observePreDelete(false); // Cancel subscription to notifications
}

/**
 * @brief report the energy domain with non-zero signal
 */
std::vector<double> InputDataControl::selectedDataX() {
  auto first = m_domain.at(m_selectedWorkspaceIndex).first;
  auto second = m_domain.at(m_selectedWorkspaceIndex).second;
  auto X = m_workspace->dataX(m_selectedWorkspaceIndex);
  // crop the zero signal
  std::vector<double> x(X.begin() + first, X.begin() + second);
  return x;
}

/*
 * @brief report the first and last values of Q with non-zero signal
 * for the current selected slice
 */
std::pair<double, double> InputDataControl::getCurrentRange() {
  auto domain = m_domain.at(m_selectedWorkspaceIndex);
  auto X = m_workspace->dataX(m_selectedWorkspaceIndex);
  auto second = domain.second;
  if (m_workspace->isHistogramData()) {
    second -= 1;
  }
  return std::pair<double, double>(X.at(domain.first), X.at(second));
}

/**
 * @brief report the non-zero signal
 */
std::vector<double> InputDataControl::selectedDataY() {
  auto first = m_domain.at(m_selectedWorkspaceIndex).first;
  auto second = m_domain.at(m_selectedWorkspaceIndex).second;
  if (m_workspace->isHistogramData()) {
    second -= 1;
  }
  auto Y = m_workspace->dataY(m_selectedWorkspaceIndex);
  std::vector<double> y(Y.begin() + first,
                        Y.begin() + second); // crop the zero signal
  return y;
}

/**
 * @brief report the error for the non-zero signal
 */
std::vector<double> InputDataControl::selectedDataE() {
  auto first = m_domain.at(m_selectedWorkspaceIndex).first;
  auto second = m_domain.at(m_selectedWorkspaceIndex).second;
  if (m_workspace->isHistogramData()) {
    second -= 1;
  }
  auto E = m_workspace->dataE(m_selectedWorkspaceIndex);
  std::vector<double> e(E.begin() + first,
                        E.begin() + second); // crop the zero signal
  return e;
}

/**
 * @brief report the energy for the slice currently selected
 */
double InputDataControl::getSelectedEnergy() {
  return m_workspace->getAxis(1)->getValue(m_selectedWorkspaceIndex);
}

/**
 * @brief report the name of the workspace containing the slices
 * @exception attribute m_workspace has not yet been set
 * @return name of the workspace containing the slices
 */
std::string InputDataControl::getWorkspaceName() {
  if (!m_workspace) {
    throw std::runtime_error("InpuDataControl has not set m_workspace!");
  }
  return m_workspace->getName();
}

/**
 * @brief report the workspace index of the slice selected
 * @exception attribute m_workspace has not yet been set
 * @return the workspace index of the slice selected
 */
size_t InputDataControl::getWorkspaceIndex() {
  if (!m_workspace) {
    throw std::runtime_error("InpuDataControl has not set m_workspace!");
  }
  return m_selectedWorkspaceIndex;
}

/**
 * @brief Query if user selected a slice for fitting
 */
bool InputDataControl::isSliceSelectedForFitting() {
  if (m_workspace) {
    return true;
  }
  return false;
}

/*              *************************
 *              **  Protected Members  **
 *              *************************/

/**
 * @brief Actions when slices workspace is deleted
 */
void InputDataControl::preDeleteHandle(
    const std::string &workspaceName,
    const boost::shared_ptr<Mantid::API::Workspace> workspace) {
  UNUSED_ARG(workspaceName);
  Mantid::API::MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace);
  if (!ws || (ws != m_workspace)) {
    return;
  }
  m_workspace.reset();
  m_selectedWorkspaceIndex = 0;
  m_domain.clear();
}

/*              ********************
 *              **  Public Slots  **
 *              ********************/

/**
 * @brief Initialize the domain and emit appropriate signal
 * @param workspaceName
 */
void InputDataControl::updateWorkspace(const QString &workspaceName) {
  m_workspace = Mantid::API::AnalysisDataService::Instance()
                    .retrieveWS<Mantid::API::MatrixWorkspace>(
                        workspaceName.toStdString());
  m_domain.resize(m_workspace->getNumberHistograms());
  emit signalWorkspaceUpdated();
}

/**
 * @brief Update attributes when a new workspace index is selected
 * @param workspaceIndex the newly selected workspace index
 * @sa emits signalSliceForFittingUpdated
 */
void InputDataControl::updateSliceForFitting(const size_t &workspaceIndex) {
  m_selectedWorkspaceIndex = workspaceIndex;
  this->updateDomain();
  emit signalSliceForFittingUpdated();
}

/*              ***************
 *              **  Private  **
 *              ***************/

/*
 * @brief find the energy-range with non-zero signal
 * for current workspace index
 */
void InputDataControl::updateDomain() {
  auto y = m_workspace->dataY(m_selectedWorkspaceIndex);
  // find first index with non-zero signal
  auto it =
      std::find_if(y.begin(), y.end(), [](const double &s) { return s > 0.0; });
  int first = static_cast<int>(std::distance(y.begin(), it));
  // find first index with zero signal after the non-zero signal range
  it = std::find_if(it, y.end(), [](const double &s) { return s == 0.0; });
  int second = static_cast<int>(std::distance(y.begin(), it));
  m_domain.at(m_selectedWorkspaceIndex) = std::pair<int, int>(first, second);
}
}
}
}
