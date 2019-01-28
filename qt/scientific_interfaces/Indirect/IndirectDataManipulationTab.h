// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_
#include "IndirectDataManipulation.h"
#include "IndirectTab.h"

#include "DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectDataManipulationTab : public IndirectTab {
  Q_OBJECT

public:
  IndirectDataManipulationTab(QWidget *parent = nullptr);

private:
  //IndirectDataManipulation *m_parent;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATAMANIPULATIONTAB_H_ */
