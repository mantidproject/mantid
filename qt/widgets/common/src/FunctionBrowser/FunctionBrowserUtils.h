// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDWIDGETS_FUNCTIONBROWSERUTILS_H_
#define MANTIDWIDGETS_FUNCTIONBROWSERUTILS_H_
#include "MantidAPI/IFunction_fwd.h"

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

/**
 * Split a qualified parameter name into function index and local parameter
 * name.
 * @param paramName :: Fully qualified parameter name (includes function index)
 *   for example: f0.f1.A0
 * @return :: A pair with the first item is the function index and the
 * second item is the param local name.
 */
std::pair<QString, QString> splitParameterName(const QString &paramName);

IFunction_sptr getFunctionWithPrefix(const QString& prefix, const IFunction_sptr &fun);

} // namespace MantidWidgets
} // namespace MantidQt


#endif // MANTIDWIDGETS_FUNCTIONBROWSERUTILS_H_
