// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_

#include "ui_IndirectDataManipulation.h"

#include "IndirectDataManipulationTab.h"

#include "Elwin.h"
#include "IndirectMoments.h"
#include "IndirectSqw.h"
#include "IndirectSymmetrise.h"
#include "Iqt.h"

#include "MantidQtWidgets/Common/UserSubWindow.h"

#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum IDMTabChoice { SYMMETRISE2, SQW2, MOMENTS2, ELWIN2, IQT2 };

class IndirectDataManipulation : public MantidQt::API::UserSubWindow {
  Q_OBJECT

  friend class IndirectDataManipulationTab;

public:
  explicit IndirectDataManipulation(QWidget *parent = nullptr);
  ~IndirectDataManipulation() override;

  static std::string name() { return "Data Manipulation"; }
  static QString categoryInfo() { return "Indirect"; }

private:
  void
  handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf);

  Poco::NObserver<IndirectDataManipulation,
                  Mantid::Kernel::ConfigValChangeNotification>
      m_changeObserver;
  std::map<unsigned int, IndirectDataManipulationTab *> m_tabs;

  Ui::IndirectDataManipulation m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATION_H_ */
