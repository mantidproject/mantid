/*WIKI* 


===Bin-by-bin mode===
In this, the default scenario, each spectrum in the workspace is normalised on a bin-by-bin basis by the monitor spectrum given. The error on the monitor spectrum is taken into account.
The normalisation scheme used is:

<math>(s_i)_{Norm}=(\frac{s_i}{m_i})*\Delta w_i*\frac{\sum_i{m_i}}{\sum_i(\Delta w_i)}</math>

where <math>s_i</math> is the signal in a bin, <math>m_i</math> the count in the corresponding monitor bin, <math>\Delta w_i</math> the bin width, <math>\sum_i{m_i}</math> the integrated monitor count and <math>\sum_i{\Delta w_i}</math> the sum of the monitor bin widths.
In words, this means that after normalisation each bin is multiplied by the bin width and the total monitor count integrated over the entire frame, and then divided by the total frame width. This leads to a normalised histogram which has unit of counts, as before.

If the workspace does not have common binning, then the monitor spectrum is rebinned internally to match each data spectrum prior to doing the normalisation.

===Normalisation by integrated count mode===
This mode is used if one or both of the relevant 'IntegrationRange' optional parameters are set. If either is set to a value outside the workspace range, then it will be reset to the frame minimum or maximum, as appropriate.

The error on the integrated monitor spectrum is taken into account in the normalisation. No adjustment of the overall normalisation takes place, meaning that the output values in the output workspace are technically dimensionless.

=== Restrictions on the input workspace ===

The data must be histogram, non-distribution data.

===Subalgorithms used===

The [[ExtractSingleSpectrum]] algorithm is used to pull out the monitor spectrum if it's part of the InputWorkspace.
For the 'integrated range' option, the [[Integration]] algorithm is used to integrate the monitor spectrum.

In both cases, the [[Divide]] algorithm is used to perform the normalisation.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/NormaliseToMonitor.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListAnyValidator.h"
#include <cfloat>
#include <iomanip>

namespace Mantid
{

//
namespace Kernel {

// the class to verify and modify interconnected properties. 
class PropChanger: public IPropertySettings
{
    // the name of the property, which specifies the workspace which has to be modified
    std::string host_ws_name;
    // the name of the property, which specifies the workspace which can contain single spectra to normalize by 
    std::string host_monws_name;
    // the name of the property, which specifies if you want to allow normalizing by any spectra.
    std::string allow_any_spectra;
    // the pointer to the main host algorithm.
    IPropertyManager * host_algo;

    // the string with allowed monitors indexes
    mutable std::vector<std::string> allowed_values;
    // if the monitors id input string is enabled. 
    mutable bool is_enabled;
    // auxiliary function to obtain list of monitor's ID-s (allowed_values) from the host workspace;
   void  monitor_id_reader()const;
public:
    PropChanger(IPropertyManager * algo,const std::string &WSProperty,const std::string &MonWSProperty,const std::string &AllowAnySpectra):
      host_ws_name(WSProperty),host_monws_name(MonWSProperty), allow_any_spectra(AllowAnySpectra),host_algo(algo),is_enabled(true){}
  // if input to "
   bool isEnabled()const{
       API::MatrixWorkspace_const_sptr monitorsWS = host_algo->getProperty(host_monws_name);
       if(monitorsWS){
           is_enabled = false;
       }else{
           is_enabled = true;
       }
       return is_enabled;
   }
   //
   bool isConditionChanged()const{
       if(!is_enabled)return false;
       // read monitors list from the input workspace
       monitor_id_reader();
       // if no monitors are associated with workspace -- ok -- you just have to use spectra_ID (may be not on an monitor)
       if(allowed_values.empty())return false;
       // clear MonitorSpectraProperty if you decided to use MonitorID 
       //host_algo->setProperty("MonitorSpectrum","-1");
       //Kernel::Property *pProperty = host_algo->getPointerToProperty(
       return true;
   }
   // function which modifies the allowed values for the list of monitors. 
   void applyChanges(Property *const pProp){
       PropertyWithValue<int>* piProp  = dynamic_cast<PropertyWithValue<int>* >(pProp);
       if(!piProp){
           throw(std::invalid_argument("modify allowed value has been called on wrong property"));
       }
       std::vector<int> ival(allowed_values.size());
       for(size_t i=0;i<ival.size();i++){
                ival[i]=boost::lexical_cast<int>(allowed_values[i]);
       }
       piProp->modify_validator(new ListAnyValidator<int>(ival));
           
   }
   // interface needs it but if indeed proper clone used -- do not know. 
   virtual IPropertySettings* clone(){return new PropChanger(host_algo,host_ws_name,host_monws_name,allow_any_spectra);}
   virtual ~PropChanger(){};

};
// read the monitors list from the workspace;
void
PropChanger::monitor_id_reader()const
{
    allowed_values.clear();
    API::MatrixWorkspace_const_sptr inputWS = host_algo->getProperty(host_ws_name);
    if(!inputWS){
        return ;
    }

    Geometry::Instrument_const_sptr pInstr = inputWS->getInstrument();
    if (!pInstr){
        return ;
    }
    std::vector<detid_t> mon = pInstr->getMonitors();
    allowed_values.resize(mon.size());
    for(size_t i=0;i<mon.size();i++){
        allowed_values[i]=boost::lexical_cast<std::string>(mon[i]);
    }
    return ;
}

} // end Kernel Namespace;

namespace Algorithms
{

// Register with the algorithm factory
DECLARE_ALGORITHM(NormaliseToMonitor)

using namespace Kernel;
using namespace API;
using std::size_t;

// logger for operations  
Kernel::Logger& NormaliseToMonitor::g_log =Kernel::Logger::get("Algorithms");

/// Default constructor
NormaliseToMonitor::NormaliseToMonitor() :
  Algorithm(), m_monitor(), m_commonBins(false),
  m_integrationMin( EMPTY_DBL() ),//EMPTY_DBL() is a tag to say that the value hasn't been set
  m_integrationMax( EMPTY_DBL() )
{}

/// Destructor
NormaliseToMonitor::~NormaliseToMonitor() {}

/// Sets documentation strings for this algorithm
void NormaliseToMonitor::initDocs()
{
  this->setWikiSummary("Normalises a 2D workspace by a specified spectrum, spectrum, described by a monitor ID or spectrun provided in a separate worskspace. ");
  this->setOptionalMessage("Normalises a 2D workspace by a specified spectrum or spectrum, described by monitor ID." 
                           "If monitor spectrum specified, it is used as input property");
}

void NormaliseToMonitor::init()
{
  CompositeWorkspaceValidator<> *val = new CompositeWorkspaceValidator<>;
  val->add(new HistogramValidator<>);
  val->add(new RawCountValidator<>);
  // It's been said that we should restrict the unit to being wavelength, but I'm not sure about that...
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,val),
    "Name of the input workspace. Must be a non-distribution histogram.");

   //
 // declareProperty("NofmalizeByAnySpectra",false,
 //   "Allows you to normalize the workspace by any spectra, not just by the monitor one");
  // Can either set a spectrum within the workspace to be the monitor spectrum.....
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
    "Name to use for the output workspace");
  // should be any spectrum ID, but named this property MonitorSpectrum to keep compartibility with previous scripts
  // Can either set a spectrum within the workspace to be the monitor spectrum.....
  declareProperty("MonitorSpectrum",-1,
      "The spectrum number within the InputWorkspace you want to normalize by (usually monitor spectrum but can be any)",Direction::InOut);

  // Or take monitor ID to identify the spectrum one wish to use or
   declareProperty("MonitorID",-1,
    "The monitor ID, which defines the monitor spectrum within the InputWorkspace. Will be overridden by Monitor spectrum if one is provided in the field above");
   // set up the validator, which would verify if spectrum is correct
   setPropertySettings("MonitorID",new PropChanger(this,"InputWorkspace","MonitorWorkspace","NormalizeByAnySpectra"));

  // ...or provide it in a separate workspace (note: optional WorkspaceProperty)
  declareProperty(new WorkspaceProperty<>("MonitorWorkspace","",Direction::Input,true,val->clone()),
    "A workspace containing the monitor spectrum");
  declareProperty("MonitorWorkspaceSpectrum",0,
      "The spectrum number within the MonitorWorkspace you want to normalize by (usually monitor spectrum but can be any)",Direction::InOut);

  // If users set either of these optional properties two things happen
  // 1) normalisation is by an integrated count instead of bin-by-bin
  // 2) if the value is within the range of X's in the spectrum it crops the spectrum
  declareProperty( "IntegrationRangeMin", EMPTY_DBL(),
    "If set, normalisation will be by integrated count from this minimum x value");
  declareProperty( "IntegrationRangeMax", EMPTY_DBL(),
    "If set, normalisation will be by integrated count up to this maximum x value");
  declareProperty("IncludePartialBins", false, 
    "If true and an integration range is set then partial bins at either \n"
    "end of the integration range are also included");
}

void NormaliseToMonitor::exec()
{
  // Get the input workspace
  const MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  // First check the inputs, throws std::runtime_error if a property is invalid
  this->checkProperties(inputWS);

  // See if the normalisation with integration properties are set,
  // throws std::runtime_error if a property is invalid
  const bool integrate = this->setIntegrationProps();

  if ( integrate )
  {
    this->normaliseByIntegratedCount(inputWS,outputWS);
  }
  else
  {
    this->normaliseBinByBin(inputWS,outputWS);
  }

  setProperty("OutputWorkspace",outputWS);
}

/** Makes sure that the input properties are set correctly
 *  @param inputWorkspace The input workspace
 *  @throw std::runtime_error If the properties are invalid
 */
void NormaliseToMonitor::checkProperties(API::MatrixWorkspace_sptr inputWorkspace)
{
  // Check where the monitor spectrum should come from
  Property* monSpec = getProperty("MonitorSpectrum");
  Property* monWS   = getProperty("MonitorWorkspace");
  Property* monID   = getProperty("MonitorID");
  // Is the monitor spectrum within the main input workspace
  const bool inWS = !monSpec->isDefault();
  // Or is it in a separate workspace
  const bool sepWS = !monWS->isDefault();
  // or monitor ID
  const bool monSP = !monID->isDefault();
  // something has to be set
  if ( !inWS && !sepWS && !monSP)
  {
    const std::string mess("Neither the MonitorSpectrum, nor the MonitorID or the MonitorWorkspace property has been set");
    g_log.error()<<mess<<std::endl;
    throw std::runtime_error(mess);
  }
  // One and only one of these properties should have been set
  if ( inWS && sepWS )
  {
    const std::string mess("Only one of the MonitorSpectrum and MonitorWorkspace properties should be set");
    throw std::runtime_error(mess);
  }

  // Do a check for common binning and store
  m_commonBins = API::WorkspaceHelpers::commonBoundaries(inputWorkspace);

  int spec_num(-1);
  // Check the monitor spectrum or workspace and extract into new workspace
  m_monitor = sepWS ? this->getMonitorWorkspace(inputWorkspace) : this->getInWSMonitorSpectrum(inputWorkspace,spec_num) ;

  // Check that the 'monitor' spectrum actually relates to a monitor - warn if not
  try {
    Geometry::IDetector_const_sptr mon = m_monitor->getDetector(0);
    if ( !mon->isMonitor() )
    {
      g_log.warning()<<"The spectrum N: "<<spec_num<<" in MonitorWorkspace does not refer to a monitor.\n"
                     <<"Continuing with normalisation regardless.";
    }
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.warning("Unable to check if the spectrum provided relates to a monitor - "
                  "the instrument is not fully specified.\n"
                  "Continuing with normalisation regardless.");
  }
}

/** Checks and retrieves the requested spectrum out of the input workspace
 *  @param inputWorkspace The input workspace
 *  @returns A workspace containing the monitor spectrum only
 *  @returns spectra number (WS ID) which is used to normalize by
 *  @throw std::runtime_error If the properties are invalid
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::getInWSMonitorSpectrum(API::MatrixWorkspace_sptr inputWorkspace,int &spectra_num)
{
 // this is the index of the spectra within the workspace and we need to indetnify it either from DetID or fron SpecID
 // size_t spectra_num(-1);
// try monitor spectrum. If it is specified, it overides everything
  int monitorSpec = getProperty("MonitorSpectrum");
  if(monitorSpec<0){ 
      // Get hold of the monitor spectrum
    int monitorID = getProperty("MonitorID");
    if (monitorID < 0) {
        throw std::runtime_error("Both MonitorSpectrum and MonitorID can not be negative");
    }
    // set spectra of detector's ID of one selected monitor ID
    std::vector<detid_t> detID(1,monitorID);
    // got the index of correspondent spectras (should be only one). 
    std::vector<size_t>  indexList;
    inputWorkspace->getIndicesFromDetectorIDs(detID,indexList);
    if(indexList.empty()){
         throw std::runtime_error("Can not find spectra, coorespoinding to the requested monitor ID");
    }
    if(indexList.size()>1){
         throw std::runtime_error("More then one spectra coorespods to the requested monitor ID, which is unheard of");
    }
    spectra_num = (int)indexList[0];
  }else{ // monitor spectrum is specified.
        spec2index_map specs;
        const SpectraAxis* axis = dynamic_cast<const SpectraAxis*>(inputWorkspace->getAxis(1));
        if ( ! axis)
        {
            throw std::runtime_error("Cannot retrieve monitor spectrum - spectrum numbers not attached to workspace");
        }
        axis->getSpectraIndexMap(specs);
        if ( ! specs.count(monitorSpec) )
        {
            throw std::runtime_error("Input workspace does not contain spectrum number given for MonitorSpectrum");
        }
        spectra_num = (int)specs[monitorSpec];
  }
  return this->extractMonitorSpectrum(inputWorkspace,spectra_num);
}

/** Checks and retrieves the monitor spectrum out of the input workspace
 *  @param inputWorkspace The input workspace
 *  @returns A workspace containing the monitor spectrum only
 *  @throw std::runtime_error If the properties are invalid
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::getMonitorWorkspace(API::MatrixWorkspace_sptr inputWorkspace)
{
  // Get the workspace from the ADS. Will throw if it's not there.
  MatrixWorkspace_sptr monitorWS = getProperty("MonitorWorkspace");
  int monitorSpec = getProperty("MonitorWorkspaceSpectrum");
  // Check that it's a single spectrum workspace
  if ( static_cast<int>(monitorWS->getNumberHistograms()) < monitorSpec+1 )
  {
    throw std::runtime_error("The MonitorWorkspace must contain the MonitorWorkspaceSpectrum");
  }
  // Check that the two workspace come from the same instrument
  if ( monitorWS->getInstrument()->getName() != inputWorkspace->getInstrument()->getName() )
  {
    throw std::runtime_error("The Input and Monitor workspaces must come from the same instrument");
  }
  // Check that they're in the same units
  if ( monitorWS->getAxis(0)->unit() != inputWorkspace->getAxis(0)->unit() )
  {
    throw std::runtime_error("The Input and Monitor workspaces must have the same unit");
  }

  // In this case we need to test whether the bins in the monitor workspace match
  m_commonBins = (m_commonBins &&
                  API::WorkspaceHelpers::matchingBins(inputWorkspace,monitorWS,true) );

  // If the workspace passes all these tests, make a local copy because it will get changed
  return this->extractMonitorSpectrum(monitorWS,monitorSpec);
}

/** Pulls the monitor spectrum out of a larger workspace
 *  @param WS :: The workspace containing the spectrum to extract
 *  @param index :: The index of the spectrum to extract
 *  @returns A workspace containing the single spectrum requested
 */
API::MatrixWorkspace_sptr NormaliseToMonitor::extractMonitorSpectrum(API::MatrixWorkspace_sptr WS, const std::size_t index)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("ExtractSingleSpectrum");
  childAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", WS);
  childAlg->setProperty<int>("WorkspaceIndex", static_cast<int>(index));

  childAlg->executeAsSubAlg();

  // Only get to here if successful
  return childAlg->getProperty("OutputWorkspace");
}

/** Sets the maximum and minimum X values of the monitor spectrum to use for integration
 *  @return True if the maximum or minimum values are set
 *  @throw std::runtime_error If the minimum was set higher than the maximum
 */
bool NormaliseToMonitor::setIntegrationProps()
{
  m_integrationMin = getProperty("IntegrationRangeMin");
  m_integrationMax = getProperty("IntegrationRangeMax");

  // Check if neither of these have been changed from their defaults (EMPTY_DBL())
  if ( isEmpty(m_integrationMin) && isEmpty(m_integrationMax) )
  {
    // Nothing has been set so the user doesn't want to use integration so let's move on
    return false;
  }
  // Yes integration is going to be used...

  // There is only one set of values that is unacceptable let's check for that
  if ( !isEmpty(m_integrationMin) && !isEmpty(m_integrationMax) )
  {
    if ( m_integrationMin > m_integrationMax )
    {
      throw std::runtime_error("Integration minimum set to larger value than maximum!");
    }
  }

  // Now check the end X values are within the X value range of the workspace
  if ( isEmpty(m_integrationMin) || m_integrationMin < m_monitor->readX(0).front() )
  {
    g_log.warning() << "Integration range minimum set to workspace min: " << m_integrationMin << std::endl;
    m_integrationMin = m_monitor->readX(0).front();
  }
  if ( isEmpty(m_integrationMax) || m_integrationMax > m_monitor->readX(0).back() )
  {
    g_log.warning() << "Integration range maximum set to workspace max: " << m_integrationMax << std::endl;
    m_integrationMax = m_monitor->readX(0).back();
  }
  
  // Return indicating that these properties should be used
  return true;
}

/** Carries out a normalisation based on the integrated count of the monitor over a range
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The result workspace
 */
void NormaliseToMonitor::normaliseByIntegratedCount(API::MatrixWorkspace_sptr inputWorkspace,
                                                    API::MatrixWorkspace_sptr& outputWorkspace)
{
  // Add up all the bins so it's just effectively a single value with an error
  IAlgorithm_sptr integrate = createSubAlgorithm("Integration");
  integrate->setProperty<MatrixWorkspace_sptr>("InputWorkspace", m_monitor);
  integrate->setProperty("RangeLower",m_integrationMin);
  integrate->setProperty("RangeUpper",m_integrationMax);
  integrate->setProperty<bool>("IncludePartialBins",getProperty("IncludePartialBins"));

  integrate->executeAsSubAlg();

  // Get back the result
  m_monitor = integrate->getProperty("OutputWorkspace");

  // Run the divide algorithm explicitly to enable progress reporting
  IAlgorithm_sptr divide = createSubAlgorithm("Divide",0.0,1.0);
  divide->setProperty<MatrixWorkspace_sptr>("LHSWorkspace", inputWorkspace);
  divide->setProperty<MatrixWorkspace_sptr>("RHSWorkspace", m_monitor);
  divide->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", outputWorkspace);

  divide->executeAsSubAlg();

  // Get back the result
  outputWorkspace = divide->getProperty("OutputWorkspace");
}

/** Carries out the bin-by-bin normalisation
 *  @param inputWorkspace The input workspace
 *  @param outputWorkspace The result workspace
 */
void NormaliseToMonitor::normaliseBinByBin(API::MatrixWorkspace_sptr inputWorkspace,
                                           API::MatrixWorkspace_sptr& outputWorkspace)
{ 
  // Only create output workspace if different to input one
  if (outputWorkspace != inputWorkspace ) 
    outputWorkspace = WorkspaceFactory::Instance().create(inputWorkspace);

  // Get hold of the monitor spectrum
  const MantidVec& monX = m_monitor->readX(0);
  MantidVec& monY = m_monitor->dataY(0);
  MantidVec& monE = m_monitor->dataE(0);
  // Calculate the overall normalisation just the once if bins are all matching
  if (m_commonBins) this->normalisationFactor(m_monitor->readX(0),&monY,&monE);

  const size_t numHists = inputWorkspace->getNumberHistograms();
  MantidVec::size_type specLength = inputWorkspace->blocksize();
  Progress prog(this,0.0,1.0,numHists);
  // Loop over spectra
  PARALLEL_FOR3(inputWorkspace,outputWorkspace,m_monitor)
  for (int64_t i = 0; i < int64_t(numHists); ++i)
  {
    PARALLEL_START_INTERUPT_REGION
    prog.report();

    const MantidVec& X = inputWorkspace->readX(i);
    // If not rebinning, just point to our monitor spectra, otherwise create new vectors
    MantidVec* Y = ( m_commonBins ? &monY : new MantidVec(specLength) );
    MantidVec* E = ( m_commonBins ? &monE : new MantidVec(specLength) );

    if (!m_commonBins)
    {
      // ConvertUnits can give X vectors of all zeroes - skip these, they cause problems
      if (X.back() == 0.0 && X.front() == 0.0) continue;
      // Rebin the monitor spectrum to match the binning of the current data spectrum
      VectorHelper::rebinHistogram(monX,monY,monE,X,*Y,*E,false);
      // Recalculate the overall normalisation factor
      this->normalisationFactor(X,Y,E);
    }

    const MantidVec& inY = inputWorkspace->readY(i);
    const MantidVec& inE = inputWorkspace->readE(i);
    MantidVec& YOut = outputWorkspace->dataY(i);
    MantidVec& EOut = outputWorkspace->dataE(i);
    outputWorkspace->dataX(i) = inputWorkspace->readX(i);
    // The code below comes more or less straight out of Divide.cpp
    for (MantidVec::size_type k = 0; k < specLength; ++k)
    {
      // Get references to the input Y's
      const double& leftY = inY[k];
      const double& rightY = (*Y)[k];

      // Calculate result and store in local variable to avoid overwriting original data if
      // output workspace is same as one of the input ones
      const double newY = leftY/rightY;

      if (fabs(rightY)>1.0e-12 && fabs(newY)>1.0e-12)
      {
        const double lhsFactor = (inE[k]<1.0e-12|| fabs(leftY)<1.0e-12) ? 0.0 : pow((inE[k]/leftY),2);
        const double rhsFactor = (*E)[k]<1.0e-12 ? 0.0 : pow(((*E)[k]/rightY),2);
        EOut[k] = std::abs(newY) * sqrt(lhsFactor+rhsFactor);
      }

      // Now store the result
      YOut[k] = newY;
    } // end loop over current spectrum

    if (!m_commonBins) { delete Y; delete E; }
    PARALLEL_END_INTERUPT_REGION
  } // end loop over spectra
  PARALLEL_CHECK_INTERUPT_REGION
}

/** Calculates the overall normalisation factor.
 *  This multiplies result by (bin width * sum of monitor counts) / total frame width.
 *  @param X The X vector
 *  @param Y The data vector
 *  @param E The error vector
 */
void NormaliseToMonitor::normalisationFactor(const MantidVec& X, MantidVec* Y, MantidVec* E)
{
  const double monitorSum = std::accumulate(Y->begin(), Y->end(), 0.0);
  const double range = X.back() - X.front();
  MantidVec::size_type specLength = Y->size();
  for (MantidVec::size_type j = 0; j < specLength; ++j)
  {
    const double factor = range / ( (X[j+1]-X[j]) * monitorSum );
    (*Y)[j] *= factor;
    (*E)[j] *= factor;
  }
}

} // namespace Algorithm
} // namespace Mantid
