// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CrossCorrelate.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

/* Trying to add minimal working set of function libraries - whittle them down */
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/System.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
/* end of minimal working set */

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;

class CrossCorrelateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being
  // created statically This means the constructor isn't called when
  // running other tests
  static CrossCorrelateTest *createSuite() { return new CrossCorrelateTest(); }
  static void destroySuite(CrossCorrelateTest *suite) { delete suite; }

  void dump_vector( std::vector< double > const &v, std::string header_line )
  {
    std::cout << header_line << std::endl;
    for( double i : v )
    {
      std::cout << " " << i;
    }
    std::cout << std::endl;
  }

  /* parameterize everything */
  /* Updated workflow */
  void test_sanity_0()
  {
    /* Create a composite function */
    /* parameterize Height and Sigma with default values in that string */
    auto function = std::make_shared<API::CompositeFunction>();
    function->addFunction( 
      std::dynamic_pointer_cast<IPeakFunction>(
        API::FunctionFactory::Instance()
        .createInitialized("name=Gaussian,Height=10,Sigma=4")));
    
    /* create the workspace */
    int const domain_radius = 10;

    std::vector< double > symmetric_domain( domain_radius * 2 + 1 );

    int num_signals = 4;

    const MatrixWorkspace_sptr workspace =
    createWorkspace<Workspace2D>( num_signals, symmetric_domain.size(), symmetric_domain.size() );

    /* populate initial domain */
    std::iota( symmetric_domain.begin(), symmetric_domain.end(), -1 * domain_radius );
    FunctionDomain1DVector domain( std::move( symmetric_domain ) );
    /* move after clear */
    symmetric_domain.clear();

    /* populate range */
    FunctionValues values( domain );
    function->function( domain, values );

    /* destroy FunctionValues object and get underlying data primitive */
    auto co_domain = std::move( values.toVector() );

    /* destroy FunctionDomain1DVector and get underlying data primitive */
    symmetric_domain = std::move( domain.getVector() );

    dump_vector( symmetric_domain, "domain:" );
    dump_vector( co_domain, "co-domain:" );

    /* HistogramX - rawData() gets const ref to the vector, mutableX() gets assignable vector */
    auto &x_data = workspace->mutableX( 0 );
    x_data.assign( symmetric_domain.cbegin(), symmetric_domain.cend() );

    /* HistogramY */
    auto &y_data = workspace->mutableY( 0 );
    y_data.assign( co_domain.cbegin(), co_domain.cend() );

    /* apply domain modifications */
    /* adjust the symmetric domain */
    int translation = 5;
    std::vector< double > translated_domain;
    std::transform( symmetric_domain.begin(),
                    symmetric_domain.end(),
                    std::back_inserter( translated_domain ),
                    [ translation ]( double d ) -> double { return d + translation; } );
    
    /* calculate function values and put in the workspace */

    /* apply domain scaling */
    int scale_factor = 5;
    std::vector< double > scaled_domain;
    std::transform( symmetric_domain.begin(),
                    symmetric_domain.end(),
                    std::back_inserter( scaled_domain ),
                    [ scale_factor ]( double d ) -> double { return scale_factor * d; } );

    /* apply co-domain scaling - then define a class that will handle all this and sets up
       defaults */
    workspace->mutableX( 1 ) = workspace->mutableX( 0 );
    workspace->mutableY( 1 ) = scale_factor * workspace->mutableY( 0 );

    /* Captain! Get it out of there and check that the values look right */
    auto &data_codomain_scaled = workspace->mutableY( 1 ).rawData();
    auto &data_domain_scaled = workspace->mutableX( 1 ).rawData();
    dump_vector( data_domain_scaled, "scaled domain:" );
    dump_vector( data_codomain_scaled, "scaled co-domain:" );

    /* create new HistogramY and HistogramX and put them in the workspace */

    /* what will the class need to do? 
     * initialize a function object - parameter string
     * initialize a workspace - number of workspaces inferred from initializer list length
       iterate through mod objects, set them according to the operation specified, skip
       the reference. Make note of the reference so you can get the HistogramY
     * define a modify object and a scoped enum to have type and value
     * define a function that handles each type of mod object possibility
     * define a function that takes in a vector domain and returns a vector codomain
     * constructor takes in a default-valued initializer list of mod objects?
       */

    TS_ASSERT_EQUALS( 0, 1 );
  }

  /* create raw & gaussian workspaces, get stuff out of them */
  /* initial workflow */
  void test_sanity()
  {
    /* function will extend domain_radius in positive and negative directions from 0 */
    int const domain_radius = 10;
    /* add one for the origin - change name to symmetric range */
    std::vector< double > domain( domain_radius * 2 + 1 );
    std::iota( domain.begin(), domain.end(), -1 * domain_radius );

    /* matrix workspace creation */
    int num_signals = 4;
    const MatrixWorkspace_sptr workspace =
    createWorkspace<Workspace2D>( num_signals, domain.size(), domain.size() );

    /* HistogramX */
    auto &x_data = workspace->mutableX( 0 );

    /* HistogramY */
    auto &y_data = workspace->mutableY( 0 );
    
    /* Old code below - use new code above */

    /* Create a function - specify parameter string too? */
    API::IPeakFunction_sptr gaussian_function =
      std::dynamic_pointer_cast<IPeakFunction>(
        API::FunctionFactory::Instance().createFunction("Gaussian"));

    bool test = gaussian_function == nullptr;


    /* create domain and allocate co-domain */
    FunctionDomain1DVector domain_embedded( domain.cbegin(), domain.cend() );
    FunctionValues values( domain_embedded );

    /* create function */
    auto function_embedded = std::make_shared<API::CompositeFunction>();
    function_embedded->addFunction( gaussian_function );

    /* compute co-domain */
    function_embedded->function( domain_embedded, values );

    /* Does this need to be point data or bin data? Assuming point because easier... */

    x_data.assign( domain.cbegin(), domain.cend() );
    std::vector< double > values_raw = values.toVector();
    y_data.assign( values_raw.cbegin(), values_raw.cend() );

    /* shift and scale domain and recompute */
    double scale = 2;
    double shift = 2;
    std::transform( domain.begin(),
                    domain.end(),
                    domain.begin(),
                    [ scale, shift ]( double x ) -> double
                    {
                      return x * scale + shift;
                    } );

    FunctionDomain1DVector domain_modified( domain.cbegin(), domain.cend() );

    function_embedded->function( domain_modified, values );

    /* Assign to a matrix workspace */

    /* shift and scale codomain */
    /* Assign to the matrix workspace */
    /* Make all of this object oriented */
    TS_ASSERT_EQUALS( values[0], values[0] );
  }

  void testValidInput() {
    // setup and run the algorithm (includes basic checks)
    CrossCorrelate alg;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, 2.0, 4.0);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(alg, inWS);

    // specific checks
    const MantidVec &outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 3);
    TS_ASSERT_DELTA(outX[0], -1.0, 1e-6);
    TS_ASSERT_DELTA(outX[1], 0.0, 1e-6);
    TS_ASSERT_DELTA(outX[2], 1.0, 1e-6);

    const MantidVec &outY0 = outWS->readY(0);
    TS_ASSERT_EQUALS(outY0.size(), 3);
    TS_ASSERT_DELTA(outY0[0], -0.018902, 1e-6);
    TS_ASSERT_DELTA(outY0[1], 1.0, 1e-6);
    TS_ASSERT_DELTA(outY0[2], -0.018902, 1e-6);

    const MantidVec &outY1 = outWS->readY(1);
    TS_ASSERT_EQUALS(outY1.size(), 3);
    TS_ASSERT_DELTA(outY1[0], -0.681363, 1e-6);
    TS_ASSERT_DELTA(outY1[1], 0.168384, 1e-6);
    TS_ASSERT_DELTA(outY1[2], 0.456851, 1e-6);
  }

  // This tests an input X length of 3, which is the minimum the algorithm can
  // handle
  void testMinimumInputXLength() {
    // setup and run the algorithm (includes basic checks)
    CrossCorrelate alg;
    const MatrixWorkspace_const_sptr inWS = setupAlgorithm(alg, 2.0, 3.5);
    const MatrixWorkspace_const_sptr outWS = runAlgorithm(alg, inWS);

    // specific checks
    const MantidVec &outX = outWS->readX(0);
    TS_ASSERT_EQUALS(outX.size(), 1);
    TS_ASSERT_DELTA(outX[0], 0.0, 1e-6);

    const MantidVec &outY0 = outWS->readY(0);
    TS_ASSERT_EQUALS(outY0.size(), 1);
    TS_ASSERT_DELTA(outY0[0], 1.0, 1e-6);

    const MantidVec &outY1 = outWS->readY(1);
    TS_ASSERT_EQUALS(outY1.size(), 1);
    TS_ASSERT_DELTA(outY1[0], -1.0, 1e-6);
  }

  void testInputXLength2() {
    // this throws because at least 3 X values are required
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 3.0);
    runAlgorithmThrows(alg);
  }

  void testInputXLength1() {
    // this throws because at least 3 X values are required
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 2.4);
    runAlgorithmThrows(alg);
  }

  void testXMinEqualsXMax() {
    // this throws because XMin should be > XMax
    CrossCorrelate alg;
    setupAlgorithm(alg, 2.0, 2.0);
    runAlgorithmThrows(alg);
  }

  void testXMinGreaterThanXMax() {
    // this throws because XMin should be < XMax
    CrossCorrelate alg;
    setupAlgorithm(alg, 3.0, 2.0);
    runAlgorithmThrows(alg);
  }

private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the x and e values
    const int nBins = 10;
    BinEdges xValues(nBins + 1, HistogramData::LinearGenerator(0.0, 0.5));
    CountStandardDeviations e1(nBins, sqrt(3.0));

    // for y use a gaussian peak centred at 2.5 with height=10
    const double sigmaSq = 0.7 * 0.7;
    std::vector<double> yValues(nBins);
    for (size_t j = 0; j < nBins; ++j) {
      yValues[j] = 0.3 + 10.0 * exp(-0.5 * pow(xValues[j] - 2.5, 2.0) / sigmaSq);
    }

    /* Captain! */

    // create the workspace
    const int nHist = 2;
    const MatrixWorkspace_sptr ws = createWorkspace<Workspace2D>(nHist, nBins + 1, nBins);
    ws->getAxis(0)->setUnit("dSpacing");

    for (size_t i = 0; i < nHist; ++i) {
      ws->setBinEdges(i, xValues);
      ws->mutableY(i) = yValues;
      ws->setCountStandardDeviations(i, e1);

      // offset the x values for the next spectrum
      xValues += 0.5;
    }

    return ws;
  }

  // Initialise the algorithm and set the properties. Creates a fake workspace
  // for the input and returns it.
  MatrixWorkspace_const_sptr setupAlgorithm(CrossCorrelate &alg, const double xmin, const double xmax) {

    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "outWS");
    alg.setProperty("ReferenceSpectra", 0);
    alg.setProperty("WorkspaceIndexMin", 0);
    alg.setProperty("WorkspaceIndexMax", 1);
    alg.setProperty("XMin", xmin);
    alg.setProperty("XMax", xmax);

    return inWS;
  }

  // Run the algorithm and do some basic checks. Returns the output workspace.
  MatrixWorkspace_const_sptr runAlgorithm(CrossCorrelate &alg, const MatrixWorkspace_const_sptr &inWS) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // verify the output workspace
    const MatrixWorkspace_const_sptr outWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(inWS->getNumberHistograms(),
                     outWS->getNumberHistograms()); // shouldn't drop histograms

    return outWS;
  }

  // Run the algorithm with invalid input and check that it throws a runtime
  // error
  void runAlgorithmThrows(CrossCorrelate &alg) { TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &); }
};
