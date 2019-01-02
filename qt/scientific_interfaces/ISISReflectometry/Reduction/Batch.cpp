// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Batch.h"
namespace MantidQt {
namespace CustomInterfaces {

Batch::Batch(Experiment const &experiment, Instrument const &instrument,
             ReductionJobs const &reductionJobs, Slicing const &slicing)
    : m_experiment(experiment), m_instrument(instrument),
      m_reductionJobs(reductionJobs), m_slicing(slicing) {}

Experiment const &Batch::experiment() const { return m_experiment; }

Instrument const &Batch::instrument() const { return m_instrument; }

ReductionJobs const &Batch::reductionJobs() const { return m_reductionJobs; }

Slicing const &Batch::slicing() const { return m_slicing; }
} // namespace CustomInterfaces
} // namespace MantidQt
