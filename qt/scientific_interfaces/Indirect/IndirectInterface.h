// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_

#include "MantidQtWidgets/Common/UserSubWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

class IndirectInterface : public MantidQt::API::UserSubWindow {
  // Q_OBJECT

public:
  explicit IndirectInterface(QWidget *parent = nullptr);

private:
  virtual void initLayout() override = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTINTERFACE_H_ */
