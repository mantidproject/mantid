// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDIDETECTORFACTORY_H_
#define MANTID_SINQ_POLDIDETECTORFACTORY_H_

#include "MantidSINQ/DllConfig.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "boost/date_time/gregorian/gregorian.hpp"

namespace Mantid {
namespace Poldi {
/** PoldiDetectorFactory :
 *
 *Simple factory

  @author Michael Wedel, Paul Scherrer Institut - SINQ
  @date 07/02/2014
*/

class MANTID_SINQ_DLL PoldiDetectorFactory {
public:
  PoldiDetectorFactory();
  virtual ~PoldiDetectorFactory() = default;

  virtual PoldiAbstractDetector *createDetector(std::string detectorType);
  virtual PoldiAbstractDetector *
  createDetector(boost::gregorian::date experimentDate);

protected:
  boost::gregorian::date m_newDetectorDate;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDIDETECTORFACTORY_H_ */
