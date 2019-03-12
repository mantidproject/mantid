// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressObserver.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

namespace MantidQt {
namespace MantidWidgets {

    ProgressObserver::ProgressObserver(AlgorithmProgressModel* model)
        : managedAlgs { std::vector<const Mantid::API::IAlgorithm*>() }
        , m_model(model)
    {
    }

    void ProgressObserver::startHandle(const Mantid::API::IAlgorithm* alg)
    {
        managedAlgs.push_back(alg);
        m_model->updatePresenters();
    }

    void ProgressObserver::finishHandle(const Mantid::API::IAlgorithm* alg)
    {
        this->removeFrom(alg);
    }

    void ProgressObserver::errorHandle(const Mantid::API::IAlgorithm* alg,
        const std::string& what)
    {
        this->removeFrom(alg);
    }
    void ProgressObserver::progressHandle(const Mantid::API::IAlgorithm* alg,
        const double progress, const std::string& message)
    {
        m_model->updateProgress(alg, progress, message);
    }

    void ProgressObserver::removeFrom(const Mantid::API::IAlgorithm* alg)
    {
        this->stopObserving(alg);
        const auto it = std::find(managedAlgs.begin(), managedAlgs.end(), alg);
        if (it != managedAlgs.end()) {
            managedAlgs.erase(it);
        }
    }

} // namespace MantidWidgets
} // namespace MantidQt