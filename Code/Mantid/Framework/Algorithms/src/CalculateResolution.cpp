#include "MantidAlgorithms/CalculateResolution.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"

#include <cmath>

#include <boost/shared_ptr.hpp>

namespace
{

  using namespace Mantid::API;
  using namespace Mantid::Geometry;

  boost::shared_ptr<const IComponent> getComponent(MatrixWorkspace_sptr ws, const std::string& comp)
  {
    if(!ws)
      throw std::runtime_error("Invalid workspace.");

    auto instrument = ws->getInstrument();

    if(!instrument)
      throw std::runtime_error("Could not fetch workspace's instrument.");

    auto components = instrument->getAllComponentsWithName(comp);
    if(components.size() < 1)
      throw std::runtime_error("Instrument has no component named \"" + comp + "\"");

    return components.front();
  }
}

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(CalculateResolution)


    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    CalculateResolution::CalculateResolution()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    CalculateResolution::~CalculateResolution()
    {
    }

    //----------------------------------------------------------------------------------------------

    /// Algorithm's name for identification. @see Algorithm::name
    const std::string CalculateResolution::name() const { return "CalculateResolution";};

    /// Algorithm's version for identification. @see Algorithm::version
    int CalculateResolution::version() const { return 1;};

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string CalculateResolution::category() const { return "Reflectometry\\ISIS";}

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string CalculateResolution::summary() const { return "Calculates the reflectometry resolution (dq/q) for a given workspace.";};

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
    */
    void CalculateResolution::init()
    {
      declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input,boost::make_shared<InstrumentValidator>()),
      "Workspace to calculate the instrument resolution of.");

      declareProperty("Theta", Mantid::EMPTY_DBL(), "Theta in degrees", Direction::Input);
      declareProperty("Resolution", Mantid::EMPTY_DBL(), "Calculated resolution (dq/q).", Direction::Output);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
    */
    void CalculateResolution::exec()
    {
      const MatrixWorkspace_sptr ws = getProperty("Workspace");
      double theta = getProperty("Theta");

      if(isEmpty(theta))
      {
        const Kernel::Property* logData = ws->mutableRun().getLogData("THETA");

        if(!logData)
          throw std::runtime_error("Value for theta could not be found in log. You must provide it.");

        const std::string thetaStr = logData->value();
        Mantid::Kernel::Strings::convert<double>(thetaStr, theta);

        g_log.notice() << "Found '" << theta << "' as value for theta in log." << std::endl;
      }

      boost::shared_ptr<const IComponent> slit1, slit2;
      slit1 = getComponent(ws, "slit1");
      slit2 = getComponent(ws, "slit2");

      const double slit1Z = slit1->getPos().Z() * 1000.0; //Converting from mm to m
      const double slit2Z = slit2->getPos().Z() * 1000.0; //Converting from mm to m

      const double slit1VG = slit1->getNumberParameter("vertical gap").front();
      const double slit2VG = slit2->getNumberParameter("vertical gap").front();

      const double vGap = slit1VG + slit2VG;
      const double zDiff = slit2Z - slit1Z;

      const double resolution = atan(vGap / (2 * zDiff)) * 180.0 / M_PI / theta;

      setProperty("Resolution", resolution);
    }

  } // namespace Algorithms
} // namespace Mantid

