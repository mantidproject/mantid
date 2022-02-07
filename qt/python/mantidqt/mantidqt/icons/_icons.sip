// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// This file defines the Python bindings for the
// _icons library.
%Module(name=_iconsqt5)

%UnitCode
#if defined(_MSC_VER)
  # pragma warning( disable: 4290 )
#elif defined(__GNUC__) && !defined(__clang__)
  // We need default visibility for this module so that
  // python can see the init function.
  #pragma GCC visibility push(default)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wpedantic"
  #if __GNUC__ >= 5
    #pragma GCC diagnostic ignored "-Wsuggest-override"
  #endif
#endif
%End

%ModuleCode
#include "MantidQtIcons/Icon.h"
using namespace MantidQt::Icons;
%End

// PyQt5 Gui module
%Import QtGui/QtGuimod.sip

QIcon getIcon(const QString &iconName, const QString &color = QString("black"), const double scaleFactor = 1.0);

QIcon getIcon(const QStringList &iconNames, const QList<QVariant> &options);
