// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * \class StubInstrumentActor
 * \brief A stub implementation of IInstrumentActor with all methods doing nothing.
 *
 * This class provides a no-op implementation of the IInstrumentActor interface
 * just to make the Python instrument view work
 */
class StubInstrumentActor : public IInstrumentActor {
  Q_OBJECT
public:
  StubInstrumentActor(Mantid::API::MatrixWorkspace_sptr const &workspace) : m_workspace(workspace) {};
  ~StubInstrumentActor() override = default;

  void draw(bool /*picking*/ = false) const override {}

  void getBoundingBox(Mantid::Kernel::V3D & /*minBound*/, Mantid::Kernel::V3D & /*maxBound*/,
                      const bool /*excludeMonitors*/) const override {}

  std::shared_ptr<const Mantid::Geometry::Instrument> getInstrument() const override {
    return m_workspace->getInstrument();
  }

  std::shared_ptr<const Mantid::API::MatrixWorkspace> getWorkspace() const override { return m_workspace; }

  const Mantid::Geometry::ComponentInfo &componentInfo() const override { return m_workspace->componentInfo(); }

  const Mantid::Geometry::DetectorInfo &detectorInfo() const override { return m_workspace->detectorInfo(); }

  GLColor getColor(size_t /*index*/) const override { return GLColor(0, 0, 0, 0); }
  double minBinValue() const override { return 0.0; }
  double maxBinValue() const override { return 0.0; }
  size_t ndetectors() const override { return 0; }
  Mantid::detid_t getDetID(size_t /*pickID*/) const override { return -1; }
  const Mantid::Kernel::V3D getDetPos(size_t /*pickID*/) const override { return Mantid::Kernel::V3D(0, 0, 0); }
  double getIntegratedCounts(size_t /*index*/) const override { return 0.0; }
  size_t getWorkspaceIndex(size_t /*index*/) const override { return 0; }
  void getBinMinMaxIndex(size_t /*wi*/, size_t & /*imin*/, size_t & /*imax*/) const override {}
  const std::vector<size_t> &components() const override {
    static const std::vector<size_t> emptyVec;
    return emptyVec;
  }
  const InstrumentRenderer &getInstrumentRenderer() const override {
    static const InstrumentRenderer *stubRenderer = nullptr;
    if (!stubRenderer) {
      throw std::runtime_error("StubInstrumentActor: getInstrumentRenderer() is not properly implemented");
    }
    return *stubRenderer;
  }

private:
  Mantid::API::MatrixWorkspace_sptr m_workspace;
};

} // namespace MantidWidgets
} // namespace MantidQt
