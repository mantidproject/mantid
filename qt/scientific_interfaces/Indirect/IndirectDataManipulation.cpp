// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataManipulation.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
DECLARE_SUBWINDOW(IndirectDataManipulation)

IndirectDataManipulation::IndirectDataManipulation(QWidget *parent)
    : UserSubWindow(parent),
      m_changeObserver(*this,
                       &IndirectDataManipulation::handleDirectoryChange) {
  m_uiForm.setupUi(this);

  //m_tabs.emplace(SYMMETRISE2, new IndirectSymmetrise(
  //                                m_uiForm.twIDMTabs->widget(SYMMETRISE2)));
  //m_tabs.emplace(SQW2, new IndirectSqw(m_uiForm.twIDMTabs->widget(SQW2)));
  //m_tabs.emplace(MOMENTS2,
  //               new IndirectMoments(m_uiForm.twIDMTabs->widget(MOMENTS2)));
  m_tabs.emplace(ELWIN2, new Elwin(m_uiForm.twIDMTabs->widget(ELWIN2)));
  m_tabs.emplace(IQT2, new Iqt(m_uiForm.twIDMTabs->widget(IQT2)));
}

void IndirectDataManipulation::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
