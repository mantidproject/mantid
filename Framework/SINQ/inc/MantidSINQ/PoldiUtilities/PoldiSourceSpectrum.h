// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Interpolation.h"
#include "MantidSINQ/DllConfig.h"

#include "boost/shared_ptr.hpp"

namespace Mantid {
namespace Poldi {

/** PoldiSourceSpectrum :
 *
  Class that represents the source spectrum at POLDI. It is constructed
  from a list of Wavelength/Intensity-pairs and actual intensities for
  a certain wavelength are obtained by interpolation.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 13/05/2014
*/

class MANTID_SINQ_DLL PoldiSourceSpectrum {
public:
  PoldiSourceSpectrum(const Kernel::Interpolation &spectrum);
  PoldiSourceSpectrum(const Geometry::Instrument_const_sptr &poldiInstrument);
  double intensity(double wavelength) const;

protected:
  void setSpectrumFromInstrument(const Geometry::Instrument_const_sptr &poldiInstrument);
  Geometry::IComponent_const_sptr getSourceComponent(const Geometry::Instrument_const_sptr &poldiInstrument);
  Geometry::Parameter_sptr getSpectrumParameter(const Geometry::IComponent_const_sptr &source,
                                                const Geometry::ParameterMap_sptr &instrumentParameterMap);
  void setSpectrum(const Geometry::Parameter_sptr &spectrumParameter);

  Kernel::Interpolation m_spectrum;
};

using PoldiSourceSpectrum_sptr = std::shared_ptr<PoldiSourceSpectrum>;
using PoldiSourceSpectrum_const_sptr = std::shared_ptr<const PoldiSourceSpectrum>;
} // namespace Poldi
} // namespace Mantid
