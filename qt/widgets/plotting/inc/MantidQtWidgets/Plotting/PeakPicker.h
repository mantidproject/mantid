// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGET_PEAKPICKER_H
#define MANTIDQT_MANTIDWIDGET_PEAKPICKER_H

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Plotting/Qwt/PeakPicker.h"
#else
#include "MantidQtWidgets/Plotting/Mpl/PeakPicker.h"
#endif

#endif // MANTIDQT_MANTIDWIDGET_PEAKPICKER_H
