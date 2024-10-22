// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MomentsModel.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <MantidQtWidgets/Common/ConfiguredAlgorithm.h>
#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

MomentsModel::MomentsModel() : m_inputWorkspace(), m_outputWorkspaceName(), m_eMin(), m_eMax(), m_scale(false) {}

API::IConfiguredAlgorithm_sptr MomentsModel::setupMomentsAlgorithm() const {
  auto momentsAlg = AlgorithmManager::Instance().create("SofQWMoments", -1);
  momentsAlg->initialize();
  momentsAlg->setAlwaysStoreInADS(false);
  auto properties = std::make_unique<AlgorithmRuntimeProps>();
  properties->setProperty("InputWorkspace", m_inputWorkspace);
  properties->setProperty("EnergyMin", m_eMin);
  properties->setProperty("EnergyMax", m_eMax);
  properties->setProperty("Scale", m_scale ? m_scaleValue : 1.0);

  MantidQt::API::IConfiguredAlgorithm_sptr confAlg =
      std::make_shared<API::ConfiguredAlgorithm>(momentsAlg, std::move(properties));
  return confAlg;
}

void MomentsModel::setInputWorkspace(const std::string &workspace) {
  m_inputWorkspace = workspace;
  m_outputWorkspaceName = m_inputWorkspace.substr(0, m_inputWorkspace.length() - 4) + "_Moments";
}

void MomentsModel::setEMin(double eMin) { m_eMin = eMin; }

void MomentsModel::setEMax(double eMax) { m_eMax = eMax; }

void MomentsModel::setScale(bool scale) { m_scale = scale; }

void MomentsModel::setScaleValue(double scaleValue) { m_scaleValue = scaleValue; }

std::string MomentsModel::getOutputWorkspace() const { return m_outputWorkspaceName; }

} // namespace MantidQt::CustomInterfaces
