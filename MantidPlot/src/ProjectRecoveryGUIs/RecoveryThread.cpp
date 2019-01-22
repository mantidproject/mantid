// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RecoveryThread.h"
#include "ProjectRecovery.h"

bool RecoveryThread::getFailedRun() { return m_failedRunInThread; }

void RecoveryThread::setCheckpoint(const Poco::Path &checkpoint) {
  m_checkpoint = checkpoint;
}

void RecoveryThread::setProjRecPtr(MantidQt::ProjectRecovery *projectRec) {
  m_projRec = projectRec;
}

void RecoveryThread::run() {
  m_failedRunInThread = !m_projRec->loadRecoveryCheckpoint(m_checkpoint);
}