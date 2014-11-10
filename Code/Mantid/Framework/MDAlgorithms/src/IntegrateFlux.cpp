#include "MantidMDAlgorithms/IntegrateFlux.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace MDAlgorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(IntegrateFlux)

  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string IntegrateFlux::name() const { return "IntegrateFlux"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int IntegrateFlux::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string IntegrateFlux::category() const { return "MDAlgorithms";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string IntegrateFlux::summary() const { return "Interates spectra in a matrix workspace at a set of points.";};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void IntegrateFlux::init()
  {
    declareProperty(new WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    auto validator = boost::make_shared<Kernel::BoundedValidator<int>>();
    validator->setLower(2);
    declareProperty("NPoints", 1000, validator, "Number of points per output spectrum.");
    declareProperty(new WorkspaceProperty<API::Workspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void IntegrateFlux::exec()
  {
    DataObjects::EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
    size_t nX = static_cast<size_t>( (int)getProperty("NPoints") );

    auto outputWS = createOutputWorkspace( *inputWS, nX );

    integrateSpectra( *inputWS, *outputWS );

    setProperty("OutputWorkspace",outputWS);
  }

  /**
   * Create an empty output workspace with required dimensions and defined x-values
   * @param eventWS :: The input event workspace.
   * @param nX :: Suggested size of the output spectra. It can change in the actual output.
   */
  boost::shared_ptr<API::MatrixWorkspace> IntegrateFlux::createOutputWorkspace( const DataObjects::EventWorkspace& eventWS, size_t nX ) const
  {
    size_t nSpec = eventWS.getNumberHistograms();

    if ( nSpec == 0 )
    {
      throw std::runtime_error("Input workspace has no data.");
    }

    // make sure the output spectrum size isn't too large
    auto nEvents = eventWS.getEventList(0).getNumberEvents();
    if ( nX > nEvents )
    {
      nX = nEvents;
    }

    // and not 0 or 1 as they are to be used for interpolation
    if ( nX < 2 )
    {
      throw std::runtime_error("Failed to create output."
        "Output spectra should have at least two points.");
    }

    // crate empty output workspace
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create( "Workspace2D", nSpec, nX, nX );

    // claculate the integration points and save them in the x-vactors of integrFlux
    double xMin = eventWS.getEventXMin();
    double xMax = eventWS.getEventXMax();
    double dx = ( xMax - xMin ) / static_cast<double>( nX - 1 );
    auto &X = ws->dataX(0);
    auto ix = X.begin();
    // x-values are equally spaced between the min and max tof in the first flux spectrum
    for(double x = xMin; ix != X.end(); ++ix, x += dx)
    {
      *ix = x;
    }

    // share the xs for all spectra
    auto xRef = ws->refX(0);
    for(size_t sp = 1; sp < nSpec; ++sp)
    {
      ws->setX(sp,xRef);
    }

    return ws;
  }

  /**
   * Integrate spectra in eventWS at x-values in integrWS and save the results in y-vectors of integrWS.
   * @param eventWS :: A workspace to integrate. The events have to be weighted-no-time.
   * @param integrWS :: A workspace to store the results.
   */
  void IntegrateFlux::integrateSpectra( const DataObjects::EventWorkspace& eventWS, API::MatrixWorkspace &integrWS )
  {
    size_t nSpec = eventWS.getNumberHistograms();
    assert( nSpec == integrWS.getNumberHistograms() );

    auto &X = integrWS.readX(0);
    // loop overr the spectra and integrate
    for(size_t sp = 0; sp < nSpec; ++sp)
    {
      std::vector<Mantid::DataObjects::WeightedEventNoTime> el = eventWS.getEventList(sp).getWeightedEventsNoTime();
      auto &outY = integrWS.dataY(sp);
      double sum = 0;
      auto x = X.begin() + 1;
      size_t i = 1;
      // the integral is a running sum of the event weights in the spectrum
      for(auto evnt = el.begin(); evnt != el.end(); ++evnt)
      {
        double tof = evnt->tof();
        while( x != X.end() && *x < tof )
        {
          outY[i] = sum;
          ++x; ++i;
        }
        if ( x == X.end() ) break;
        sum += evnt->weight();
        outY[i] = sum;
      }
    }
  }

} // namespace MDAlgorithms
} // namespace Mantid