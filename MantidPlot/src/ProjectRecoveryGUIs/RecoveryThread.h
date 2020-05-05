// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <Poco/Path.h>
#include <QThread>

namespace MantidQt {
class ProjectRecovery;
}

class RecoveryThread : public QThread {
  Q_OBJECT

public:
  RecoveryThread() { m_projRec = nullptr; }
  bool getFailedRun();
  void setCheckpoint(const Poco::Path &checkpoint);
  void setProjRecPtr(MantidQt::ProjectRecovery *projectRec);

protected:
  void run() override;

private:
  bool m_failedRunInThread = true;
  Poco::Path m_checkpoint;
  MantidQt::ProjectRecovery *m_projRec;
};
