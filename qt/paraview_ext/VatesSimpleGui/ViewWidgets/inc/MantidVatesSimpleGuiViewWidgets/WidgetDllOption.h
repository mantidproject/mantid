// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDVATES_SIMPLEGUI_VIEWWIDGETS_DLLOPTION_H_
#define MANTIDVATES_SIMPLEGUI_VIEWWIDGETS_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTID_VATES_SIMPLEGUI_VIEWWIDGETS
#define EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS DLLExport
#else
#define EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS DLLImport
#endif // IN_MANTID_VATES_SIMPLEGUI_VIEWWIDGETS

#endif // MANTIDVATES_SIMPLEGUI_VIEWWIDGETS_DLLOPTION_H_
