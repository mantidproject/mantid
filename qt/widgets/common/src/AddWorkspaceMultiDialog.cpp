// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AddWorkspaceMultiDialog.h"
#include "MantidAPI/AlgorithmManager.h"
#include <MantidAPI/AlgorithmProperties.h>
#include <MantidAPI/AlgorithmRuntimeProps.h>
#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>
#include <QFileInfo>
#include <utility>

namespace {
using namespace Mantid::API;

MantidQt::API::IConfiguredAlgorithm_sptr configureLoadAlgorithm(const QString &filename,
                                                                const AlgorithmRuntimeProps &loadProps) {
  QFileInfo const fileinfo(filename);
  auto loader = AlgorithmManager::Instance().createUnmanaged("Load");
  loader->initialize();
  loader->setProperty("Filename", filename.toStdString());
  loader->setProperty("OutputWorkspace", fileinfo.baseName().toStdString());
  loader->updatePropertyValues(loadProps);
  auto properties = std::make_unique<AlgorithmRuntimeProps>();

  return std::make_shared<MantidQt::API::ConfiguredAlgorithm>(loader, std::move(properties));
}
} // namespace

namespace MantidQt::MantidWidgets {

AddWorkspaceMultiDialog::AddWorkspaceMultiDialog(QWidget *parent)
    : QDialog(parent), m_algRunner(std::make_unique<API::QtJobRunner>()), m_loadProperties() {
  m_uiForm.setupUi(this);
  m_algRunner->subscribe(this);
  connect(m_uiForm.pbSelAll, SIGNAL(clicked()), this, SLOT(selectAllSpectra()));
  connect(m_uiForm.pbResetDefault, SIGNAL(clicked()), this, SLOT(updateSelectedSpectra()));
  connect(m_uiForm.dsInputFiles, SIGNAL(filesFound()), this, SLOT(handleFilesFound()));
  connect(m_uiForm.pbUnify, SIGNAL(clicked()), this, SLOT(unifyRange()));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SLOT(emitAddData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SLOT(close()));
}
std::string AddWorkspaceMultiDialog::workspaceName() const {
  throw std::logic_error("This method is not implemented in AddWorkspaceMultiDialog class and shouldn't be called");
}

void AddWorkspaceMultiDialog::setup() { m_uiForm.tbWorkspace->setupTable(); }

StringPairVec AddWorkspaceMultiDialog::selectedNameIndexPairs() const {

  return m_uiForm.tbWorkspace->retrieveSelectedNameIndexPairs();
}

bool AddWorkspaceMultiDialog::isEmpty() const {
  return ((m_uiForm.tbWorkspace->rowCount() == 0) || (m_uiForm.tbWorkspace->selectedItems().isEmpty()));
}

void AddWorkspaceMultiDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.tbWorkspace->setWSSuffixes(suffices);
}

void AddWorkspaceMultiDialog::setFBSuffices(const QStringList &suffices) {
  return m_uiForm.dsInputFiles->setFileExtensions(suffices);
}

/**
 * Set an extra property on the load algorithm  of the workspace multi dialog
 * before execution
 *
 * @param propName :: The name of the Load algorithm property to be set
 * @param enable :: The value of the Load algorithm property to be set
 */

void AddWorkspaceMultiDialog::setLoadProperty(const std::string &propName, bool enable) {
  Mantid::API::AlgorithmProperties::update(propName, enable, m_loadProperties);
}

void AddWorkspaceMultiDialog::selectAllSpectra() { return m_uiForm.tbWorkspace->selectAll(); }

void AddWorkspaceMultiDialog::emitAddData() { emit addData(this); }

void AddWorkspaceMultiDialog::updateSelectedSpectra() { return m_uiForm.tbWorkspace->resetIndexRangeToDefault(); }

void AddWorkspaceMultiDialog::unifyRange() { return m_uiForm.tbWorkspace->unifyRange(); }

void AddWorkspaceMultiDialog::handleFilesFound() {
  updateAddButtonState(false);
  auto fileNames = m_uiForm.dsInputFiles->getFilenames();
  std::deque<API::IConfiguredAlgorithm_sptr> loadQueue;
  std::transform(fileNames.begin(), fileNames.end(), std::back_inserter(loadQueue),
                 [&](auto const &fileName) { return configureLoadAlgorithm(fileName, m_loadProperties); });
  m_algRunner->setAlgorithmQueue(std::move(loadQueue));
  m_algRunner->executeAlgorithmQueue();
}

void AddWorkspaceMultiDialog::notifyBatchComplete(bool error) {
  UNUSED_ARG(error);
  updateAddButtonState(true);
}

void AddWorkspaceMultiDialog::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr &algorithm,
                                                   std::string const &message) {
  UNUSED_ARG(algorithm);
  updateAddButtonState(true);
  QMessageBox::warning(this, "Loading error", message.c_str());
}

void AddWorkspaceMultiDialog::updateAddButtonState(bool enabled) const {
  enabled ? m_uiForm.pbAdd->setText("Add Data") : m_uiForm.pbAdd->setText("Loading");
  m_uiForm.pbAdd->setEnabled(enabled);
}
} // namespace MantidQt::MantidWidgets
