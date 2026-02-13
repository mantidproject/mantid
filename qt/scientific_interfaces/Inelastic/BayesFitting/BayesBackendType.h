// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <QString>

namespace MantidQt {
namespace CustomInterfaces {

enum class BayesBackendType { QUASI_ELASTIC_BAYES, QUICK_BAYES };

std::unordered_map<BayesBackendType, QString> const backendToQStr{
    {BayesBackendType::QUASI_ELASTIC_BAYES, QString("quasielasticbayes")},
    {BayesBackendType::QUICK_BAYES, QString("quickbayes")},
};

} // namespace CustomInterfaces
} // namespace MantidQt
