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

#include <QDoubleValidator>
#include <QFileInfo>

using namespace DataValidationHelper;
using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

MomentsModel::MomentsModel() : m_inputWorkspace(), m_outputWorkspaceName(), m_eMin(), m_eMax(), m_scale(false) {}

IAlgorithm_sptr MomentsModel::setupAlgorithm() {
  IAlgorithm_sptr momentsAlg = AlgorithmManager::Instance().create("SofQWMoments", -1);
  momentsAlg->initialize();
  momentsAlg->setProperty("InputWorkspace", m_inputWorkspace);
  momentsAlg->setProperty("EnergyMin", m_eMin);
  momentsAlg->setProperty("EnergyMax", m_eMax);
  momentsAlg->setProperty("OutputWorkspace", m_outputWorkspaceName);

  if (m_scale) {
    momentsAlg->setProperty("Scale", m_scaleValue);
  } else {
    momentsAlg->setProperty("Scale", 1.0);
  }

  return momentsAlg;
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
