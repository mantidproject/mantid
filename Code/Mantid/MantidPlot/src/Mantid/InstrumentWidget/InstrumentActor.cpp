#include "InstrumentActor.h"
#include "CompAssemblyActor.h"
#include "ObjComponentActor.h"
#include "SampleActor.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

#include <boost/math/special_functions/fpclassify.hpp>

#include <QSettings>
#include <QMessageBox>

#include <numeric>
#include "MantidGeometry/IDTypes.h"

using namespace Mantid::Kernel::Exception;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

double InstrumentActor::m_tolerance = 0.00001;

/**
 * Constructor
 * @param wsName :: Workspace
 */
InstrumentActor::InstrumentActor(const QString &wsName):
m_workspace(boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()))),
m_autoscaling(true),
m_maskedColor(100,100,100),
m_failedColor(200,200,200),
m_sampleActor(NULL)
{
  if (!m_workspace)
    throw std::logic_error("InstrumentActor passed a workspace that isn't a MatrixWorkspace");

  const size_t nHist = m_workspace->getNumberHistograms();
  m_WkspBinMin = DBL_MAX;
  m_WkspBinMax = -DBL_MAX;
  for (size_t i = 0; i < nHist; ++i)
  {
    const Mantid::MantidVec & values = m_workspace->readX(i);
    double xtest = values.front();
    if( xtest != std::numeric_limits<double>::infinity() )
    {

      if( xtest < m_WkspBinMin )
      {
        m_WkspBinMin = xtest;
      }
      else if( xtest > m_WkspBinMax )
      {
        m_WkspBinMax = xtest;
      }
      else {}
    }

    xtest = values.back();
    if( xtest != std::numeric_limits<double>::infinity() )
    {
      if( xtest < m_WkspBinMin )
      {
        m_WkspBinMin = xtest;
      }
      else if( xtest > m_WkspBinMax )
      {
        m_WkspBinMax = xtest;
      }
      else {}
    }
  }
  m_WkspDataPositiveMin = DBL_MAX;
  loadSettings();

  blockSignals(true);
  setIntegrationRange(m_WkspBinMin,m_WkspBinMax);
  blockSignals(false);

  m_id2wi_map.reset(m_workspace->getDetectorIDToWorkspaceIndexMap(false));

  // If the instrument is empty, maybe only having the sample and source
  if (getInstrument()->nelements() < 3)
  {
    QMessageBox::warning(NULL,"MantidPlot - Warning","The instrument is probably empty","OK");
  }

  // this adds actors for all instrument components to the scene and fills in m_detIDs
  m_scene.addActor(new CompAssemblyActor(*this,getInstrument()->getComponentID()));

  FindComponentVisitor findVisitor(getInstrument()->getSample()->getComponentID());
  accept(findVisitor);
  const ObjComponentActor* samplePosActor = dynamic_cast<const ObjComponentActor*>(findVisitor.getActor());

  m_sampleActor = new SampleActor(*this,m_workspace->sample(),samplePosActor);
  m_scene.addActor(m_sampleActor);
}

/**
 * Destructor
 */
InstrumentActor::~InstrumentActor()
{
  saveSettings();
}

bool InstrumentActor::accept(const GLActorVisitor& visitor)
{
  bool ok = m_scene.accept(visitor);
  const SetVisibilityVisitor* vv = dynamic_cast<const SetVisibilityVisitor*>(&visitor);
  if (vv && m_sampleActor)
  {
    m_sampleActor->setVisibility(m_sampleActor->getSamplePosActor()->isVisible());
  }
  invalidateDisplayLists();
  return ok;
}

/** Returns the workspace relating to this instrument view.
 *  !!!! DON'T USE THIS TO GET HOLD OF THE INSTRUMENT !!!!
 *  !!!! USE InstrumentActor::getInstrument() BELOW !!!!
 */
MatrixWorkspace_const_sptr InstrumentActor::getWorkspace() const
{
  return m_workspace;
}

Instrument_const_sptr InstrumentActor::getInstrument() const
{
  // First see if there is a 'physical' instrument available. Use it if there is.
  Instrument_const_sptr retval = m_workspace->getInstrument()->getPhysicalInstrument();
  if ( ! retval )
  {
    // Otherwise get hold of the 'main' instrument and use that
    retval = m_workspace->getInstrument();
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

/** Retrieve the workspace index corresponding to a particular detector
 *  @param id The detector id
 *  @returns  The workspace index containing data for this detector
 *  @throws Exception::NotFoundError If the detector is not represented in the workspace
 */
size_t InstrumentActor::getWorkspaceIndex(Mantid::detid_t id) const
{
  Mantid::detid2index_map::const_iterator it = m_id2wi_map->find(id);
  if ( it == m_id2wi_map->end() )
  {
    throw NotFoundError("No workspace index for detector",id);
  }
  return it->second;
}

void InstrumentActor::setIntegrationRange(const double& xmin,const double& xmax)
{
  if (!getWorkspace()) return;

  m_BinMinValue = xmin;
  m_BinMaxValue = xmax;

  bool binEntireRange = m_BinMinValue == m_WkspBinMin && m_BinMaxValue == m_WkspBinMax;

  //Use the workspace function to get the integrated spectra
  getWorkspace()->getIntegratedSpectra(m_specIntegrs, m_BinMinValue, m_BinMaxValue, binEntireRange);

  m_DataMinValue = DBL_MAX;
  m_DataMaxValue = -DBL_MAX;
  
  //Now we need to convert to a vector where each entry is the sum for the detector ID at that spot (in integrated_values).
  for (size_t i=0; i < m_specIntegrs.size(); i++)
  {
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
    if (sum > 0 && sum < m_WkspDataPositiveMin)
    {
      m_WkspDataPositiveMin = sum;
    }
  }

  // No preset value
  if(binEntireRange)
  {
    m_WkspDataMin = m_DataMinValue;
    m_WkspDataMax = m_DataMaxValue;
  }
  if (m_autoscaling)
  {
    m_DataMinScaleValue = m_DataMinValue;
    m_DataMaxScaleValue = m_DataMaxValue;
  }
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

void InstrumentActor::resetColors()
{
  QwtDoubleInterval qwtInterval(m_DataMinScaleValue,m_DataMaxScaleValue);
  m_colors.resize(m_specIntegrs.size());

  Instrument_const_sptr inst = m_workspace->getInstrument();

  //PARALLEL_FOR1(m_workspace)
  for (int iwi=0; iwi < int(m_specIntegrs.size()); iwi++)
  {
    size_t wi = size_t(iwi);
    double integratedValue = m_specIntegrs[wi];
    try
    {
      // FIXME: This getdetector call is very slow.
      Mantid::Geometry::IDetector_const_sptr det = m_workspace->getDetector(wi);
      // FIXME: This get on parameters is PARALLEL_CRITICAL, which kills the parallel loop.
      boost::shared_ptr<Mantid::Geometry::Parameter> maskedParam = m_workspace->instrumentParameters().get(det.get(),"masked");

      bool masked = bool(maskedParam);

      // FIXME: The following does NOT work because isDetectorMasked() does not look for the parametrized version of the instrument. I think
//      const std::set<detid_t>& dets = m_workspace->getSpectrum(wi)->getDetectorIDs();
//      bool masked = false;
//      if (dets.size() > 0)
//      {
//        detid_t id = *dets.begin();
//        masked = inst->isDetectorMasked(id);
//      }

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

void InstrumentActor::update()
{
  setIntegrationRange(m_BinMinValue,m_BinMaxValue);
  resetColors();
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

void InstrumentActor::loadColorMap(const QString& fname,bool reset_colors)
{
  m_colorMap.loadMap(fname);
  m_currentColorMap = fname;
  if (reset_colors)
  {
    resetColors();
  }
}

size_t InstrumentActor::push_back_detid(Mantid::detid_t id)const
{
  m_detIDs.push_back(id);
  return m_detIDs.size() - 1;
}

void InstrumentActor::changeScaleType(int type)
{
  m_colorMap.changeScaleType(static_cast<GraphOptions::ScaleType>(type));
  resetColors();
}

void InstrumentActor::loadSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  int scaleType = settings.value("ScaleType", 0 ).toInt();
  //Load Colormap. If the file is invalid the default stored colour map is used
  m_currentColorMap = settings.value("ColormapFile", "").toString();
  // Set values from settings
  if (!m_currentColorMap.isEmpty())
  {
    loadColorMap(m_currentColorMap,false);
  }
  
  m_colorMap.changeScaleType(static_cast<GraphOptions::ScaleType>(scaleType));
  settings.endGroup();
}

void InstrumentActor::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  settings.setValue("ColormapFile", m_currentColorMap);
  settings.setValue("ScaleType", (int)m_colorMap.getScaleType() );
  settings.endGroup();
}

void InstrumentActor::setMinValue(double vmin)
{
  if (m_autoscaling) return;
  if (vmin < m_DataMinValue)
  {
    vmin = m_DataMinValue;
  }
  if (vmin > m_DataMaxValue) return;
  m_DataMinScaleValue = vmin;
  resetColors();
}

void InstrumentActor::setMaxValue(double vmax)
{
  if (m_autoscaling) return;
  if (vmax < m_DataMinValue) return;
  if (vmax > m_DataMaxValue)
  {
    vmax = m_DataMaxValue;
  }
  m_DataMaxScaleValue = vmax;
  resetColors();
}

void InstrumentActor::setMinMaxRange(double vmin, double vmax)
{
  if (m_autoscaling) return;
  if (vmin < m_DataMinValue)
  {
    vmin = m_DataMinValue;
  }
  if (vmax > m_DataMaxValue)
  {
    vmax = m_DataMaxValue;
  }
  if (vmin >= vmax) return;
  m_DataMinScaleValue = vmin;
  m_DataMaxScaleValue = vmax;
  resetColors();
}

bool InstrumentActor::wholeRange()const
{
  return m_BinMinValue == m_WkspBinMin && m_BinMaxValue == m_WkspBinMax;
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

bool SetVisibleComponentVisitor::visit(GLActor* actor)const
{
  ComponentActor* comp = dynamic_cast<ComponentActor*>(actor);
  if (comp)
  {
    bool on = comp->getComponent()->getComponentID() == m_id;
    actor->setVisibility(on);
    return on;
  }
  return false;
}

bool FindComponentVisitor::visit(GLActor* actor)const
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
