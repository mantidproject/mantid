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

/*
* The ProgressObserver keeps track of progress, finish and error events happening
* to an algorithm, it will push an update to the model, which updates the presenters.
* 
* This class does not track the starting of algorithms, and needs to be attached
* to an already running algorithm.
*/
namespace MantidQt {
namespace MantidWidgets {

    class AlgorithmProgressModel;

    class ProgressObserver : public Mantid::API::AlgorithmObserver {
    public:
        ProgressObserver(AlgorithmProgressModel* model);

        void startHandle(const Mantid::API::IAlgorithm* alg) override;
        void finishHandle(const Mantid::API::IAlgorithm* alg) override;
        void progressHandle(const Mantid::API::IAlgorithm* alg, double progress,
            const std::string& message) override;
        void errorHandle(const Mantid::API::IAlgorithm* alg,
            const std::string& what) override;

        void removeFrom(const Mantid::API::IAlgorithm* alg);

        std::vector<const Mantid::API::IAlgorithm*> managedAlgs;

    private:
        AlgorithmProgressModel* m_model;
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSOBSERVER_H