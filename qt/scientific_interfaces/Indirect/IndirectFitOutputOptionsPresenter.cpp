// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsPresenter::IndirectFitOutputOptionsPresenter(
    IndirectFitOutputOptionsModel *model, IndirectFitOutputOptionsView *view)
    : QObject(nullptr), m_model(model), m_view(view) {
  // connect(m_view.get(), SIGNAL(selectedSpectraChanged(const std::string &)),
  //        this, SLOT(updateSpectraList(const std::string &)));
}

IndirectFitOutputOptionsPresenter::~IndirectFitOutputOptionsPresenter() {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
