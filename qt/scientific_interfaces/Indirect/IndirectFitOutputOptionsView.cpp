// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsView.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputOptionsView::IndirectFitOutputOptionsView(QWidget *parent)
    : API::MantidWidget(parent),
      m_outputOptions(new Ui::IndirectFitOutputOptions) {
  m_outputOptions->setupUi(this);

  // connect(m_selector->cbMaskSpectrum, SIGNAL(currentIndexChanged(int)), this,
  //        SLOT(enableMaskLineEdit(int)));
}

IndirectFitOutputOptionsView::~IndirectFitOutputOptionsView() {}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
