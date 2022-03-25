// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "IInstViewModel.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL InstViewModel : public IInstViewModel {
public:
  InstViewModel(std::unique_ptr<MantidWidgets::IMessageHandler> messageHandler = nullptr);
  void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) override;
  MantidWidgets::InstrumentActor *getInstrumentViewActor() const override;
  Mantid::Kernel::V3D getSamplePos() const override;
  Mantid::Kernel::V3D getAxis() const override;
  std::vector<Mantid::detid_t> detIndicesToDetIDs(std::vector<size_t> const &detIndices) const override;

private:
  std::unique_ptr<MantidWidgets::InstrumentActor> m_actor;
  std::unique_ptr<MantidWidgets::IMessageHandler> m_messageHandler;

  std::unique_ptr<MantidWidgets::InstrumentActor>
  createInstrumentViewActor(Mantid::API::MatrixWorkspace_sptr &workspace) const;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
