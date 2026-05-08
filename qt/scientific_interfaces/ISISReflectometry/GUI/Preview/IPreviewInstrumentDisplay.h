// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"

#include <QLayout>
#include <cstddef>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class IPreviewInstrumentDisplay {
public:
  virtual ~IPreviewInstrumentDisplay() = default;

  virtual void updateWorkspace(Mantid::API::MatrixWorkspace_sptr &workspace) = 0;
  virtual void resetInstView() = 0;
  virtual void plotInstView() = 0;
  virtual void setInstViewZoomMode() = 0;
  virtual void setInstViewEditMode() = 0;
  virtual void setInstViewSelectRectMode() = 0;
  virtual std::vector<Mantid::detid_t> getSelectedDetectorIDs() const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
