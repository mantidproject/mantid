// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobRunner.h"
namespace MantidQt {
namespace CustomInterfaces {

BatchJobRunner::BatchJobRunner(Batch batch)
    : m_batch(std::move(batch)), m_isProcessing(false),
      m_isAutoreducing(false) {}

bool BatchJobRunner::isProcessing() const { return m_isProcessing; }

bool BatchJobRunner::isAutoreducing() const { return m_isAutoreducing; }

void BatchJobRunner::resumeReduction() { m_isProcessing = true; }

void BatchJobRunner::pauseReduction() { m_isProcessing = false; }

void BatchJobRunner::resumeAutoreduction() { m_isAutoreducing = true; }

void BatchJobRunner::pauseAutoreduction() { m_isAutoreducing = false; }

} // namespace CustomInterfaces
} // namespace MantidQt
