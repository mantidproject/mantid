// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/OptionsDialogPresenter.h"

using namespace MantidQt::MantidWidgets;

/**
 * Construct a new presenter with the given view
 * @param view :: a handle to a view for this presenter
 */
OptionsDialogPresenter::OptionsDialogPresenter(OptionsDialog *view)
    : m_view(view){}