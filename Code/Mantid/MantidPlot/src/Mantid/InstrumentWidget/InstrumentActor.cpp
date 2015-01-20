#include "InstrumentActor.h"
#include "CompAssemblyActor.h"
#include "ObjComponentActor.h"
#include "SampleActor.h"
#include "ComponentActor.h"
#include "ObjCompAssemblyActor.h"
#include "RectangularDetectorActor.h"
#include "GLActorVisitor.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/string.hpp>

#include <QSettings>
#include <QMessageBox>

#include <numeric>
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/ReadLock.h"

using namespace Mantid::Kernel::Exception;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

/// to be used in std::transform
struct Sqrt
{
  double operator()(double x)
  {
    return sqrt(x);
  }
};

double InstrumentActor::m_tolerance = 0.00001;

/**
 * Constructor. Creates a tree of GLActors. Each actor is responsible for displaying insrument components in 3D.
 * Some of the components have "pick ID" assigned to them. Pick IDs can be uniquely converted to a RGB colour value
 * which in turn can be used for picking the component from the screen.
 * @param wsName :: Workspace name
 * @param autoscaling :: True to start with autoscaling option on. If on the min and max of
 *   the colormap scale are defined by the min and max of the data.
 * @param scaleMin :: Minimum value of the colormap scale. Used to assign detector colours. Ignored if autoscaling == true.
 * @param scaleMax :: Maximum value of the colormap scale. Used to assign detector colours. Ignored if autoscaling == true.
 */
InstrumentActor::InstrumentActor(const QString &wsName, bool autoscaling, double scaleMin, double scaleMax):
m_workspace(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString())),
m_ragged(true),
m_autoscaling(autoscaling),
m_defaultPos(),
m_maskedColor(100,100,100),
m_failedColor(200,200,200)
{
  // settings
  loadSettings();

  auto sharedWorkspace = m_workspace.lock();
  if (!sharedWorkspace)
    throw std::logic_error("InstrumentActor passed a workspace that isn't a MatrixWorkspace");

  // set up the color map
  if (!m_currentColorMapFilename.isEmpty())
  {
    loadColorMap(m_currentColorMapFilename,false);
  }
  m_colorMap.changeScaleType(m_scaleType);

  // set up data ranges and colours
  setUpWorkspace(sharedWorkspace, scaleMin, scaleMax);

  Instrument_const_sptr instrument = getInstrument();

  // If the instrument is empty, maybe only having the sample and source
  const int nelements = instrument->nelements();
  if ( ( nelements == 0 ) ||
       ( nelements == 1 && ( instrument->getSource() || instrument->getSample() ) ) ||
       ( nelements == 2 && instrument->getSource() && instrument->getSample() )
     )
  {
    QMessageBox::warning(NULL,"MantidPlot - Warning","This instrument appears to contain no detectors","OK");
  }

  // this adds actors for all instrument components to the scene and fills in m_detIDs
  m_scene.addActor(new CompAssemblyActor(*this,instrument->getComponentID()));
  setupPickColors();

  if ( !m_showGuides )
  {
    // hide guide and other components
    showGuides( m_showGuides );
  }
}

/**
 * Destructor
 */
InstrumentActor::~InstrumentActor()
{
  saveSettings();
}

/**
  * Set up the workspace: calculate the value ranges, set the colours.
  * @param sharedWorkspace :: A shared pointer to the workspace.
  * @param scaleMin :: Minimum limit on the color map axis. If autoscale this value is ignored.
  * @param scaleMax :: Maximum limit on the color map axis. If autoscale this value is ignored.
  */
void InstrumentActor::setUpWorkspace(boost::shared_ptr<const Mantid::API::MatrixWorkspace> sharedWorkspace, double scaleMin, double scaleMax)
{
  const size_t nHist = sharedWorkspace->getNumberHistograms();
  m_WkspBinMinValue = DBL_MAX;
  m_WkspBinMaxValue = -DBL_MAX;
  for (size_t i = 0; i < nHist; ++i)
  {
    const Mantid::MantidVec & values = sharedWorkspace->readX(i);
    double xtest = values.front();
    if( xtest != std::numeric_limits<double>::infinity() )
    {

      if( xtest < m_WkspBinMinValue )
      {
        m_WkspBinMinValue = xtest;
      }
      else if( xtest > m_WkspBinMaxValue )
      {
        m_WkspBinMaxValue = xtest;
      }
      else {}
    }

    xtest = values.back();
    if( xtest != std::numeric_limits<double>::infinity() )
    {
      if( xtest < m_WkspBinMinValue )
      {
        m_WkspBinMinValue = xtest;
      }
      else if( xtest > m_WkspBinMaxValue )
      {
        m_WkspBinMaxValue = xtest;
      }
      else {}
    }
  }

  // set some values as the variables will be used 
  m_DataPositiveMinValue = DBL_MAX;
  m_DataMinValue = -DBL_MAX;
  m_DataMaxValue =  DBL_MAX;

  if ( !m_autoscaling )
  {
    setDataMinMaxRange( scaleMin, scaleMax );
  }
  setDataIntegrationRange(m_WkspBinMinValue,m_WkspBinMaxValue);
  resetColors();

  // set the ragged flag using a workspace validator
  auto wsValidator = Mantid::API::CommonBinsValidator();
  m_ragged = ! wsValidator.isValid(sharedWorkspace).empty();

  /// Keep the pointer to the detid2index map
  m_detid2index_map = sharedWorkspace->getDetectorIDToWorkspaceIndexMap();

}

/** Used to set visibility of an actor corresponding to a particular component
 * When selecting a component in the InstrumentTreeWidget
 *
 * @param visitor :: Visitor to be accepted bu this actor.
 * @param rule :: A rule defining visitor acceptance by assembly actors.
 */
bool InstrumentActor::accept(GLActorVisitor& visitor, VisitorAcceptRule rule)
{
  bool ok = m_scene.accept(visitor, rule);
  visitor.visit(this);
  invalidateDisplayLists();
  return ok;
}

bool InstrumentActor::accept(GLActorConstVisitor &visitor, GLActor::VisitorAcceptRule rule) const
{
    bool ok = m_scene.accept(visitor, rule);
    visitor.visit(this);
    return ok;
}

void InstrumentActor::setChildVisibility(bool on)
{
    m_scene.setChildVisibility(on);
    auto guidesVisitor = SetVisibleNonDetectorVisitor(m_showGuides);
    m_scene.accept( guidesVisitor );
}

bool InstrumentActor::hasChildVisible() const
{
    return m_scene.hasChildVisible();
}

/** Returns the workspace relating to this instrument view.
 *  !!!! DON'T USE THIS TO GET HOLD OF THE INSTRUMENT !!!!
 *  !!!! USE InstrumentActor::getInstrument() BELOW !!!!
 */
MatrixWorkspace_const_sptr InstrumentActor::getWorkspace() const
{
  auto sharedWorkspace = m_workspace.lock();

  if ( !sharedWorkspace )
  {
    throw std::runtime_error("Instrument view: workspace doesn't exist");
  }

  return sharedWorkspace;
}

/** Returns the mask workspace relating to this instrument view as a MatrixWorkspace
 */
MatrixWorkspace_sptr InstrumentActor::getMaskMatrixWorkspace() const
{
    if ( !m_maskWorkspace )
    {
        initMaskHelper();
    }
    return m_maskWorkspace;
}

/**
  * Returns the mask workspace relating to this instrument view as a IMaskWorkspace.
  * Guarantees to return a valid pointer
  */
IMaskWorkspace_sptr InstrumentActor::getMaskWorkspace() const
{
    if ( !m_maskWorkspace )
    {
        initMaskHelper();
    }
    return boost::dynamic_pointer_cast<IMaskWorkspace>( m_maskWorkspace );
}

/**
  * Returns the mask workspace relating to this instrument view as a IMaskWorkspace
  * if it exists or empty pointer if it doesn't.
  */
IMaskWorkspace_sptr InstrumentActor::getMaskWorkspaceIfExists() const
{
    if ( !m_maskWorkspace ) return IMaskWorkspace_sptr();
    return boost::dynamic_pointer_cast<IMaskWorkspace>( m_maskWorkspace );
}

/**
  * Apply mask stored in the helper mask workspace to the data workspace.
  */
void InstrumentActor::applyMaskWorkspace()
{
    if ( !m_maskWorkspace ) return;
    try
    {
        Mantid::API::IAlgorithm * alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("MaskDetectors",-1);
        alg->setPropertyValue( "Workspace", getWorkspace()->name() );
        alg->setProperty( "MaskedWorkspace", m_maskWorkspace );
        alg->execute();
        // After the algorithm finishes the InstrumentWindow catches the after-replace notification
        // and updates this instrument actor.
    }
    catch(...)
    {
        QMessageBox::warning(NULL,"MantidPlot - Warning","An error accured when applying the mask.","OK");
    }
    clearMaskWorkspace();
}

/**
  * Removes the mask workspace.
  */
void InstrumentActor::clearMaskWorkspace()
{
    bool needColorRecalc = false;
    if ( m_maskWorkspace )
    {
        needColorRecalc = getMaskWorkspace()->getNumberMasked() > 0;
    }
    m_maskWorkspace.reset();
    if ( needColorRecalc )
    {
        resetColors();
    }
}

Instrument_const_sptr InstrumentActor::getInstrument() const
{
    Instrument_const_sptr retval;

    // Look to see if we have set the property in the properties file
    // to define our 'default' view
    std::string view = Mantid::Kernel::ConfigService::Instance().getString("instrument.view.geometry");

    auto sharedWorkspace = getWorkspace();
    Mantid::Kernel::ReadLock _lock(*sharedWorkspace);

    if ( boost::iequals("Default", view) || boost::iequals("Physical", view))
    {      
        // First see if there is a 'physical' instrument available. Use it if there is.
        retval = sharedWorkspace->getInstrument()->getPhysicalInstrument();
    }
    else if (boost::iequals("Neutronic", view))
    {
        retval = sharedWorkspace->getInstrument();
    }

    if ( ! retval )
    {
        // Otherwise get hold of the 'main' instrument and use that
        retval = sharedWorkspace->getInstrument();
    }

    return retval;
}

const MantidColorMap& InstrumentActor::getColorMap() const
{
  return m_colorMap;
}

IDetector_const_sptr InstrumentActor::getDetector(size_t i) const
{
  try
  {
    // Call the local getInstrument, NOT the one on the workspace
    return this->getInstrument()->getDetector(m_detIDs.at(i));
  }
  catch(...)
  {
    return IDetector_const_sptr();
  }
  // Add line that can never be reached to quiet compiler complaints
  return IDetector_const_sptr();
}

Mantid::detid_t InstrumentActor::getDetID(size_t pickID)const
{
  if ( pickID < m_detIDs.size() )
  {
    return m_detIDs[pickID];
  }
  return -1;
}

/**
 * Get a component id of a picked component.
 */
Mantid::Geometry::ComponentID InstrumentActor::getComponentID(size_t pickID) const
{
  size_t ndet = m_detIDs.size();
  if ( pickID < ndet )
  {
    auto det = getDetector( m_detIDs[pickID] );
    return det->getComponentID();
  }
  else if (pickID < ndet + m_nonDetIDs.size())
  {
    return m_nonDetIDs[pickID - ndet];
  }
  return Mantid::Geometry::ComponentID();
}

/** Retrieve the workspace index corresponding to a particular detector
 *  @param id The detector id
 *  @returns  The workspace index containing data for this detector
 *  @throws Exception::NotFoundError If the detector is not represented in the workspace
 */
size_t InstrumentActor::getWorkspaceIndex(Mantid::detid_t id) const
{
  auto mapEntry = m_detid2index_map.find(id);
  if ( mapEntry == m_detid2index_map.end() )
  {
    throw Kernel::Exception::NotFoundError("Detector ID not in workspace",id);
  }

  return mapEntry->second;
}

/**
 * Set an interval in the data workspace x-vector's units in which the data are to be
 * integrated to calculate the detector colours.
 *
 * @param xmin :: The lower bound.
 * @param xmax :: The upper bound.
 */
void InstrumentActor::setIntegrationRange(const double& xmin,const double& xmax)
{
  setDataIntegrationRange(xmin, xmax);
  resetColors();
}

/** Gives the total signal in the spectrum relating to the given detector
 *  @param id The detector id
 *  @return The signal, or -1 if the detector is not represented in the workspace
 */
double InstrumentActor::getIntegratedCounts(Mantid::detid_t id)const
{
  try {
    size_t i = getWorkspaceIndex(id);
    return m_specIntegrs.at(i);
  } catch (NotFoundError &) {
    // If the detector is not represented in the workspace
    return -1.0;
    }
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on the x-axis.
 * Checks (approximately) if the workspace is ragged or not and uses the appropriate summation
 * method.
 *
 * @param dets :: A list of detector IDs to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 * @param size :: (optional input) Size of the output vectors. If not given it will be determined automatically.
 */
void InstrumentActor::sumDetectors(QList<int> &dets, std::vector<double> &x, std::vector<double> &y, size_t size) const
{
  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  if ( size > ws->blocksize() || size == 0 )
  {
    size = ws->blocksize();
  }

  if ( m_ragged )
  {
    // could be slower than uniform
    sumDetectorsRagged( dets, x, y, size );
  }
  else
  {
    // should be faster than ragged
    sumDetectorsUniform( dets, x, y );
  }
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on the x-axis.
 * Assumes that all spectra share the x vector.
 *
 * @param dets :: A list of detector IDs to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 */
void InstrumentActor::sumDetectorsUniform(QList<int>& dets, std::vector<double>&x, std::vector<double>&y) const
{

    size_t wi;
    bool isDataEmpty = dets.isEmpty();

    if ( !isDataEmpty )
    {
        try {
            wi = getWorkspaceIndex( dets[0] );
        } catch (Mantid::Kernel::Exception::NotFoundError &) {
          isDataEmpty = true; // Detector doesn't have a workspace index relating to it
        }
    }

    if ( isDataEmpty )
    {
        x.clear();
        y.clear();
        return;
    }

    // find the bins inside the integration range
    size_t imin,imax;
    getBinMinMaxIndex(wi,imin,imax);

    Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
    const Mantid::MantidVec& X = ws->readX(wi);
    x.assign(X.begin() + imin, X.begin() + imax);
    if ( ws->isHistogramData() )
    {
      // calculate the bin centres
      std::transform(x.begin(),x.end(),X.begin() + imin + 1,x.begin(),std::plus<double>());
      std::transform(x.begin(),x.end(),x.begin(),std::bind2nd(std::divides<double>(),2.0));
    }
    y.resize(x.size(),0);
    // sum the spectra
    foreach(int id, dets)
    {
        try {
          size_t index = getWorkspaceIndex( id );
          const Mantid::MantidVec& Y = ws->readY(index);
          std::transform(y.begin(),y.end(),Y.begin() + imin,y.begin(),std::plus<double>());
        } catch (Mantid::Kernel::Exception::NotFoundError &) {
          continue; // Detector doesn't have a workspace index relating to it
        }
    }
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on the x-axis.
 * Assumes that all spectra have different x vectors.
 *
 * @param dets :: A list of detector IDs to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 * @param size :: (input) Size of the output vectors.
 */
void InstrumentActor::sumDetectorsRagged(QList<int> &dets, std::vector<double> &x, std::vector<double> &y, size_t size) const
{
  if ( dets.isEmpty() || size == 0 )
  {
      x.clear();
      y.clear();
      return;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  //  create a workspace to hold the data from the selected detectors
  Mantid::API::MatrixWorkspace_sptr dws = Mantid::API::WorkspaceFactory::Instance().create(ws,dets.size());

  // x-axis limits
  double xStart = maxBinValue();
  double xEnd = minBinValue();

  size_t nSpec = 0; // number of actual spectra to add
  // fill in the temp workspace with the data from the detectors
  foreach(int id, dets)
  {
      try {
        size_t index = getWorkspaceIndex( id );
        dws->dataX(nSpec) = ws->readX(index);
        dws->dataY(nSpec) = ws->readY(index);
        dws->dataE(nSpec) = ws->readE(index);
        double xmin = dws->readX(nSpec).front();
        double xmax = dws->readX(nSpec).back();
        if ( xmin < xStart )
        {
          xStart = xmin;
        }
        if ( xmax > xEnd )
        {
          xEnd = xmax;
        }
        ++nSpec;
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        continue; // Detector doesn't have a workspace index relating to it
      }
  }

  if ( nSpec == 0 )
  {
      x.clear();
      y.clear();
      return;
  }

  // limits should exceed the integration range
  if ( xStart < minBinValue() )
  {
    xStart = minBinValue();
  }

  if ( xEnd > maxBinValue() )
  {
    xEnd = maxBinValue();
  }

  double dx = (xEnd - xStart) / static_cast<double>(size - 1);
  std::string params = QString("%1,%2,%3").arg(xStart).arg(dx).arg(xEnd).toStdString();
  std::string outName = "_TMP_sumDetectorsRagged";

  try
  {
    // rebin all spectra to the same binning
    Mantid::API::IAlgorithm * alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("Rebin",-1);
    alg->setProperty( "InputWorkspace", dws );
    alg->setPropertyValue( "OutputWorkspace", outName );
    alg->setPropertyValue( "Params", params );
    alg->execute();

    ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outName));
    Mantid::API::AnalysisDataService::Instance().remove( outName );

    x = ws->readX(0);
    y = ws->readY(0);
    // add the spectra
    for(size_t i = 0; i < nSpec; ++i)
    {
      const Mantid::MantidVec& Y = ws->readY(i);
      std::transform( y.begin(), y.end(), Y.begin(), y.begin(), std::plus<double>() );
    }
  }
  catch(std::invalid_argument&)
  {
    // wrong Params for any reason
    x.resize(size,(xEnd + xStart)/2);
    y.resize(size,0.0);
  }

}

/**
 * Recalculate the detector colors based on the integrated values in m_specIntegrs and
 * the masking information in ....
 */
void InstrumentActor::resetColors()
{
  QwtDoubleInterval qwtInterval(m_DataMinScaleValue,m_DataMaxScaleValue);
  m_colors.resize(m_specIntegrs.size());

  auto sharedWorkspace = getWorkspace();

  Instrument_const_sptr inst = getInstrument();
  IMaskWorkspace_sptr mask = getMaskWorkspaceIfExists();

  //PARALLEL_FOR1(m_workspace)
  for (int iwi=0; iwi < int(m_specIntegrs.size()); iwi++)
  {
    size_t wi = size_t(iwi);
    double integratedValue = m_specIntegrs[wi];
    try
    {
      // Find if the detector is masked
      const std::set<detid_t>& dets = sharedWorkspace->getSpectrum(wi)->getDetectorIDs();
      bool masked = false;

      if ( mask )
      {
        masked = mask->isMasked( dets );
      }
      else
      {
        masked = inst->isDetectorMasked(dets);
      }

      if (masked)
      {
        m_colors[wi] = m_maskedColor;
      }
      else
      {
        QRgb color = m_colorMap.rgb(qwtInterval,integratedValue);
        m_colors[wi] = GLColor(qRed(color), qGreen(color), qBlue(color));
      }
    }
    catch(NotFoundError &)
    {
      m_colors[wi] = m_failedColor;
      continue;
    }
  }
  if (m_scene.getNumberOfActors() > 0)
  {
    dynamic_cast<CompAssemblyActor*>(m_scene.getActor(0))->setColors();
    invalidateDisplayLists();
  }
  emit colorMapChanged();
}

void InstrumentActor::updateColors()
{
  setIntegrationRange(m_BinMinValue,m_BinMaxValue);
  resetColors();
}

/**
 * @param on :: True or false for on or off.
 */
void InstrumentActor::showGuides(bool on)
{
  auto visitor = SetVisibleNonDetectorVisitor( on );
  this->accept( visitor );
  m_showGuides = on;
}

GLColor InstrumentActor::getColor(Mantid::detid_t id)const
{
  try {
    size_t i = getWorkspaceIndex(id);
    return m_colors.at(i);
  } catch (NotFoundError &) {
    // Return the first color if the detector is not represented in the workspace
    return m_colors.front();
  }
}

void InstrumentActor::draw(bool picking)const
{
  m_scene.draw(picking);
}

/**
 * @param fname :: A color map file name.
 * @param reset_colors :: An option to reset the detector colors.
 */
void InstrumentActor::loadColorMap(const QString& fname,bool reset_colors)
{
  m_colorMap.loadMap(fname);
  m_currentColorMapFilename = fname;
  if (reset_colors)
  {
    resetColors();
  }
}

//------------------------------------------------------------------------------
/** Add a detector ID to the pick list (m_detIDs)
 * The order of detids define the pickIDs for detectors.
 *
 * @param id :: detector ID to add.
 * @return pick ID of the added detector
 */
size_t InstrumentActor::pushBackDetid(Mantid::detid_t id)const
{
  m_detIDs.push_back(id);
  return m_detIDs.size() - 1;
}

//------------------------------------------------------------------------------
/** Add a non-detector component ID to the pick list (m_nonDetIDs)
 *
 * @param actor :: ObjComponentActor for the component added.
 * @param compID :: component ID to add.
 */
void InstrumentActor::pushBackNonDetid(ObjComponentActor* actor, Mantid::Geometry::ComponentID compID)const
{
  m_nonDetActorsTemp.push_back(actor);
  m_nonDetIDs.push_back(compID);
}

//------------------------------------------------------------------------------
/**
 * Set pick colors to non-detectors strored by calls to pushBackNonDetid().
 */
void InstrumentActor::setupPickColors()
{
  assert( m_nonDetActorsTemp.size() == m_nonDetIDs.size() );
  auto nDets = m_detIDs.size();
  for(size_t i = 0; i < m_nonDetActorsTemp.size(); ++i)
  {
    m_nonDetActorsTemp[i]->setPickColor( makePickColor(nDets + i) );
  }
  m_nonDetActorsTemp.clear();
}

//------------------------------------------------------------------------------
/** If needed, cache the detector positions for all detectors.
 * Call this BEFORE getDetPos().
 * Does nothing if the positions have already been cached.
 */
void InstrumentActor::cacheDetPos() const
{
  if (m_detPos.size() != m_detIDs.size())
  {
    m_detPos.clear();
    for (size_t i=0; i<m_detIDs.size(); i++)
    {
      IDetector_const_sptr det = this->getDetector(i);
      m_detPos.push_back( det->getPos() );
    }
  }
}


//------------------------------------------------------------------------------
/** Get the cached detector position
 *
 * @param pickID :: pick Index maching the getDetector() calls;
 * @return the real-space position of the detector
 */
const Mantid::Kernel::V3D & InstrumentActor::getDetPos(size_t pickID)const
{
  if ( pickID < m_detPos.size() )
  {
    return m_detPos.at(pickID);
  }
  return m_defaultPos;
}

/**
 * @param type :: 0 - linear, 1 - log10.
 */
void InstrumentActor::changeScaleType(int type)
{
  m_colorMap.changeScaleType(static_cast<GraphOptions::ScaleType>(type));
  resetColors();
}

void InstrumentActor::loadSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  m_scaleType = static_cast<GraphOptions::ScaleType>( settings.value("ScaleType", 0 ).toInt() );
  //Load Colormap. If the file is invalid the default stored colour map is used
  m_currentColorMapFilename = settings.value("ColormapFile", "").toString();
  // Set values from settings
  m_showGuides = settings.value("ShowGuides", false).toBool();
  settings.endGroup();
}

void InstrumentActor::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  settings.setValue("ColormapFile", m_currentColorMapFilename);
  settings.setValue("ScaleType", (int)m_colorMap.getScaleType() );
  settings.setValue("ShowGuides", m_showGuides);
  settings.endGroup();
}

void InstrumentActor::setMinValue(double vmin)
{
  if (m_autoscaling) return;
  if (vmin >= m_DataMaxScaleValue) return;
  m_DataMinScaleValue = vmin;
  resetColors();
}

void InstrumentActor::setMaxValue(double vmax)
{
  if (m_autoscaling) return;
  if (vmax <= m_DataMinScaleValue) return;
  m_DataMaxScaleValue = vmax;
  resetColors();
}

void InstrumentActor::setMinMaxRange(double vmin, double vmax)
{
  if (m_autoscaling) return;
  setDataMinMaxRange(vmin,vmax);
  resetColors();
}

bool InstrumentActor::wholeRange()const
{
  return m_BinMinValue == m_WkspBinMinValue && m_BinMaxValue == m_WkspBinMaxValue;
}

/**
 * Set autoscaling of the y axis. If autoscaling is on the minValue() and maxValue()
 * return the actual min and max values in the data. If autoscaling is off
 *  minValue() and maxValue() are fixed and do not change after changing the x integration range.
 * @param on :: On or Off.
 */
void InstrumentActor::setAutoscaling(bool on)
{
  m_autoscaling = on;
  if (on)
  {
    m_DataMinScaleValue = m_DataMinValue;
    m_DataMaxScaleValue = m_DataMaxValue;
    //setIntegrationRange(m_DataMinValue,m_DataMaxValue);
    resetColors();
  }
}

/**
 * Initialize the helper mask workspace with the mask from the data workspace.
 */
void InstrumentActor::initMaskHelper() const
{
  if ( m_maskWorkspace ) return;
  try
  {
    // extract the mask (if any) from the data to the mask workspace
    const std::string maskName = "__InstrumentActor_MaskWorkspace";
    Mantid::API::IAlgorithm * alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("ExtractMask",-1);
    alg->setPropertyValue( "InputWorkspace", getWorkspace()->name() );
    alg->setPropertyValue( "OutputWorkspace", maskName );
    alg->execute();

    m_maskWorkspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(maskName));
    Mantid::API::AnalysisDataService::Instance().remove( maskName );
  }
  catch( ... )
  {
    // don't know what to do here yet ...
    QMessageBox::warning(NULL,"MantidPlot - Warning","An error accured when extracting the mask.","OK");
  }
}

/**
  * Checks if the actor has a mask workspace attached.
  */
bool InstrumentActor::hasMaskWorkspace() const
{
    return m_maskWorkspace ? true : false;
}

/**
  * Find a rotation from one orthonormal basis set (Xfrom,Yfrom,Zfrom) to
  * another orthonormal basis set (Xto,Yto,Zto). Both sets must be right-handed
  * (or same-handed, I didn't check). The method doesn't check the sets for orthogonality
  * or normality. The result is a rotation quaternion such that:
  *   R.rotate(Xfrom) == Xto
  *   R.rotate(Yfrom) == Yto
  *   R.rotate(Zfrom) == Zto
  * @param Xfrom :: The X axis of the original basis set
  * @param Yfrom :: The Y axis of the original basis set
  * @param Zfrom :: The Z axis of the original basis set
  * @param Xto :: The X axis of the final basis set
  * @param Yto :: The Y axis of the final basis set
  * @param Zto :: The Z axis of the final basis set
  * @param R :: The output rotation as a quaternion
  * @param out :: Debug printout flag
  */
void InstrumentActor::BasisRotation(const Mantid::Kernel::V3D& Xfrom,
                const Mantid::Kernel::V3D& Yfrom,
                const Mantid::Kernel::V3D& Zfrom,
                const Mantid::Kernel::V3D& Xto,
                const Mantid::Kernel::V3D& Yto,
                const Mantid::Kernel::V3D& Zto,
                Mantid::Kernel::Quat& R,
                bool out
                )
{
  // Find transformation from (X,Y,Z) to (XX,YY,ZZ)
  // R = R1*R2*R3, where R1, R2, and R3 are Euler rotations

//  std::cerr<<"RCRotation-----------------------------\n";
//  std::cerr<<"From "<<Xfrom<<Yfrom<<Zfrom<<'\n';
//  std::cerr<<"To   "<<Xto<<Yto<<Zto<<'\n';

  double sZ = Zfrom.scalar_prod(Zto);
  if (fabs(sZ - 1) < m_tolerance) // vectors the same
  {
    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance)
    {
      R = Mantid::Kernel::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance)
    {
      R = Mantid::Kernel::Quat(180,Zfrom);
    }
    else
    {
      R = Mantid::Kernel::Quat(Xfrom,Xto);
    }
  }
  else if(fabs(sZ + 1) < m_tolerance) // rotated by 180 degrees
  {
    if (fabs(Xfrom.scalar_prod(Xto)-1) < m_tolerance)
    {
      R = Mantid::Kernel::Quat(180.,Xfrom);
    }
    else if (fabs(Yfrom.scalar_prod(Yto)-1) < m_tolerance)
    {
      R = Mantid::Kernel::Quat(180.,Yfrom);
    }
    else
    {
      R = Mantid::Kernel::Quat(180.,Xto)*Mantid::Kernel::Quat(Xfrom,Xto);
    }
  }
  else
  {
    // Rotation R1 of system (X,Y,Z) around Z by alpha
    Mantid::Kernel::V3D X1;
    Mantid::Kernel::Quat R1;

    X1 = Zfrom.cross_prod(Zto);
    X1.normalize();

    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance)
    {
      R = Mantid::Kernel::Quat(Zfrom,Zto);
      return;
    }

    sX = Xfrom.scalar_prod(X1);
    if (fabs(sX - 1) < m_tolerance)
    {
      R1 = Mantid::Kernel::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
    {
      R1 = Mantid::Kernel::Quat(180.,Zfrom);
    }
    else
    {
      R1 = Mantid::Kernel::Quat(Xfrom,X1);
    }
    if (out)
    std::cerr<<"R1="<<R1<<'\n';

    // Rotation R2 around X1 by beta
    Mantid::Kernel::Quat R2(Zfrom,Zto); // vectors are different
    if (out)
    std::cerr<<"R2="<<R2<<'\n';

    // Rotation R3 around ZZ by gamma
    Mantid::Kernel::Quat R3;
    sX = Xto.scalar_prod(X1);
    if (fabs(sX - 1) < m_tolerance)
    {
      R3 = Mantid::Kernel::Quat();
    }
    else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
    {
      R3 = Mantid::Kernel::Quat(180.,Zto);
    }
    else
    {
      R3 = Mantid::Kernel::Quat(X1,Xto);
    }
    if (out)
    std::cerr<<"R3="<<R3<<'\n';

    // Combined rotation
    R = R3*R2*R1;
  }
}

/**
 * Calculate a rotation to look in a particular direction.
 *
 * @param eye :: A direction to look in
 * @param up :: A vector showing the 'up' direction after the rotation. It doesn't have to be normal to eye
 *   just non-collinear. If up is collinear to eye the actual 'up' direction is undefined.
 * @param R :: The result rotation.
 */
void InstrumentActor::rotateToLookAt(const Mantid::Kernel::V3D &eye, const Mantid::Kernel::V3D &up, Mantid::Kernel::Quat &R)
{
    if ( eye.nullVector() )
    {
        throw std::runtime_error("The eye vector is null in InstrumentActor::rotateToLookAt.");
    }

    // Basis vectors of the OpenGL reference frame. Z points into the screen, Y points up.
    const Mantid::Kernel::V3D X(1,0,0);
    const Mantid::Kernel::V3D Y(0,1,0);
    const Mantid::Kernel::V3D Z(0,0,1);

    Mantid::Kernel::V3D x,y,z;
    z = eye;
    z.normalize();
    y = up;
    x = y.cross_prod(z);
    if (x.nullVector())
    {
        // up || eye
        if ( z.X() != 0.0 )
        {
            x.setY(1.0);
        }
        else if ( z.Y() != 0.0 )
        {
            x.setZ(1.0);
        }
        else
        {
            x.setX(1.0);
        }
    }
    x.normalize();
    y = z.cross_prod(x);

    BasisRotation(x,y,z,X,Y,Z,R);
}

/**
 * Find the offsets in the spectrum's x vector of the bounds of integration.
 * @param wi :: The works[ace index of the spectrum.
 * @param imin :: Index of the lower bound: x_min == readX(wi)[imin]
 * @param imax :: Index of the upper bound: x_max == readX(wi)[imax]
 */
void InstrumentActor::getBinMinMaxIndex( size_t wi, size_t& imin, size_t& imax ) const
{
  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  const Mantid::MantidVec& x = ws->readX(wi);
  Mantid::MantidVec::const_iterator x_begin = x.begin();
  Mantid::MantidVec::const_iterator x_end = x.end();
  if (x_begin == x_end)
  {
    throw std::runtime_error("No bins found to plot");
  }
  if (ws->isHistogramData())
  {
    --x_end;
  }
  if ( wholeRange() )
  {
    imin = 0;
    imax = static_cast<size_t>(x_end - x_begin);
  }
  else
  {
    Mantid::MantidVec::const_iterator x_from = std::lower_bound( x_begin, x_end, minBinValue() );
    Mantid::MantidVec::const_iterator x_to = std::upper_bound( x_begin, x_end, maxBinValue() );
    imin = static_cast<size_t>(x_from - x_begin);
    imax = static_cast<size_t>(x_to - x_begin);
    if (imax <= imin)
    {
      if (x_from == x_end)
      {
        --x_from;
        x_to = x_end;
      }
      else
      {
        x_to = x_from + 1;
      }
      imin = static_cast<size_t>(x_from - x_begin);
      imax = static_cast<size_t>(x_to - x_begin);
    }
  }
}

/**
  * Set the minimum and the maximum data values on the color map scale.
  */
void InstrumentActor::setDataMinMaxRange(double vmin, double vmax)
{
  if (vmin < m_DataMinValue)
  {
    vmin = m_DataMinValue;
  }
  if (vmin >= vmax) return;
  m_DataMinScaleValue = vmin;
  m_DataMaxScaleValue = vmax;
}

void InstrumentActor::setDataIntegrationRange(const double& xmin,const double& xmax)
{
  m_BinMinValue = xmin;
  m_BinMaxValue = xmax;

  auto workspace = getWorkspace();
  //Use the workspace function to get the integrated spectra
  workspace->getIntegratedSpectra(m_specIntegrs, m_BinMinValue, m_BinMaxValue, wholeRange());

  // get the workspace indices of monitors in order to exclude them from finding of the max value
  auto monitorIDs = getInstrument()->getMonitors();
  std::vector<size_t> monitorIndices;
  workspace->getIndicesFromDetectorIDs( monitorIDs, monitorIndices );

  // check that there is at least 1 non-monitor spectrum
  if ( monitorIndices.size() == m_specIntegrs.size() )
  {
      // there are only monitors - cannot skip them
      monitorIndices.clear();
  }

  if ( m_specIntegrs.empty() )
  {
      // in case there are no spectra set some arbitrary values
      m_DataMinValue = 1.0;
      m_DataMaxValue = 10.0;
      m_DataPositiveMinValue = 1.0;
  }
  else
  {
      m_DataMinValue = DBL_MAX;
      m_DataMaxValue = -DBL_MAX;

      //Now we need to convert to a vector where each entry is the sum for the detector ID at that spot (in integrated_values).
      for (size_t i=0; i < m_specIntegrs.size(); i++)
      {
        // skip the monitors
        if ( std::find( monitorIndices.begin(), monitorIndices.end(), i ) != monitorIndices.end() )
        {
          continue;
        }
        double sum = m_specIntegrs[i];
        if( boost::math::isinf(sum) || boost::math::isnan(sum) )
        {
          throw std::runtime_error("The workspace contains values that cannot be displayed (infinite or NaN).\n"
                                   "Please run ReplaceSpecialValues algorithm for correction.");
        }
        //integrated_values[i] = sum;
        if( sum < m_DataMinValue )
        {
          m_DataMinValue = sum;
        }
        if( sum > m_DataMaxValue )
        {
          m_DataMaxValue = sum;
        }
        if (sum > 0 && sum < m_DataPositiveMinValue)
        {
          m_DataPositiveMinValue = sum;
        }
      }
  }

  if (m_autoscaling)
  {
    m_DataMinScaleValue = m_DataMinValue;
    m_DataMaxScaleValue = m_DataMaxValue;
  }
}

//-------------------------------------------------------------------------//
bool SetVisibleComponentVisitor::visit(GLActor* actor)
{
    actor->setVisibility(false);
    return false;
}

bool SetVisibleComponentVisitor::visit(GLActorCollection *actor)
{
    bool visible = actor->hasChildVisible();
    actor->setVisibility(visible);
    return visible;
}

bool SetVisibleComponentVisitor::visit(ComponentActor *actor)
{
    bool on = actor->getComponent()->getComponentID() == m_id;
    actor->setVisibility(on);
    return on;
}

bool SetVisibleComponentVisitor::visit(CompAssemblyActor *actor)
{
    bool visible = false;
    if ( actor->getComponent()->getComponentID() == m_id )
    {
        visible = true;
        actor->setChildVisibility(true);
    }
    else
    {
        visible = actor->hasChildVisible();
        actor->setVisibility(visible);
    }
    return visible;
}

bool SetVisibleComponentVisitor::visit(ObjCompAssemblyActor *actor)
{
    bool on = actor->getComponent()->getComponentID() == m_id;
    actor->setVisibility(on);
    return on;
}

bool SetVisibleComponentVisitor::visit(InstrumentActor *actor)
{
    bool visible = false;
    if ( actor->getInstrument()->getComponentID() == m_id )
    {
        visible = true;
        actor->setChildVisibility(true);
    }
    else
    {
        visible = actor->hasChildVisible();
        actor->setVisibility(visible);
    }
    return visible;
}

bool SetVisibleComponentVisitor::visit(RectangularDetectorActor *actor)
{
    bool on = actor->getComponent()->getComponentID() == m_id || actor->isChildDetector(m_id);
    actor->setVisibility(on);
    return on;
}

//-------------------------------------------------------------------------//
/**
 * Visits an actor and if it is a "non-detector" sets its visibility.
 *
 * @param actor :: A visited actor.
 * @return always false to traverse all the instrument tree.
 */
bool SetVisibleNonDetectorVisitor::visit(GLActor* actor)
{
  ComponentActor* comp = dynamic_cast<ComponentActor*>(actor);
  if ( comp && comp->isNonDetector() )
  {
    actor->setVisibility(m_on);
  }
  return false;
}

//-------------------------------------------------------------------------//
bool FindComponentVisitor::visit(GLActor* actor)
{
  ComponentActor* comp = dynamic_cast<ComponentActor*>(actor);
  if (comp)
  {
    if (comp->getComponent()->getComponentID() == m_id)
    {
      m_actor = comp;
      return true;
    }
  }
  return false;
}
