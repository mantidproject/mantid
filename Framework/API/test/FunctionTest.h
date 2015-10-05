#ifndef IFUNCTIONTEST_H_
#define IFUNCTIONTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ParameterTie.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

class MocSpectrum : public ISpectrum {
public:
  MocSpectrum(size_t nx, size_t ny) : m_x(nx), m_y(ny), m_e(ny) {}

  void clearData() {}
  void setData(const MantidVec &) {}
  void setData(const MantidVec &, const MantidVec &) {}

  void setData(const MantidVecPtr &) {}
  void setData(const MantidVecPtr &, const MantidVecPtr &) {}

  void setData(const MantidVecPtr::ptr_type &) {}
  void setData(const MantidVecPtr::ptr_type &, const MantidVecPtr::ptr_type &) {
  }

  virtual MantidVec &dataX() { return m_x; }
  virtual MantidVec &dataY() { return m_y; }
  virtual MantidVec &dataE() { return m_e; }

  virtual const MantidVec &dataX() const { return m_x; }
  virtual const MantidVec &dataY() const { return m_y; }
  virtual const MantidVec &dataE() const { return m_e; }

  virtual size_t getMemorySize() const { return 0; }

  MantidVec m_x, m_y, m_e;
};

class MocMatrixWorkspace : public MatrixWorkspace {
public:
  MocMatrixWorkspace(size_t nspec, size_t nx, size_t ny) : m_blocksize(ny) {
    for (size_t i = 0; i < nspec; ++i) {
      MocSpectrum sp(nx, ny);
      m_spectra.push_back(sp);
    }
  }

  ~MocMatrixWorkspace() {}

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  virtual std::size_t size() const { return m_spectra.size() * m_blocksize; }
  /// Returns the size of each block of data returned by the dataY accessors
  virtual std::size_t blocksize() const { return m_blocksize; }
  /// Returns the number of histograms in the workspace
  virtual std::size_t getNumberHistograms() const { return m_spectra.size(); }

  /// Return the underlying ISpectrum ptr at the given workspace index.
  virtual ISpectrum *getSpectrum(const size_t index) {
    return &m_spectra[index];
  }

  /// Return the underlying ISpectrum ptr (const version) at the given workspace
  /// index.
  virtual const ISpectrum *getSpectrum(const size_t index) const {
    return &m_spectra[index];
  }
  const std::string id(void) const { return ""; }
  void init(const size_t &, const size_t &, const size_t &) {}

private:
  std::vector<MocSpectrum> m_spectra;
  size_t m_blocksize;
};

class IFT_Funct : public ParamFunction, public IFunction1D {
public:
  IFT_Funct() {
    declareParameter("c0", 0.0, "this is the famous c0 blah...");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name() const { return "IFT_Funct"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out[i] = c0 + x * (c1 + x * (c2 + x * c3));
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) {
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out->set(i, 0, 1.);
      out->set(i, 1, x);
      out->set(i, 2, x * x);
      out->set(i, 3, x * x * x);
    }
  }
};

class FunctionTest : public CxxTest::TestSuite {
public:
  void testIFunction() {
    IFT_Funct f;

    TS_ASSERT_EQUALS(f.parameterDescription(0),
                     "this is the famous c0 blah...");

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    TS_ASSERT_EQUALS(f.parameterDescription(0),
                     "this is the famous c0 blah...");

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.getParameter(0), 1.0);
    TS_ASSERT_EQUALS(f.getParameter(1), 1.1);
    TS_ASSERT_EQUALS(f.getParameter(2), 1.2);
    TS_ASSERT_EQUALS(f.getParameter(3), 1.3);

    TS_ASSERT_EQUALS(f.parameterName(0), "c0");
    TS_ASSERT_EQUALS(f.parameterName(1), "c1");
    TS_ASSERT_EQUALS(f.parameterName(2), "c2");
    TS_ASSERT_EQUALS(f.parameterName(3), "c3");

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 1.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 1.3);

    TS_ASSERT_EQUALS(f.parameterIndex("c0"), 0);
    TS_ASSERT_EQUALS(f.parameterIndex("c1"), 1);
    TS_ASSERT_EQUALS(f.parameterIndex("c2"), 2);
    TS_ASSERT_EQUALS(f.parameterIndex("c3"), 3);

    std::string str = "name=IFT_Funct,c0=1,c1=1.1,c2=1.2,c3=1.3";

    TS_ASSERT_EQUALS(f.asString(), str);

    TS_ASSERT_EQUALS(f.activeParameter(0), 1.0);
    TS_ASSERT_EQUALS(f.activeParameter(1), 1.1);
    TS_ASSERT_EQUALS(f.activeParameter(2), 1.2);
    TS_ASSERT_EQUALS(f.activeParameter(3), 1.3);

    TS_ASSERT_EQUALS(f.nameOfActive(0), "c0");
    TS_ASSERT_EQUALS(f.nameOfActive(1), "c1");
    TS_ASSERT_EQUALS(f.nameOfActive(2), "c2");
    TS_ASSERT_EQUALS(f.nameOfActive(3), "c3");

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(!f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(!f.isFixed(3));
  }

  void testFix() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.fix(1);
    f.fix(3);

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.activeParameter(0), 1.0);
    TS_ASSERT_EQUALS(f.activeParameter(2), 1.2);

    TS_ASSERT_EQUALS(f.nameOfActive(0), "c0");
    TS_ASSERT_EQUALS(f.nameOfActive(2), "c2");

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(f.isFixed(3));

    TS_ASSERT_THROWS(f.activeParameter(1), std::runtime_error);
    TS_ASSERT_THROWS(f.activeParameter(3), std::runtime_error);
    TS_ASSERT_THROWS(f.setActiveParameter(1, 0), std::runtime_error);
    TS_ASSERT_THROWS(f.setActiveParameter(3, 0), std::runtime_error);
  }

  void testUnfix() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.fix(1);
    f.fix(3);

    f.unfix(3);

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.activeParameter(0), 1.0);
    TS_ASSERT_EQUALS(f.activeParameter(2), 1.2);
    TS_ASSERT_EQUALS(f.activeParameter(3), 1.3);

    TS_ASSERT_EQUALS(f.nameOfActive(0), "c0");
    TS_ASSERT_EQUALS(f.nameOfActive(2), "c2");
    TS_ASSERT_EQUALS(f.nameOfActive(3), "c3");

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(!f.isFixed(3));
  }

  void testSetActiveParameter() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.fix(1);
    f.fix(3);

    TS_ASSERT_EQUALS(f.nParams(), 4);

    f.setActiveParameter(0, 2.0);
    f.setActiveParameter(2, 2.1);

    TS_ASSERT_EQUALS(f.activeParameter(0), 2.0);
    TS_ASSERT_EQUALS(f.activeParameter(2), 2.1);

    TS_ASSERT_EQUALS(f.getParameter(0), 2.0);
    TS_ASSERT_EQUALS(f.getParameter(1), 1.1);
    TS_ASSERT_EQUALS(f.getParameter(2), 2.1);
    TS_ASSERT_EQUALS(f.getParameter(3), 1.3);

    TS_ASSERT_EQUALS(f.getParameter("c0"), 2.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 1.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 2.1);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 1.3);
  }

  void testTie() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.tie("c1", "0");
    f.tie("c3", "0");

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.activeParameter(0), 1.0);
    TS_ASSERT_EQUALS(f.activeParameter(2), 1.2);

    TS_ASSERT_EQUALS(f.nameOfActive(0), "c0");
    TS_ASSERT_EQUALS(f.nameOfActive(2), "c2");

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(f.isFixed(3));

    TS_ASSERT(f.isActive(0));
    TS_ASSERT(!f.isActive(1));
    TS_ASSERT(f.isActive(2));
    TS_ASSERT(!f.isActive(3));

    TS_ASSERT(!f.getTie(0));
    TS_ASSERT(f.getTie(1) && !f.getTie(1)->isDefault());
    TS_ASSERT(!f.getTie(2));
    TS_ASSERT(f.getTie(3) && !f.getTie(3)->isDefault());
  }

  void testApplyTies() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.tie("c1", "c0+4");
    f.tie("c3", "c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 0.6);
  }

  void testRemoveTie() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.tie("c1", "c0+4");
    f.tie("c3", "c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 0.6);

    f.removeTie("c3");
    f.setParameter("c3", 3.3);

    f.applyTies();

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 3.3);

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(!f.isFixed(3));

    TS_ASSERT(!f.getTie(0));
    TS_ASSERT(f.getTie(1) && !f.getTie(1)->isDefault());
    TS_ASSERT(!f.getTie(2));
    TS_ASSERT(!f.getTie(3));
  }

  void testClearTies() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    f.setParameter("c2", 1.2);
    f.setParameter("c3", 1.3);

    f.tie("c1", "c0+4");
    f.tie("c3", "c2/2");

    f.applyTies();

    TS_ASSERT_EQUALS(f.nParams(), 4);

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 5.0);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 0.6);

    f.clearTies();
    f.setParameter("c1", 3.1);
    f.setParameter("c3", 3.3);

    f.applyTies();

    TS_ASSERT_EQUALS(f.getParameter("c0"), 1.0);
    TS_ASSERT_EQUALS(f.getParameter("c1"), 3.1);
    TS_ASSERT_EQUALS(f.getParameter("c2"), 1.2);
    TS_ASSERT_EQUALS(f.getParameter("c3"), 3.3);

    TS_ASSERT(!f.isFixed(0));
    TS_ASSERT(!f.isFixed(1));
    TS_ASSERT(!f.isFixed(2));
    TS_ASSERT(!f.isFixed(3));
  }

  void testExplicitlySet() {
    IFT_Funct f;

    f.setParameter("c0", 1.0);
    f.setParameter("c1", 1.1);
    TS_ASSERT(f.isExplicitlySet(0));
    TS_ASSERT(f.isExplicitlySet(1));
    TS_ASSERT(!f.isExplicitlySet(2));
    TS_ASSERT(!f.isExplicitlySet(3));
  }

  // void xtest_setWorkspace_works()
  //{
  //  MatrixWorkspace_sptr ws(new MocMatrixWorkspace(10,11,10));

  //  MantidVec& x = ws->dataX(3);
  //  MantidVec& y = ws->dataY(3);
  //  for(size_t i=0; i < y.size(); ++i)
  //  {
  //    x[i] = 0.1 * static_cast<double>(i);
  //    y[i] = static_cast<double>(i);
  //  }
  //  x.back() = 0.1 * static_cast<double>(y.size());
  //  AnalysisDataService::Instance().add("IFT_Test_WS",ws);
  //  IFT_Funct f;
  //  TS_ASSERT_THROWS_NOTHING(f.setWorkspace(ws,"WorkspaceIndex=3,StartX=0.2,EndX
  //  = 0.8",true));
  //  TS_ASSERT_EQUALS(f.dataSize(),8);
  //  TS_ASSERT_EQUALS(f.getData(),&y[2]);
  //  AnalysisDataService::Instance().remove("IFT_Test_WS");
  //}

  /** Refs #3003: Test to make sure setMatrix works in parallel */
  // void xtest_setWorkspace_works_inParallel()
  //{
  //  double expected;
  //  int numpixels = 15000;
  //  MatrixWorkspace_sptr ws(new MocMatrixWorkspace(numpixels,11,10));
  //  for (size_t wi=0; wi<ws->getNumberHistograms(); wi++)
  //  {
  //    MantidVec& x = ws->dataX(wi);
  //    MantidVec& y = ws->dataY(wi);
  //    for(size_t i=0; i < y.size(); ++i)
  //    {
  //      x[i] = 0.1 * double(i);
  //      y[i] = double(i);
  //    }
  //    x.back() = 0.1 * double(y.size());
  //    expected = y[2];
  //  }

  //  // NOTE: In parallel, there is a segfault on SNS build servers
  //  ubuntu-10.04 and RHEL6. The rest pass!?
  //  PARALLEL_FOR_NO_WSP_CHECK()
  //  for (int i=0; i<numpixels; i++)
  //  {
  //    IFT_Funct f;
  //    std::stringstream mess;
  //    mess << "WorkspaceIndex=" << i << ",StartX=0.2,EndX = 0.8";
  //    TS_ASSERT_THROWS_NOTHING(f.setWorkspace(ws, mess.str(),true ));
  //    TS_ASSERT_EQUALS(f.dataSize(),8);
  //    TS_ASSERT_EQUALS(*f.getData(), expected);
  //  }
  //}
};

#endif /*IFUNCTIONTEST_H_*/
