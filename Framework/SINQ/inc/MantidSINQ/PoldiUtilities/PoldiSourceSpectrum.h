// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDISOURCESPECTRUM_H
#define MANTID_SINQ_POLDISOURCESPECTRUM_H

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
  PoldiSourceSpectrum(Kernel::Interpolation spectrum);
  PoldiSourceSpectrum(Geometry::Instrument_const_sptr poldiInstrument);
  double intensity(double wavelength) const;

protected:
  void
  setSpectrumFromInstrument(Geometry::Instrument_const_sptr poldiInstrument);
  Geometry::IComponent_const_sptr
  getSourceComponent(Geometry::Instrument_const_sptr poldiInstrument);
  Geometry::Parameter_sptr
  getSpectrumParameter(Geometry::IComponent_const_sptr source,
                       Geometry::ParameterMap_sptr instrumentParameterMap);
  void setSpectrum(Geometry::Parameter_sptr spectrumParameter);

  Kernel::Interpolation m_spectrum;
};

using PoldiSourceSpectrum_sptr = boost::shared_ptr<PoldiSourceSpectrum>;
using PoldiSourceSpectrum_const_sptr =
    boost::shared_ptr<const PoldiSourceSpectrum>;
} // namespace Poldi
} // namespace Mantid

#endif // MANTID_SINQ_POLDISOURCESPECTRUM_H
