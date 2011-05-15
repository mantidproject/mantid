#ifndef H_TEST_IMD_DIMENSION
#define H_TEST_IMD_DIMENSION

#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>


class IMDDimensionReal: public Mantid::Geometry::IMDDimension
{
public:
	
	virtual std::string getName()       const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return "";}
    virtual std::string getUnits()      const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return "";}
    virtual std::string getDimensionId()const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return "";}
    virtual std::string toXMLString() const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return "";};

    virtual double getMaximum()    const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return 0;}
    virtual double getMinimum()    const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return 0;}
    virtual size_t getNBins()      const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return 0;}
    virtual double getX(size_t /*ind*/)const{throw(Mantid::Kernel::Exception::NotImplementedError(""));return 0;}

//    virtual bool getIsIntegrated() const -- should trhow not-implemented through getNbins;
 
};
//
class IMDDimensionTest : public CxxTest::TestSuite
{
public:
	void testGetName(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getName(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetUnits(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getUnits(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetDimensionId(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getDimensionId(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testToXMLString(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->toXMLString(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetMaximum(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getMaximum(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetMinimum(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getMinimum(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetNBins(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getNBins(),Mantid::Kernel::Exception::NotImplementedError);
	}
	void testGetX(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getX(0),Mantid::Kernel::Exception::NotImplementedError);
	}
//  ISIS methods;
 	void testGetDataShift(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getDataShift(),std::runtime_error);
	}
	void testGetStride(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getStride(),std::runtime_error);
	}
	void testIsReciprocal(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->isReciprocal(),std::runtime_error);
	}
	void testGetDirection(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getDirection(),std::runtime_error);
	}
	void testGetDirectionCryst(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(pDim->getDirectionCryst(),std::runtime_error);
	}
	void testGetAxisPoints(){
		std::auto_ptr<IMDDimensionReal> pDim = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		std::vector<double> rez;
		TS_ASSERT_THROWS(pDim->getAxisPoints(rez),std::runtime_error);
	}
	void testDimEqual(){
		std::auto_ptr<IMDDimensionReal> pDim1 = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
	    std::auto_ptr<IMDDimensionReal> pDim2 = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(*pDim1==*pDim2,std::runtime_error);
	}
	void testDimNEqual(){
		std::auto_ptr<IMDDimensionReal> pDim1 = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
	    std::auto_ptr<IMDDimensionReal> pDim2 = std::auto_ptr<IMDDimensionReal>(new IMDDimensionReal());
		TS_ASSERT_THROWS(*pDim1!=*pDim2,std::runtime_error);
	}

};

#endif
