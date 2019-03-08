// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSOBSERVER_H
#define ALGORITHMPROGRESSOBSERVER_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressModel;

    class ProgressObserver : public Mantid::API::AlgorithmObserver {
        /*
   * This progress observer will update the progress bar, but will NOT observe
   * start events
   */
    public:
        ProgressObserver(AlgorithmProgressModel* model,
            Mantid::API::IAlgorithm_sptr alg);

        auto name() const { return m_alg->name(); }

        auto properties() const { return m_alg->getProperties(); }

        void finishHandle(const Mantid::API::IAlgorithm* alg) override;
        void progressHandle(const Mantid::API::IAlgorithm* alg, double p,
            const std::string& msg) override;
        void errorHandle(const Mantid::API::IAlgorithm* alg,
            const std::string& what) override;

        void stopObservingCurrentAlgorithm();
        Mantid::API::IAlgorithm_sptr currentAlgorithm() const;

    private:
        AlgorithmProgressModel* m_model;

        /// The algorithm that is being observed.
        /// This object does NOT own the algorithm.
        Mantid::API::IAlgorithm_sptr m_alg;
        double progress { 0 };
        std::string message { "}" };
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSOBSERVER_H