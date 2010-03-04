#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/ParInstrument.h"
#include "MantidAPI/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid
{
namespace API
{

Kernel::Logger& MatrixWorkspace::g_log = Kernel::Logger::get("MatrixWorkspace");

/// Default constructor
MatrixWorkspace::MatrixWorkspace() : 
  Workspace(), m_axes(), m_isInitialized(false),
  sptr_instrument(new Instrument), m_spectramap(), m_sample(),
  m_YUnit(), m_YUnitLabel(), m_isDistribution(false), m_parmap(), m_masks()
{}

/// Destructor
// RJT, 3/10/07: The Analysis Data Service needs to be able to delete workspaces, so I moved this from protected to public.
MatrixWorkspace::~MatrixWorkspace()
{
  for (unsigned int i = 0; i < m_axes.size(); ++i)
  {
    delete m_axes[i];
  }
}

/** Initialize the workspace. Calls the protected init() method, which is implemented in each type of
 *  workspace. Returns immediately if the workspace is already initialized.
 *  @param NVectors The number of spectra in the workspace (only relevant for a 2D workspace
 *  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param YLength The number of data/error points in each vector (must all be the same)
 */
void MatrixWorkspace::initialize(const int &NVectors, const int &XLength, const int &YLength)
{
  // Check validity of arguments
  if (NVectors <= 0 || XLength <= 0 || YLength <= 0)
  {
    g_log.error("All arguments to init must be positive and non-zero");
    throw std::out_of_range("All arguments to init must be positive and non-zero");
  }

  // Bypass the initialization if the workspace has already been initialized.
  if (m_isInitialized) return;

  // Invoke init() method of the derived class inside a try/catch clause
  try
  {
    this->init(NVectors, XLength, YLength);
  }
  catch(std::runtime_error& ex)
  {
    g_log.error() << "Error initializing the workspace" << ex.what() << std::endl;
    throw;
  }

  // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
  m_isInitialized = true;
}

/** Set the instrument
 *
 * \param instr Shared pointer to an instrument.
 */
void MatrixWorkspace::setInstrument(const IInstrument_sptr& instr)
{
  boost::shared_ptr<Instrument> tmp = boost::dynamic_pointer_cast<Instrument>(instr);
  if (tmp)
  {
    sptr_instrument=tmp;
  }
  else
  {
    boost::shared_ptr<ParInstrument> tmp = boost::dynamic_pointer_cast<ParInstrument>(instr);
    if (tmp)
    {
      sptr_instrument = tmp->baseInstrument();
      m_parmap = tmp->getParameterMap();
    }
  }
}
/** Set the sample
 *  @param sample constant reference to the sample object
 */
void MatrixWorkspace::setSample(const Sample& sample)
{ 
  m_sample.access()=sample;
}

/** Get a const reference to the SpectraDetectorMap associated with this workspace.
 *  Can ONLY be taken as a const reference!
 *
 *  @return The SpectraDetectorMap
 */
const SpectraDetectorMap& MatrixWorkspace::spectraMap() const
{
  return *m_spectramap;
}

/** Get a reference to the SpectraDetectorMap associated with this workspace.
 *  This non-const method will copy the map if it is shared between more than one workspace,
 *  and the reference returned will be to the copy.
 *  Can ONLY be taken by reference!
 *
 *  @return The SpectraDetectorMap
 */
SpectraDetectorMap& MatrixWorkspace::mutableSpectraMap()
{
  return m_spectramap.access();
}

/** Get a constant reference to the Sample associated with this workspace.

*/
const  Sample& MatrixWorkspace::sample() const
{
	return *m_sample;
}
/** Get a reference to the Sample associated with this workspace.

*/
Sample& MatrixWorkspace::mutableSample()
{
	return m_sample.access();
}
/** Get the effective detector for the given spectrum
 *  @param  index The workspace index for which the detector is required
 *  @return A single detector object representing the detector(s) contributing
 *          to the given spectrum number. If more than one detector contributes then
 *          the returned object's concrete type will be DetectorGroup.
 *  @throw  std::runtime_error if the SpectraDetectorMap has not been filled
 *  @throw  Kernel::Exception::NotFoundError if the SpectraDetectorMap or the Instrument
            do not contain the requested spectrum number of detector ID
 */
Geometry::IDetector_sptr MatrixWorkspace::getDetector(const int index) const
{
  if ( ! m_spectramap->nElements() )
  {
    g_log.error("SpectraDetectorMap has not been populated.");
    throw std::runtime_error("SpectraDetectorMap has not been populated.");
  }
  
  const int spectrum_number = getAxis(1)->spectraNo(index);
  const std::vector<int> dets = m_spectramap->getDetectors(spectrum_number);
  if ( dets.empty() )
  {
    g_log.debug() << "Spectrum number " << spectrum_number << " not found" << std::endl;
    throw Kernel::Exception::NotFoundError("Spectrum number not found", spectrum_number);
  }
  else if ( dets.size() == 1 ) 
  {
    // If only 1 detector for the spectrum number, just return it
    return getInstrument()->getDetector(dets[0]);
  }
  // Else need to construct a DetectorGroup and return that
  std::vector<Geometry::IDetector_sptr> dets_ptr;
  std::vector<int>::const_iterator it;
  for ( it = dets.begin(); it != dets.end(); ++it )
  {
    dets_ptr.push_back( getInstrument()->getDetector(*it) );
  }
  
  return Geometry::IDetector_sptr( new Geometry::DetectorGroup(dets_ptr) );
}

/** Returns the 2Theta scattering angle for a detector
 *  @param det A pointer to the detector object (N.B. might be a DetectorGroup)
 *  @return The scattering angle (0 < theta < pi)
 */
double MatrixWorkspace::detectorTwoTheta(Geometry::IDetector_const_sptr det) const
{
  const Geometry::V3D samplePos = getInstrument()->getSample()->getPos();
  const Geometry::V3D beamLine = samplePos - getInstrument()->getSource()->getPos();

  return det->getTwoTheta(samplePos,beamLine);
}

/** Get a shared pointer to the instrument associated with this workspace
 *
 *  @return The instrument class
 */
IInstrument_sptr MatrixWorkspace::getInstrument()const
{
  if ( m_parmap->size() == 0 )  return sptr_instrument;
  ParInstrument* pi = new ParInstrument(sptr_instrument,m_parmap);
  IInstrument* ii = static_cast<IInstrument*>(pi);
  boost::shared_ptr<IInstrument> tmp(ii);
  return (tmp);
}

/** Get a shared pointer to the instrument associated with this workspace
 *
 *  @return The instrument class
 */
boost::shared_ptr<Instrument> MatrixWorkspace::getBaseInstrument()const
{
    return sptr_instrument;
}

/**  Returns a new copy of the instrument parameters
 */
Geometry::ParameterMap& MatrixWorkspace::instrumentParameters()const
{
  return m_parmap.access();
}

const Geometry::ParameterMap& MatrixWorkspace::constInstrumentParameters() const
{
  return *m_parmap;
}

/// The number of axes which this workspace has
const int MatrixWorkspace::axes() const
{
  return static_cast<int>(m_axes.size());
}

/** Get a pointer to a workspace axis
 *  @param axisIndex The index of the axis required
 *  @throw IndexError If the argument given is outside the range of axes held by this workspace
 */
Axis* const MatrixWorkspace::getAxis(const int& axisIndex) const
{
  if ( axisIndex < 0 || axisIndex >= static_cast<int>(m_axes.size()) )
  {
    g_log.error() << "Argument to getAxis (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
    throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Argument to getAxis is invalid for this workspace");
  }

  return m_axes[axisIndex];
}

/** Replaces one of the workspace's axes with the new one provided.
 *  @param axisIndex The index of the axis to replace
 *  @param newAxis A pointer to the new axis. The class will take ownership.
 *  @throw IndexError If the axisIndex given is outside the range of axes held by this workspace
 *  @throw std::runtime_error If the new axis is not of the correct length (within one of the old one)
 */
void MatrixWorkspace::replaceAxis(const int& axisIndex, Axis* const newAxis)
{
  // First check that axisIndex is in range
  if ( axisIndex < 0 || axisIndex >= static_cast<int>(m_axes.size()) )
  {
    g_log.error() << "Value of axisIndex (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
    throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Value of axisIndex is invalid for this workspace");
  }
  // Now check that the new axis is of the correct length
  // Later, may want to allow axis to be one longer than number of vectors, to allow bins.
  if ( std::abs(newAxis->length() - m_axes[axisIndex]->length()) > 1 )
  {
    g_log.error("replaceAxis: The new axis is not a valid length");
    throw std::runtime_error("replaceAxis: The new axis is not a valid length");
  }

  // If we're OK, then delete the old axis and set the pointer to the new one
  delete m_axes[axisIndex];
  m_axes[axisIndex] = newAxis;
}

/// Returns true if the workspace contains data in histogram form, false if it's point-like
const bool MatrixWorkspace::isHistogramData() const
{
  return ( readX(0).size()==readY(0).size() ? false : true );
}

/// Returns the units of the data in the workspace
std::string MatrixWorkspace::YUnit() const
{
  return m_YUnit;
}

/// Sets a new unit for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnit(const std::string& newUnit)
{
  m_YUnit = newUnit;
}

/// Returns a caption for the units of the data in the workspace
std::string MatrixWorkspace::YUnitLabel() const
{
  std::string retVal;
  if ( !m_YUnitLabel.empty() ) retVal = m_YUnitLabel;
  else
  {
    retVal = m_YUnit;
    // If this workspace a distribution & has at least one axis & this axis has its unit set
    // then append that unit to the string to be returned
    if ( !retVal.empty() && this->isDistribution() && this->axes() && this->getAxis(0)->unit() )
    {
      retVal = retVal + " per " + this->getAxis(0)->unit()->label();
    }
  }

  return retVal;
}

/// Sets a new caption for the data (Y axis) in the workspace
void MatrixWorkspace::setYUnitLabel(const std::string& newLabel)
{
  m_YUnitLabel = newLabel;
}

/// Are the Y-values in this workspace dimensioned?
const bool& MatrixWorkspace::isDistribution() const
{
  return m_isDistribution;
}

/// Set the flag for whether the Y-values are dimensioned
bool& MatrixWorkspace::isDistribution(bool newValue)
{
  m_isDistribution = newValue;
  return m_isDistribution;
}

/** Masks a single bin. It's value (and error) will be scaled by (1-weight).
 *  @param spectrumIndex The workspace spectrum index of the bin
 *  @param binIndex      The index of the bin in the spectrum
 *  @param weight        'How heavily' the bin is to be masked. =1 for full masking (the default).
 */
void MatrixWorkspace::maskBin(const int& spectrumIndex, const int& binIndex, const double& weight)
{
  // First check the spectrumIndex is valid
  if (spectrumIndex < 0 || spectrumIndex >= this->getNumberHistograms() )
    throw Kernel::Exception::IndexError(spectrumIndex,this->getNumberHistograms(),"MatrixWorkspace::maskBin,spectrumIndex");
  // Then check the bin index
  if (binIndex < 0 || binIndex>= this->blocksize() )
    throw Kernel::Exception::IndexError(binIndex,this->blocksize(),"MatrixWorkspace::maskBin,binIndex");
  
  // Writing to m_masks is not thread-safe, so put in some protection
  PARALLEL_CRITICAL(maskBin)
  {
    // If a mask for this bin already exists, it would be replaced. But I think that is OK.
    // First get a reference to the list for this spectrum (or create a new list)
    MatrixWorkspace::MaskList& specList = m_masks[spectrumIndex];
    // Add the new value. Will automatically be put in the right place (ordered by binIndex)
    specList.insert( std::make_pair(binIndex,weight) );
  }

  this->dataY(spectrumIndex)[binIndex] *= (1-weight);
  // Do we want to scale the error?
  this->dataE(spectrumIndex)[binIndex] *= (1-weight);
}

/** Does this spectrum contain any masked bins 
 *  @param spectrumIndex The workspace spectrum index to test
 *  @return True if there are masked bins for this spectrum
 */
bool MatrixWorkspace::hasMaskedBins(const int& spectrumIndex) const
{
  // First check the spectrumIndex is valid. Return false if it isn't (decided against throwing here).
  if (spectrumIndex < 0 || spectrumIndex >= this->getNumberHistograms() ) return false;
  return (m_masks.find(spectrumIndex)==m_masks.end()) ? false : true;
}

/** Returns the list of masked bins for a spectrum. 
 *  @param  spectrumIndex
 *  @return A const reference to the list of masked bins
 *  @throw  Kernel::Exception::IndexError if there are no bins masked for this spectrum (so call hasMaskedBins first!)
 */
const MatrixWorkspace::MaskList& MatrixWorkspace::maskedBins(const int& spectrumIndex) const
{
  std::map<int,MaskList>::const_iterator it = m_masks.find(spectrumIndex);
  // Throw if there are no masked bins for this spectrum. The caller should check first using hasMaskedBins!
  if (it==m_masks.end())
  {
    g_log.error() << "There are no masked bins for spectrum index " << spectrumIndex << std::endl;
    throw Kernel::Exception::IndexError(spectrumIndex,0,"MatrixWorkspace::maskedBins");
  }
  
  return it->second;
}

long int MatrixWorkspace::getMemorySize() const
{
  return 3*size()*sizeof(double)/1024;
}

/** Add parameters to the instrument parameter map that are defined in instrument
*   definition file and for which logfile data are available. Logs must be loaded 
*   before running this method.
*/
void MatrixWorkspace::populateInstrumentParameters()
{
    // Get instrument and sample

    boost::shared_ptr<Instrument> instrument = getBaseInstrument();

    // Get the data in the logfiles associated with the raw data

    const std::vector<Kernel::Property*>& logfileProp = sample().getLogData();


    // Get pointer to parameter map that we may add parameters to and information about
    // the parameters that my be specified in the instrument definition file (IDF)

    Geometry::ParameterMap& paramMap = instrumentParameters();
    std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >& paramInfoFromIDF = instrument->getLogfileCache();


    // iterator to browse throw the multimap: paramInfoFromIDF

    std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> > :: const_iterator it;
    std::pair<std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator,
        std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator> ret;


    // loop over all logfiles and see if any of these are associated with parameters in the
    // IDF

    unsigned int N = logfileProp.size();
    for (unsigned int i = 0; i < N; i++)
    {
        // Remove the path, the run number and extension from logfile filename

        std::string logName = logfileProp[i]->name();

        // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map

        ret = paramInfoFromIDF.equal_range(logName);
        for (it=ret.first; it!=ret.second; ++it)
        {
            double value = ((*it).second)->createParamValue(static_cast<Kernel::TimeSeriesProperty<double>*>(logfileProp[i]));

            // special cases of parameter names

            std::string paramN = ((*it).second)->m_paramName;
            if ( paramN.compare("x")==0 || paramN.compare("y")==0 || paramN.compare("z")==0 )
                paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
            else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
            {
                paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
            }
            else
                paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
    }

    // Check if parameters have been specified using the 'value' attribute rather than the 'logfile-id' attribute
    // All such parameters have been stored using the key = "".
    ret = paramInfoFromIDF.equal_range("");
    Kernel::TimeSeriesProperty<double>* dummy = NULL;
    for (it = ret.first; it != ret.second; ++it)
    {
      double value = ((*it).second)->createParamValue(dummy);

      // special case of parameter names

      std::string paramN = ((*it).second)->m_paramName;
      std::string category = ((*it).second)->m_type;
      if ( category.compare("fitting") == 0 )
      {
        std::ostringstream str;
        str << value << " , " << ((*it).second)->m_fittingFunction << " , " << ((*it).second)->m_constraint << " , " << ((*it).second)->m_tie;
        paramMap.add("FitParameter",((*it).second)->m_component, paramN, str.str());
      }
      else
      {
      if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
        paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
      else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )        
        paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
      else
        paramMap.addDouble(((*it).second)->m_component, paramN, value);
      }
    }
}


} // namespace API
} // Namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::MatrixWorkspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::MatrixWorkspace>;

namespace Mantid
{
namespace Kernel
{

template<> DLLExport
Mantid::API::MatrixWorkspace_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type";
    throw std::runtime_error(message);
  }
}

template<> DLLExport
Mantid::API::MatrixWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_const_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return prop->operator()();
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type";
    throw std::runtime_error(message);
  }
}


} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
