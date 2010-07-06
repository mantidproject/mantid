//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/NumericAxis.h"

/// @cond
// Don't document this very long winded way of getting "radians" to print on the axis.
namespace
{
  class Degrees : public Mantid::Kernel::Unit
  {
    const std::string unitID() const { return ""; }
    const std::string caption() const { return "Scattering angle"; }
    const std::string label() const { return "degrees"; }
    void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const {}
    void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const {}
  };
} // end anonynmous namespace
/// @endcond

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertSpectrumAxis)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_const_sptr;

void ConvertSpectrumAxis::init()
{
  declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

  declareProperty("Target","",new ListValidator(std::vector<std::string>(1,"theta")),
                  "The detector attribute to convert the spectrum axis to");
}

void ConvertSpectrumAxis::exec()
{
  // Get the input workspace
  Workspace2D_const_sptr inputWS = getProperty("InputWorkspace");

  // Check that the input workspace has a spectrum axis for axis 1
  // Could put this in a validator later
  const Axis* const specAxis = inputWS->getAxis(1);
  if ( ! specAxis->isSpectra() )
  {
    g_log.error("The input workspace must have spectrum numbers along one axis");
    throw std::runtime_error("The input workspace must have spectrum numbers along one axis");
  }

  // Loop over the original spectrum axis, finding the theta (n.b. not 2theta!) for each spectrum
  // and storing it's corresponding workspace index
  // Map will be sorted on theta, so resulting axis will be ordered as well
  std::multimap<double,int> theta2indexMap;
  const int axisLength = inputWS->getNumberHistograms();
  bool warningGiven = false;
  for (int i = 0; i < axisLength; ++i)
  {
    try {
      IDetector_const_sptr det = inputWS->getDetector(i);
      theta2indexMap.insert( std::make_pair( inputWS->detectorTwoTheta(det)*180.0/M_PI , i ) );
    } catch(Exception::NotFoundError) {
      if (!warningGiven) g_log.warning("The instrument definition is incomplete - spectra dropped from output");
      warningGiven = true;
    }
  }

  // Create the output workspace. Can't re-use the input one because we'll be re-ordering the spectra.
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS,
    theta2indexMap.size(),inputWS->readX(0).size(),inputWS->blocksize());
  setProperty("OutputWorkspace",outputWS);

  // Now set up a new, numeric axis holding the theta values corresponding to each spectrum
  NumericAxis* const newAxis = new NumericAxis(theta2indexMap.size());
  outputWS->replaceAxis(1,newAxis);
  // The unit of this axis is radians. Use the 'radians' unit defined above.
  newAxis->unit() = boost::shared_ptr<Unit>(new Degrees);

  std::multimap<double,int>::const_iterator it;
  int currentIndex = 0;
  for (it = theta2indexMap.begin(); it != theta2indexMap.end(); ++it)
  {
    // Set the axis value
    newAxis->setValue(currentIndex,it->first);
    // Now copy over the data
    outputWS->dataX(currentIndex) = inputWS->dataX(it->second);
    outputWS->dataY(currentIndex) = inputWS->dataY(it->second);
    outputWS->dataE(currentIndex) = inputWS->dataE(it->second);
    ++currentIndex;
  }

}

} // namespace Algorithms
} // namespace Mantid
