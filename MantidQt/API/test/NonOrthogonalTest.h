#ifndef MANTIDQT_API_NONORTHOGONALTEST_H_
#define MANTIDQT_API_NONORTHOGONALTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/MDUnitFactory.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidDataObjects/CoordTransformAffine.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/MDUnit.h"
#include "MantidCrystal/SetUB.h"

class NonOrthogonalTest : public CxxTest::TestSuite {
private:
	Mantid::API::IMDEventWorkspace_sptr getOrthogonalEventWorkspace() {
		Mantid::Kernel::ReciprocalLatticeUnitFactory factory;
		auto product = factory.create(Mantid::Kernel::Units::Symbol::RLU);
		Mantid::Geometry::HKL frame(product);
		auto workspace =
			Mantid::DataObjects::MDEventsTestHelper::makeMDEWWithFrames<3>(
				5, -10, 10, frame);
		return workspace;
	}
	size_t m_dimX = 0;
	size_t m_dimY = 1;
public:
	static NonOrthogonalTest *createSuite() {
		return new NonOrthogonalTest;
	}
	static void destroySuite(NonOrthogonalTest *suite) { delete suite; }
  void test_constructor() {
	  TSM_ASSERT("Orthogonal workspaces should not be transformed", true);
  }

};

#endif /* MANTIDQT_API_NONORTHOGONALTEST_H_ */
