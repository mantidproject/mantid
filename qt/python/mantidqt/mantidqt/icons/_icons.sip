// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// This file defines the Python bindings for the
// _icons library.
%Module(name=_iconsqt5)

%ModuleCode
#include "MantidQtIcons/Icon.h"
#include "MantidKernel/WarningSuppressions.h"

#if defined(__GNUC__) && !defined(__clang__)
  // We need default visibility for this module so that
  // python can see the init function.
  #pragma GCC visibility push(default)
#endif
MSVC_DIAG_OFF(4290)
GNU_DIAG_OFF("pedantic")
GNU_DIAG_OFF("suggest-override")
GNU_DIAG_OFF("missing-field-initializers")

using namespace MantidQt::Icons;
%End

// PyQt5 Gui module
%Import QtGui/QtGuimod.sip

QIcon getIcon(const QString &iconName, const QString &color = QString("black"), const double scaleFactor = 1.0);

QIcon getIcon(const QStringList &iconNames, const QList<QVariant> &options);
