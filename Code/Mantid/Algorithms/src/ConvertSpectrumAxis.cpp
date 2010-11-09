//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <cfloat>

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
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
    {
      (void) xdata; (void) ydata; (void) l1; (void)l2; (void)twoTheta; (void)emode; (void)efixed; (void)delta;
    }

    void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
    {
      (void) xdata; (void) ydata; (void) l1; (void)l2; (void)twoTheta; (void)emode; (void)efixed; (void)delta;
    }
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
  void ConvertSpectrumAxis::init()
  {
    // Validator for Input Workspace
    CompositeValidator<> *wsVal = new CompositeValidator<>;
    wsVal->add(new HistogramValidator<>);
    wsVal->add(new SpectraAxisValidator<>);
    
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, wsVal));
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));
    std::vector<std::string> targetOptions = Mantid::Kernel::UnitFactory::Instance().getKeys();
    targetOptions.push_back("theta");
    declareProperty("Target","",new ListValidator(targetOptions),
      "The detector attribute to convert the spectrum axis to");
    std::vector<std::string> eModeOptions;
    eModeOptions.push_back("Direct");
    eModeOptions.push_back("Indirect");
    declareProperty("EMode", "Direct",new ListValidator(eModeOptions),
      "The energy mode type required for some conversions");
  }
  void ConvertSpectrumAxis::exec()
  {
    // Get the input workspace
    MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
    std::string unitTarget = getProperty("Target");
    const Axis* const specAxis = inputWS->getAxis(1);
    // Loop over the original spectrum axis, finding the theta (n.b. not 2theta!) for each spectrum
    // and storing it's corresponding workspace index
    // Map will be sorted on theta, so resulting axis will be ordered as well
    std::multimap<double,int> indexMap;
    const int nHist = inputWS->getNumberHistograms();
    const int nBins = inputWS->blocksize();
    const bool isHist = inputWS->isHistogramData();
    int nxBins;
    if ( isHist ) { nxBins = nBins+1; }
    else { nxBins = nBins; }
    bool warningGiven = false;
    if ( unitTarget != "theta" )
    {
      Kernel::Unit_const_sptr fromUnit = inputWS->getAxis(0)->unit();
      Kernel::Unit_const_sptr toUnit = UnitFactory::Instance().create(unitTarget);
      IObjComponent_const_sptr source = inputWS->getInstrument()->getSource();
      IObjComponent_const_sptr sample = inputWS->getInstrument()->getSample();
      std::vector<double> emptyVector;
      const double l1 = source->getDistance(*sample);
      const std::string emodeStr = getProperty("EMode");
      int emode = 0;
      if (emodeStr == "Direct") emode=1;
      else if (emodeStr == "Indirect") emode=2;
      const double delta = 0.0;
      double efixed;
      for ( int i = 0; i < nHist; i++ )
      {
        std::vector<double> xval;
        xval.push_back(inputWS->readX(i).front());
        xval.push_back(inputWS->readX(i).back());
        IDetector_sptr detector = inputWS->getDetector(i);
        double twoTheta, l1val, l2;
        if ( ! detector->isMonitor() )
        {
          twoTheta = inputWS->detectorTwoTheta(detector);
          l2 = detector->getDistance(*sample);
          l1val = l1;
          if (emode==2)
          {
            std::vector<double> efixedVec = detector->getNumberParameter("Efixed");
            if ( efixedVec.empty() )
            {
              int detid = detector->getID();
              IDetector_sptr detectorSingle = inputWS->getInstrument()->getDetector(detid);
              efixedVec = detectorSingle->getNumberParameter("Efixed");
            }
            if (! efixedVec.empty() ) 
            {
              efixed = efixedVec.at(0);
              g_log.debug() << "Detector: " << detector->getID() << " EFixed: " << efixed << "\n";
            }
            else
            {
              efixed = 0.0;
              g_log.warning() << "Efixed could not be found for detector " << detector->getID() << ", set to 0.0\n";
            }
          }
        }
        else
        {
          twoTheta = 0.0;
          l2 = l1;
          l1val = 0.0;
          efixed = DBL_MIN;
        }
        fromUnit->toTOF(xval,emptyVector,l1val,l2,twoTheta,emode,efixed,delta);
        toUnit->fromTOF(xval,emptyVector,l1val,l2,twoTheta,emode,efixed,delta);
        double value = ( xval.front() + xval.back() ) / 2;
        indexMap.insert(std::make_pair(value, i));
      }
    }
    else
    {
      for (int i = 0; i < nHist; ++i)
      {
        try 
        {
          IDetector_const_sptr det = inputWS->getDetector(i);
          indexMap.insert( std::make_pair( inputWS->detectorTwoTheta(det)*180.0/M_PI , i ) );
        }
        catch(Exception::NotFoundError &)
        {
          if (!warningGiven) g_log.warning("The instrument definition is incomplete - spectra dropped from output");
          warningGiven = true;
        }
      }
    }
    // Create the output workspace. Can't re-use the input one because we'll be re-ordering the spectra.
    MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, indexMap.size(),nxBins,nBins);
    // Now set up a new, numeric axis holding the theta values corresponding to each spectrum
    NumericAxis* const newAxis = new NumericAxis(indexMap.size());
    outputWS->replaceAxis(1,newAxis);
    // The unit of this axis is radians. Use the 'radians' unit defined above.
    if ( unitTarget == "theta" )
    {
      newAxis->unit() = boost::shared_ptr<Unit>(new Degrees);
    }
    else
    {
      newAxis->unit() = UnitFactory::Instance().create(unitTarget);
    }
    std::multimap<double,int>::const_iterator it;
    int currentIndex = 0;
    for (it = indexMap.begin(); it != indexMap.end(); ++it)
    {
      // Set the axis value
      newAxis->setValue(currentIndex,it->first);
      // Now copy over the data
      outputWS->dataX(currentIndex) = inputWS->dataX(it->second);
      outputWS->dataY(currentIndex) = inputWS->dataY(it->second);
      outputWS->dataE(currentIndex) = inputWS->dataE(it->second);
      ++currentIndex;
    }
    setProperty("OutputWorkspace",outputWS);
  }
} // namespace Algorithms
} // namespace Mantid
