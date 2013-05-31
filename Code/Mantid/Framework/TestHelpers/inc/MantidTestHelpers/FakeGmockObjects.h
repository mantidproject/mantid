#ifndef FAKEGMOCKOBJECTS_H_
#define FAKEGMOCKOBJECTS_H_

/*
 * FakeObjects.h: Fake Tester objects for APITest
 *
 *  Created on: Jul 5, 2011
 *      Author: Janik Zikovsky
 */

#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/INearestNeighboursFactory.h"
#include "gmock/gmock.h"
#include <iostream>
#include <fstream>
#include <map>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid;


//Helper and typedef for mocking NearestNeighbourFactory usage
class MockNearestNeighboursFactory : public Mantid::Geometry::INearestNeighboursFactory
{
public:
  MOCK_METHOD3(create, Mantid::Geometry::INearestNeighbours*(boost::shared_ptr<const Mantid::Geometry::Instrument>,const Mantid::Geometry::ISpectrumDetectorMapping&, bool));
  MOCK_METHOD4(create, Mantid::Geometry::INearestNeighbours*(int,boost::shared_ptr<const Mantid::Geometry::Instrument>,const Mantid::Geometry::ISpectrumDetectorMapping&, bool));
  virtual ~MockNearestNeighboursFactory()
  {
  }
};

//Helper typedef and type for mocking NearestNeighbour map usage.
typedef std::map<Mantid::specid_t, Mantid::Kernel::V3D> SpectrumDistanceMap;
class MockNearestNeighbours : public Mantid::Geometry::INearestNeighbours {
 public:
  MOCK_CONST_METHOD2(neighboursInRadius, SpectrumDistanceMap(specid_t spectrum, double radius));
  MOCK_CONST_METHOD1(neighbours, SpectrumDistanceMap(specid_t spectrum));
  MOCK_METHOD0(die, void());
  virtual ~MockNearestNeighbours()
  {
    die();
  }
};




#endif /* FAKEOBJECTS_H_ */
