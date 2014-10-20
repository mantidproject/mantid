#ifndef MANTID_GEOMETRY_RIGIDATOMSCATTERERTEST_H_
#define MANTID_GEOMETRY_RIGIDATOMSCATTERERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/RigidAtomScatterer.h"

using Mantid::Geometry::RigidAtomScatterer;
using namespace Mantid::Kernel;

class RigidAtomScattererTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static RigidAtomScattererTest *createSuite() { return new RigidAtomScattererTest(); }
    static void destroySuite( RigidAtomScattererTest *suite ) { delete suite; }


    void testScatterer()
    {
        RigidAtomScatterer scatterer("Si", V3D(0.0, 0.0, 0.0));
    }


};


#endif /* MANTID_GEOMETRY_RIGIDATOMSCATTERERTEST_H_ */
