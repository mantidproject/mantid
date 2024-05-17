// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunView.h"

#include "RunPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

RunView::RunView(QWidget *parent) : QWidget(parent), m_presenter() { m_uiForm.setupUi(parent); }

void RunView::subscribePresenter(IRunPresenter *presenter) { m_presenter = presenter; }

} // namespace CustomInterfaces
} // namespace MantidQt