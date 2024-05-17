// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunPresenter.h"
#include "RunView.h"

namespace MantidQt {
namespace CustomInterfaces {

RunPresenter::RunPresenter(IRunView *view) : m_view(view) { m_view->subscribePresenter(this); }

} // namespace CustomInterfaces
} // namespace MantidQt