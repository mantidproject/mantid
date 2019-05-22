// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATORTEST_H_
#define MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/FunctionDomain1DSpectrumCreator.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::FunctionDomain1DSpectrumCreator;

using namespace Mantid::API;

class FunctionDomain1DSpectrumCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionDomain1DSpectrumCreatorTest *createSuite() {
    return new FunctionDomain1DSpectrumCreatorTest();
  }
  static void destroySuite(FunctionDomain1DSpectrumCreatorTest *suite) {
    delete suite;
  }

  void testInstantiation() {
    TS_ASSERT_THROWS_NOTHING(FunctionDomain1DSpectrumCreator creator;)
  }

  void testDefaultConstructor() {
    TestableFunctionDomain1DSpectrumCreator creator;

    TS_ASSERT_EQUALS(creator.m_workspaceIndexIsSet, false);
    TS_ASSERT(!creator.m_matrixWorkspace);
    TS_ASSERT_THROWS(creator.getDomainSize(), const std::invalid_argument &);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    TS_ASSERT_THROWS(creator.createDomain(domain, values),
                     const std::invalid_argument &);
  }

  void testSetWorkspaceIndex() {
    TestableFunctionDomain1DSpectrumCreator creator;

    creator.setWorkspaceIndex(10);

    TS_ASSERT_EQUALS(creator.m_workspaceIndex, 10);
    TS_ASSERT_EQUALS(creator.m_workspaceIndexIsSet, true);
  }

  void testSetMatrixWorkspace() {
    TestableFunctionDomain1DSpectrumCreator creator;

    MatrixWorkspace_sptr matrixWs =
        WorkspaceCreationHelper::create2DWorkspace123(10, 15);
    creator.setMatrixWorkspace(matrixWs);

    TS_ASSERT_EQUALS(creator.m_matrixWorkspace->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(creator.m_matrixWorkspace->readX(0).size(), 15);
    TS_ASSERT_EQUALS(creator.m_matrixWorkspace->readX(0)[0], 1.0);
    TS_ASSERT_EQUALS(creator.m_matrixWorkspace->readX(4)[0], 1.0);
  }

  void testThrowIfWorkspaceInvalid() {
    TestableFunctionDomain1DSpectrumCreator creator;

    // throws, because workspace and index are not set.
    TS_ASSERT_THROWS(creator.throwIfWorkspaceInvalid(), const std::invalid_argument &);

    creator.setMatrixWorkspace(
        WorkspaceCreationHelper::create2DWorkspace123(10, 15));
    // still throws, since workspace index has not been set explicitly.
    TS_ASSERT_THROWS(creator.throwIfWorkspaceInvalid(), const std::invalid_argument &);

    creator.setWorkspaceIndex(4);
    TS_ASSERT_THROWS_NOTHING(creator.throwIfWorkspaceInvalid());

    creator.setWorkspaceIndex(34);
    // throws also, because index is invalid
    TS_ASSERT_THROWS(creator.throwIfWorkspaceInvalid(), const std::invalid_argument &);
  }

  void testGetDomainSize() {
    FunctionDomain1DSpectrumCreator creator;
    creator.setMatrixWorkspace(
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 5, 0.0, 1.0));
    creator.setWorkspaceIndex(0);

    TS_ASSERT_EQUALS(creator.getDomainSize(), 5);

    creator.setMatrixWorkspace(
        WorkspaceCreationHelper::create2DWorkspace123(1, 15));

    TS_ASSERT_EQUALS(creator.getDomainSize(), 15);
  }

  void testCreateDomain() {
    TestableFunctionDomain1DSpectrumCreator creator;
    creator.setMatrixWorkspace(
        WorkspaceCreationHelper::create2DWorkspace123(1, 5));
    creator.setWorkspaceIndex(0);

    FunctionDomain_sptr domain;
    FunctionValues_sptr values;

    creator.createDomain(domain, values);

    TS_ASSERT(domain);
    TS_ASSERT(values);

    boost::shared_ptr<FunctionDomain1DSpectrum> spectrum =
        boost::dynamic_pointer_cast<FunctionDomain1DSpectrum>(domain);

    TS_ASSERT(spectrum);
    TS_ASSERT_EQUALS(spectrum->getWorkspaceIndex(), 0);
    TS_ASSERT_EQUALS(spectrum->size(), 5);
  }

private:
  class TestableFunctionDomain1DSpectrumCreator
      : public FunctionDomain1DSpectrumCreator {
    friend class FunctionDomain1DSpectrumCreatorTest;

  public:
    TestableFunctionDomain1DSpectrumCreator()
        : FunctionDomain1DSpectrumCreator() {}
    ~TestableFunctionDomain1DSpectrumCreator() override {}
  };
};

#endif /* MANTID_CURVEFITTING_FUNCTIONDOMAIN1DSPECTRUMCREATORTEST_H_ */
