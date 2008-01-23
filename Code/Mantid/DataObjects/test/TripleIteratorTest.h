#ifndef TRIPLEITERATORTEST_
#define TRIPLEITERATORTEST_

#include <algorithm> 
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/Workspace2D.h" 
#include "MantidAPI/TripleRef.h" 
#include "MantidAPI/TripleIterator.h" 

using namespace Mantid::DataObjects;
using namespace Mantid::API;

template<typename T>
class FibSeries
{
private:
  T x1;  /// Initial value 1;
  T x2;  /// Initial value 2;

public:

  FibSeries() : x1(1),x2(1) {}
  T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
};


class tripleIteratorTest : public CxxTest::TestSuite
{
private: 

  typedef boost::shared_ptr<std::vector<double> > parray;
  typedef boost::shared_ptr<Workspace1D> W1D;
  typedef boost::shared_ptr<Workspace2D> W2D;
  typedef boost::shared_ptr<Workspace> Wbase;

public:

  parray CreateRandomArray(int size)
  {
    parray x(new std::vector<double>);
    x->resize(size);
    std::generate(x->begin(),x->end(),rand);
    return x;
  }

  W1D Create1DWorkspace(int size)
  {
    W1D retVal(new Workspace1D);
    retVal->setX(CreateRandomArray(size));
    retVal->setData(CreateRandomArray(size),CreateRandomArray(size));
    return retVal;
  }

  W2D Create2DWorkspace(int histogramCount,int size)
  {
    W2D retVal(new Workspace2D);
    retVal->setHistogramNumber(histogramCount);
    for (int i = 0; i < histogramCount; i++)
    {
      retVal->setX(i,CreateRandomArray(size));
      retVal->setData(i,CreateRandomArray(size),CreateRandomArray(size));
    }
    return retVal;
  }

  W1D Create1DWorkspaceFib(int size)
  {
    std::vector<double> x1,y1,e1;
    x1.resize(size);
    std::generate(x1.begin(),x1.end(),rand);	
    y1.resize(size);
    std::generate(y1.begin(),y1.end(),FibSeries<double>());
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
    for(Workspace1D::const_iterator ti(*workspace); ti != ti.end(); ++ti)
    {
      TS_ASSERT_THROWS_NOTHING
      (
        TripleRef<double> tr = *ti;
        TS_ASSERT_EQUALS(tr[0],workspace->dataX(0)[count]);
        TS_ASSERT_EQUALS(tr[1],workspace->dataY(0)[count]);
        TS_ASSERT_EQUALS(tr[2],workspace->dataE(0)[count]);
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

    Workspace1D::const_iterator ti(*workspace);
    for (int i = 0; i < size; i++) 
    {
      //move the iterator on one
      TripleRef<double> tr = *ti;
      TS_ASSERT_EQUALS(tr[0],x1[i]);
      TS_ASSERT_EQUALS(tr[1],y1[i]);
      TS_ASSERT_EQUALS(tr[2],e1[i]);
      ti++;
    }
    TS_ASSERT_EQUALS(ti,ti.end());
  }


  void testIteratorWorkspace1DAsBase()
  {
    int size = 57;
    Wbase workspace = Create1DWorkspace(size);

    int count = 0;
    for(Workspace::const_iterator ti(*workspace); ti != ti.end(); ++ti)
    {
      TS_ASSERT_THROWS_NOTHING
        (
        TripleRef<double> tr = *ti;

        TS_ASSERT_EQUALS(tr[0],workspace->dataX(0)[count]);
        TS_ASSERT_EQUALS(tr[1],workspace->dataY(0)[count]);
        TS_ASSERT_EQUALS(tr[2],workspace->dataE(0)[count]);
      )
        count++;
    }
    TS_ASSERT_EQUALS(count,size);
  }


  void testIteratorWorkspace2DAsBase()
  {
    int size = 57;
    int histogramCount = 100;
    Wbase workspace = Create2DWorkspace(histogramCount,size);
    //workspace->dataX(0) // this is the first spectrum in the workspace with real data
    int count = 0;
    for(Workspace::const_iterator ti(*workspace); ti != ti.end(); ++ti)
    {
      TS_ASSERT_THROWS_NOTHING
      (
        TripleRef<double> tr = *ti;
        int datablock = count/size;
        int blockindex = count%size;
        TS_ASSERT_EQUALS(tr[0],workspace->dataX(datablock)[blockindex]);
        TS_ASSERT_EQUALS(tr[1],workspace->dataY(datablock)[blockindex]);
        TS_ASSERT_EQUALS(tr[2],workspace->dataE(datablock)[blockindex]);
      )
        count++;
    }
    TS_ASSERT_EQUALS(count,size*histogramCount);
  }

  void testIteratorCopy()
  {
    int size = 10;
    W1D workA = Create1DWorkspaceFib(size);
    W1D workB = Create1DWorkspace(size);


    Workspace1D::const_iterator IA(*workA);
    Workspace1D::iterator IB(*workB);

    std::copy(IA.begin(),IA.end(),IB.begin());
    const std::vector<double>& x1 = workA->dataX();
    const std::vector<double>& y1 = workA->dataY();
    const std::vector<double>& e1 = workA->dataE();

    const std::vector<double>& x2 = workB->dataX();
    const std::vector<double>& y2 = workB->dataY();
    const std::vector<double>& e2 = workB->dataE();


    for (int i = 0; i < size; i++) 
    {
      TS_ASSERT_EQUALS(x1[i],x2[i]);
      TS_ASSERT_EQUALS(y1[i],y2[i]);
      TS_ASSERT_EQUALS(e1[i],e2[i]);
    }
  }

  // Disable this test until it uses cxxtest instead of cerr output
  void xtestIteratorSort()
  {
    int size = 10;
    W1D workA = Create1DWorkspaceFib(size);
    Workspace1D::iterator IA(*workA);

    // Note: this used boost lambda since I am being lazy.
    //      sort(IA.begin(),IA.end(), (boost::bind(&TripleRef<double>::first,_1)() >
    // 				 boost::bind(&TripleRef<double>::first,_2)() );

    const std::vector<double>& x1 = workA->dataX();
    const std::vector<double>& y1 = workA->dataY();
    const std::vector<double>& e1 = workA->dataE();


    for (int i = 0; i < size; i++) 
    {
      std::cerr<<x1[i]<<std::endl;
    }
    return;
  }

  void testHorizontalLoopIteratorWorkspace1D()
  {
    int size = 13;
    const int loopCountArrayLength = 6;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 5;
    loopCountArray[4] = 11;
    loopCountArray[5] = 0;
    
    Wbase workspace = Create1DWorkspace(size);

    for (int i = 0; i < loopCountArrayLength; i++)
    {
      int loopCount = loopCountArray[i];
      int count = 0;
      for(Workspace::const_iterator ti(*workspace,loopCount); ti != ti.end(); ++ti)
      {
        TS_ASSERT_THROWS_NOTHING
        (
          TripleRef<double> tr = *ti;
          TS_ASSERT_EQUALS(tr[0],workspace->dataX(0)[count%size]);
          TS_ASSERT_EQUALS(tr[1],workspace->dataY(0)[count%size]);
          TS_ASSERT_EQUALS(tr[2],workspace->dataE(0)[count%size]);
        )
          count++;
      }
      TS_ASSERT_EQUALS(count,size*loopCount);
    }
  }

  void testHorizontalLoopIteratorWorkspace2D()
  {
    int size = 57;
    int histogramCount = 100;
    Wbase workspace = Create2DWorkspace(histogramCount,size);

    const int loopCountArrayLength = 4;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 0;
    
    for (int i = 0; i < loopCountArrayLength; i++)
    {
      int loopCount = loopCountArray[i];
      int count = 0;
      for(Workspace::const_iterator ti(*workspace,loopCount); ti != ti.end(); ++ti)
      {
        TS_ASSERT_THROWS_NOTHING
        (
          TripleRef<double> tr = *ti;
          int indexPosition = count%(size*histogramCount);
          int datablock = indexPosition/size;
          int blockindex = indexPosition%size;
          TS_ASSERT_EQUALS(tr[0],workspace->dataX(datablock)[blockindex]);
          TS_ASSERT_EQUALS(tr[1],workspace->dataY(datablock)[blockindex]);
          TS_ASSERT_EQUALS(tr[2],workspace->dataE(datablock)[blockindex]);
        )
          count++;
      }
      TS_ASSERT_EQUALS(count,size*histogramCount*loopCount);
    }
  }

   void testVerticalLoopIteratorWorkspace1D()
  {
    int size = 13;
    const int loopCountArrayLength = 6;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 5;
    loopCountArray[4] = 11;
    loopCountArray[5] = 0;
    
    Wbase workspace = Create1DWorkspace(size);

    for (int i = 0; i < loopCountArrayLength; i++)
    {
      int loopCount = loopCountArray[i];
      int count = 0;
      for(Workspace::const_iterator ti(*workspace,loopCount,LoopOrientation::Vertical); ti != ti.end(); ++ti)
      {
        TS_ASSERT_THROWS_NOTHING
        (
          TripleRef<double> tr = *ti;
          TS_ASSERT_EQUALS(tr[0],workspace->dataX(0)[count/loopCount]);
          TS_ASSERT_EQUALS(tr[1],workspace->dataY(0)[count/loopCount]);
          TS_ASSERT_EQUALS(tr[2],workspace->dataE(0)[count/loopCount]);
        )
          count++;
      }
      TS_ASSERT_EQUALS(count,size*loopCount);
    }
  }

   
  void testVerticalLoopIteratorWorkspace2D()
  {
    int size = 50;
    int histogramCount = 100;
    Wbase workspace = Create2DWorkspace(histogramCount,size);

    const int loopCountArrayLength = 4;
    int loopCountArray[loopCountArrayLength];
    loopCountArray[0] = 1;
    loopCountArray[1] = 2;
    loopCountArray[2] = 3;
    loopCountArray[3] = 0;
    
    for (int i = 0; i < loopCountArrayLength; i++)
    {
      int loopCount = loopCountArray[i];
      int count = 0;
      for(Workspace::const_iterator ti(*workspace,loopCount,LoopOrientation::Vertical); ti != ti.end(); ++ti)
      {
        TS_ASSERT_THROWS_NOTHING
        (
          //TripleRef<double> tr = *ti;
          int datablock = count/(size*loopCount);
          int blockindex = count/loopCount;
          //TS_ASSERT_EQUALS(tr[0],workspace->dataX(datablock)[blockindex]);
          //TS_ASSERT_EQUALS(tr[1],workspace->dataY(datablock)[blockindex]);
          //TS_ASSERT_EQUALS(tr[2],workspace->dataE(datablock)[blockindex]);
        )
          count++;
      }
      TS_ASSERT_EQUALS(count,size*histogramCount*loopCount);
    }
  }

};
#endif /*TRIPLEITERATORTEST_*/
