// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FitScriptGenerator.h"

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(FitScriptGenerator)

FitScriptGenerator::FitScriptGenerator(QWidget *parent)
    : MantidQt::API::UserSubWindow(parent), m_view(new MantidWidgets::FitScriptGeneratorView()),
      m_model(std::make_unique<MantidWidgets::FitScriptGeneratorModel>()),
      m_presenter(std::make_unique<MantidWidgets::FitScriptGeneratorPresenter>(m_view, m_model.get())) {}

FitScriptGenerator::~FitScriptGenerator() = default;

void FitScriptGenerator::initLayout() {
  this->setCentralWidget(m_view);
  this->setMinimumHeight(500);
}

} // namespace CustomInterfaces
} // namespace MantidQt
