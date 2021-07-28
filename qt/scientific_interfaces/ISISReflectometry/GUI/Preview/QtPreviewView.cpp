// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPreviewView.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
QtPreviewView::QtPreviewView(QWidget *parent) : QWidget(parent) { m_ui.setupUi(this); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry