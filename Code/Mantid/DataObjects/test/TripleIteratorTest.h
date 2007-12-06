#ifndef TRIPLEITERATORTEST_
#define TRIPLEITERATORTEST_

#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/TripleRef.h" 
#include "MantidDataObjects/tripleIterator.h" 

using namespace Mantid::DataObjects;
using namespace Mantid::Iterator;

class tripleIteratorTest : public CxxTest::TestSuite
{
private: 
  typedef boost::shared_ptr<std::vector<double> > parray;
  typedef boost::shared_ptr<Workspace1D> W1D;
 public:
    W1D Create1DWorkspace(int size)
    {
      std::vector<double> x1,y1,e1;
      x1.resize(size);
      std::fill(x1.begin(),x1.end(),rand());	
      y1.resize(size);
      std::fill(y1.begin(),y1.end(),rand());
      e1.resize(size);
      W1D retVal = W1D(new Workspace1D);
      retVal->setX(x1);
      retVal->setData(y1,e1);
      return retVal;
    }

		void testIteratorWorkspace1DLength()
	  {
      int size = 100;
	    W1D workspace = Create1DWorkspace(size);

      int count = 0;
      for(triple_iterator<Workspace1D> ti(*workspace); ti != ti.end(); ++ti)
      {
        TS_ASSERT_THROWS_NOTHING
        (
          TripleRef<double&> tr = *ti;
          double d1 = tr[0];
          double d2 = tr[1];
          double d3 = tr[2];
        )
        count++;
      }
      TS_ASSERT_EQUALS(count,size);
    }

    void testIteratorWorkspace1DOrder()
    {
      int size = 200;
	    W1D workspace = Create1DWorkspace(size);

      std::vector<double> x1 = workspace->dataX();
      std::vector<double> y1 = workspace->dataY();
      std::vector<double> e1 = workspace->dataE();

      triple_iterator<Workspace1D> ti(*workspace);
      for (int i = 0; i < size; i++)
      {
        //move the iterator on one
        TripleRef<double&> tr = *ti;
        TS_ASSERT_EQUALS(tr[0],x1[i]);
        TS_ASSERT_EQUALS(tr[1],y1[i]);
        TS_ASSERT_EQUALS(tr[2],e1[i]);
        ++ti;
      }
      TS_ASSERT_EQUALS(ti,ti.end());
	  }
};
#endif /*TRIPLEITERATORTEST_*/
