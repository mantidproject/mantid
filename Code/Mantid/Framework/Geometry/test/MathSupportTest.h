#ifndef MANTID_TESTMATHSUPPORT__
#define MANTID_TESTMATHSUPPORT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Math/mathSupport.h"


using namespace Mantid;
using namespace mathSupport;
class testMathSupport: public CxxTest::TestSuite
{
public:
	void testIndexSort(){
		//Test double
		std::vector<double> udArray;
		std::vector<int> sdArray,cdArray;
		udArray.push_back(3.3);
		udArray.push_back(4.4);
		udArray.push_back(2.2);
		udArray.push_back(5.5);
		udArray.push_back(1.1);
		udArray.push_back(5.4999999999999999999);
		indexSort(udArray,sdArray);
		cdArray.push_back(4);
		cdArray.push_back(2);
		cdArray.push_back(0);
		cdArray.push_back(1);
		cdArray.push_back(3);
		cdArray.push_back(5);
		TS_ASSERT_EQUALS(sdArray,cdArray);

		std::vector<int> uiArray;
		std::vector<int> siArray,ciArray;
		uiArray.push_back(3);
		uiArray.push_back(4);
		uiArray.push_back(2);
		uiArray.push_back(5);
		uiArray.push_back(1);
		indexSort(uiArray,siArray);
		ciArray.push_back(4);
		ciArray.push_back(2);
		ciArray.push_back(0);
		ciArray.push_back(1);
		ciArray.push_back(3);
		TS_ASSERT_EQUALS(siArray,ciArray);

		std::vector<int> uzArray,szArray;
		indexSort(uzArray,szArray);
		TS_ASSERT_EQUALS(uzArray,szArray);
	}

	void testSolveQuadratic(){ //Test quadratic solution
		double xp1w2[3]={1,2,1}; //(x+1)^2 one solution
		std::pair<std::complex<double>,std::complex<double> > output;
		TS_ASSERT_EQUALS(solveQuadratic(xp1w2,output),1);
		std::pair<std::complex<double>,std::complex<double> > result(std::complex<double>(-1.0,0.0),std::complex<double>(-1.0,0.0));
		TS_ASSERT_EQUALS(output,result);

		double test2[3]={1,0,-1};  //two rational roots
		TS_ASSERT_EQUALS(solveQuadratic(test2,output),2);
		std::pair<std::complex<double>,std::complex<double> > result2(std::complex<double>(-1.0,0.0),std::complex<double>(1.0,0.0));
		TS_ASSERT_EQUALS(output,result2);


		double test3[3]={1,0,1}; //two complex roots
		TS_ASSERT_EQUALS(solveQuadratic(test3,output),2);
		std::pair<std::complex<double>,std::complex<double> > result3(std::complex<double>(0.0,-1.0),std::complex<double>(0.0,1.0));
		TS_ASSERT_EQUALS(output,result3);
	}

	void testSolveCubic(){
		double test[4]={1.0,6.0,-4.0,-24.0};
		std::complex<double> root1,root2,root3;
		TS_ASSERT_EQUALS(solveCubic(test,root1,root2,root3),3);
		TS_ASSERT_DELTA(root1.real(),-6  ,0.0000001);
		TS_ASSERT_DELTA(root2.real(), 2  ,0.000001);
		TS_ASSERT_DELTA(root3.real(),-2.0,0.000001);
		TS_ASSERT_DELTA(root1.imag(),   0,0.0000001);
		TS_ASSERT_DELTA(root2.imag(),   0,0.000001);
		TS_ASSERT_DELTA(root3.imag(), 0.0,0.000001);	

		double test2[4]={1.0,-11.0,49.0,-75.0};
		TS_ASSERT_EQUALS(solveCubic(test2,root1,root2,root3),3);
		TS_ASSERT_DELTA(root1.real(),   3,0.0000001);
		TS_ASSERT_DELTA(root2.real(),   4,0.000001);
		TS_ASSERT_DELTA(root3.real(),   4,0.000001);
		TS_ASSERT_DELTA(root1.imag(),   0,0.0000001);
		TS_ASSERT_DELTA(root2.imag(),   3,0.000001);
		TS_ASSERT_DELTA(root3.imag(),  -3,0.000001);	
	}

};
#endif
