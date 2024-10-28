// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
/**

This file defines functions for parsing numbers from QStrings.
*/
namespace MantidQt {
namespace MantidWidgets {
// Converts a string denoting a floating point value to a double.
double EXPORT_OPT_MANTIDQT_COMMON parseDouble(QString const &in);

/// Converts a string denoting a denary integer to it
int EXPORT_OPT_MANTIDQT_COMMON parseDenaryInteger(QString const &in);
} // namespace MantidWidgets
} // namespace MantidQt
