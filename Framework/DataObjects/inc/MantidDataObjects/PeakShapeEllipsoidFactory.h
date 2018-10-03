// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_

#include "MantidKernel/System.h"
#include "PeakShapeFactory.h"

namespace Mantid {
namespace DataObjects {

/** PeakShapeEllipsoidFactory : Create ellipsoid peak shapes
*/
class DLLExport PeakShapeEllipsoidFactory : public PeakShapeFactory {
public:
  // PeakShapeFactory interface

  Mantid::Geometry::PeakShape *create(const std::string &source) const override;
  void setSuccessor(
      boost::shared_ptr<const PeakShapeFactory> successorFactory) override;

private:
  /// Successor factory
  PeakShapeFactory_const_sptr m_successor;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORY_H_ */
