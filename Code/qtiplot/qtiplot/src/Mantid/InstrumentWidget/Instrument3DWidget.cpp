#include "Instrument3DWidget.h"
#include "InstrumentActor.h"
#include "MantidObject.h"

#include "MantidAPI/IInstrument.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraDetectorMap.h"

#include "MantidKernel/Exception.h"

#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

#include "boost/shared_ptr.hpp"

#include <QTimer>
#include <QMessageBox>
#include <QString>

#include <map>
#include <cmath>
#include <cfloat>
#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

static const QRgb BLACK = qRgb(0,0,0);

Instrument3DWidget::Instrument3DWidget(QWidget* parent):
  GL3DWidget(parent),mFastRendering(true), iTimeBin(0), mDataMapping(INTEGRAL),
  mColorMap(), mInstrumentActor(NULL), mAxisDirection(Mantid::Geometry::V3D(0.0,0.0,1.0)),
  mAxisUpVector(Mantid::Geometry::V3D(0.0,1.0,0.0)), mDataMinValue(DBL_MAX), mDataMaxValue(-DBL_MAX),
  mBinMinValue(DBL_MAX), mBinMaxValue(-DBL_MAX), mDataMinEdited(false), mDataMaxEdited(false),
  mWkspDataMin(DBL_MAX), mWkspDataMax(-DBL_MAX), mWkspBinMin(DBL_MAX), mWkspBinMax(-DBL_MAX), 
  mWorkspaceName(""), mWorkspace(), mScaledValues(0)
{
  connect(this, SIGNAL(actorsPicked(const std::set<QRgb>&)), this, SLOT(fireDetectorsPicked(const std::set<QRgb>&)));
  connect(this, SIGNAL(actorHighlighted(QRgb)),this,SLOT(fireDetectorHighligted(QRgb)));
}

Instrument3DWidget::~Instrument3DWidget()
{
	makeCurrent();
}

/**
 * Set the default Axis direction of the model
 */
void Instrument3DWidget::setAxis(const Mantid::Geometry::V3D& direction)
{
	mAxisDirection = direction;
}


//------------------------------------------------------------------------------------------------
/**
 * This method is the slot when the detectors are picked using mouse. This method emits
 * signals the ids of the detector and the spectra index(not spectra number).
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorsPicked(const std::set<QRgb>& pickedColors)
{
  std::vector<int> detectorIds;
  detectorIds.reserve(pickedColors.size());
  for(std::set<QRgb>::const_iterator it = pickedColors.begin(); it!= pickedColors.end(); it++)
  {
    int iDecId = mInstrumentActor->getDetectorIDFromColor(qRed((*it))*65536+qGreen((*it))*256+qBlue((*it)));
    if(iDecId != -1)
    {
      detectorIds.push_back(iDecId);
    }
  }
  if( detectorIds.empty() ) return;
  createWorkspaceIndexList(detectorIds);
  emit detectorsSelected();

//   if( detectorIds.size() == 1)
//   {
//     //emit the detector id
//     emit actionDetectorSelected(detectorIds.front());
//     //emit the spectra id
//     emit actionSpectraSelected(spectraIndices.front());
//   }
//   else // If more than one detector selected
//   {
//     std::set<int> spectralist(spectraIndices.begin(), spectraIndices.end());
//     //emit the detector ids
//     emit actionDetectorSelectedList(detectorIds);
//     //emit the spectra ids
//     emit actionSpectraSelectedList(spectralist);
//   }

}


//------------------------------------------------------------------------------------------------
/**
 * This method is the slot when the detector is highlighted using mouse move. This method emits
 * signals the id of the detector and the spectra index(not spectra number).
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorHighligted(QRgb pickedColor)
{
  if(pickedColor == BLACK)
  {
    emit actionDetectorHighlighted(-1,-1,-1);
    return;
  }
  int iDecId = mInstrumentActor->getDetectorIDFromColor(qRed(pickedColor)*65536 + qGreen(pickedColor)*256 + qBlue(pickedColor));
  if(iDecId != -1)
  {
    //convert detector id to spectra index id
    std::vector<int> idDecVec(1, iDecId);
    createWorkspaceIndexList(idDecVec);
    int spectrumNumber(1);
    int index = m_workspace_indices.front();
    try
    {
      spectrumNumber = mWorkspace->getAxis(1)->spectraNo(index);
    }
    catch(Mantid::Kernel::Exception::IndexError&)
    {
      //Not a Workspace2D
    }

    double sum = integrateSingleSpectra(mWorkspace, index);
    //emit the detector id, spectrum number and count to display in the window
    emit actionDetectorHighlighted(iDecId, spectrumNumber, std::floor(sum));
  }

}


//------------------------------------------------------------------------------------------------
/**
 * This method sets the workspace name input to the widget.
 * @param wsName input workspace name
 */
void Instrument3DWidget::setWorkspace(const QString& wsName)
{
  MatrixWorkspace_sptr output = 
    boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  if( !output )
  {
    QMessageBox::warning(this, "MantidPlot", QString("Error retrieving workspace \"") + 
			 wsName + QString("\". Cannot render instrument"));
    return;
  }
  // Save the workspace name
  mWorkspaceName = wsName;
  mWorkspace = output;
  // Read the instrument geometry
  boost::shared_ptr<Mantid::API::IInstrument> ins = output->getInstrument();
  this->ParseInstrumentGeometry(ins);
  boost::shared_ptr<Mantid::Geometry::IObjComponent> sample = ins->getSample();
  if( sample.get() )
  {
    _trackball->setModelCenter(sample->getPos());
  }
  else
  {
    _trackball->setModelCenter(Mantid::Geometry::V3D(0.0,0.0,0.0));
  }
  defaultProjection(); // Calculate and set projection

  // Calculate bin values, data ranges and integrate data
  calculateBinRange(output);
  calculateColorCounts(output);

}


//------------------------------------------------------------------------------------------------
/**
 * This method parses the instrument information and creates the actors relating to the detectors.
 */
void Instrument3DWidget::ParseInstrumentGeometry(boost::shared_ptr<Mantid::API::IInstrument> ins)
{
  makeCurrent();
  boost::shared_ptr<GLActorCollection> scene = boost::shared_ptr<GLActorCollection>(new GLActorCollection);
  mInstrumentActor = new InstrumentActor(ins, mFastRendering);
  scene->addActor(mInstrumentActor);
  this->setActorCollection(scene);
}

//------------------------------------------------------------------------------------------------
/**
 * Calculate the minimum and maximum values of the bins for the set workspace
 */
void Instrument3DWidget::calculateBinRange(Mantid::API::MatrixWorkspace_sptr workspace)
{
  const int nHist = workspace->getNumberHistograms();
  mWkspBinMin = DBL_MAX;
  mWkspBinMax = -DBL_MAX;
  for (int i = 0; i < nHist; ++i)
  {
    const Mantid::MantidVec & values = workspace->readX(i);
    double xtest = values.front();
    if( xtest != std::numeric_limits<double>::infinity() )
    {
    
      if( xtest < mWkspBinMin )
      {
        mWkspBinMin = xtest;
      }
      else if( xtest > mWkspBinMax )
      {
        mWkspBinMax = xtest;
      }
      else {}
    }

    xtest = values.back();
    if( xtest != std::numeric_limits<double>::infinity() )
    {
      if( xtest < mWkspBinMin )
      {
        mWkspBinMin = xtest;
      }
      else if( xtest > mWkspBinMax )
      {
        mWkspBinMax = xtest;
      }
      else {}
    }
  }

  // Value has not been preset
  if( std::fabs(mBinMinValue - DBL_MAX)/DBL_MAX < 1e-08 )
  {
    mBinMinValue = mWkspBinMin;
  }

  // Value has not been preset
  if( (mBinMaxValue + DBL_MAX)/DBL_MAX < 1e-08 )
  {
    mBinMaxValue = mWkspBinMax;
  }

  // Check validity
  if( mBinMinValue < mWkspBinMin || mBinMinValue > mWkspBinMax )
  {
      mBinMinValue = mWkspBinMin;
  }

  if( mBinMaxValue > mWkspBinMax || mBinMaxValue < mWkspBinMin )
  {
    mBinMaxValue = mWkspBinMax;
  }


}

//------------------------------------------------------------------------------------------------
/**
 * Integrate the workspace. This calculates the total counts
 * in all spectra and makes the color list for each pixel, using
 * the current color map.
 */
void Instrument3DWidget::calculateColorCounts(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace)
{
  if( !workspace ) return;

  // This looks like a strange way of doing this but the CompAssemblyActor needs the colours in the same
  // order as it fills its detector lists!
  if (detector_list.size() == 0)
  {
    //Only load the detector ID list once per instance
    mInstrumentActor->getDetectorIDList(detector_list);
  }

  if( detector_list.empty() ) return;
  createWorkspaceIndexList(detector_list);

  const int n_spec = m_workspace_indices.size();
  std::vector<double> integrated_values(n_spec, 0.0);
  mWkspDataMin = DBL_MAX;
  mWkspDataMax = -DBL_MAX;
  for( int i = 0; i < n_spec; ++i )
  {
    int widx = m_workspace_indices[i];
    if( widx != -1 )
    {
      double sum = integrateSingleSpectra(workspace, widx);
      integrated_values[i] = sum;
      if( sum < mWkspDataMin )
      {
        mWkspDataMin = sum;
      }
      else if( sum > mWkspDataMax )
      {
        mWkspDataMax = sum;
      }
      else continue;

    }
    else
    {
      integrated_values[i] = -1.0;
    }
  }
  //No need to store these now
  m_workspace_indices.clear();
  m_detector_ids.clear();

  // No preset value
  if( mDataMinEdited == false )
  {
    mDataMinValue = mWkspDataMin;
  }

  if( mDataMaxEdited == false )
  {
    mDataMaxValue = mWkspDataMax;
  }

  const short max_ncols = mColorMap.getLargestAllowedCIndex() + 1;
  mScaledValues = std::vector<unsigned char>(n_spec, 0);
  std::vector<boost::shared_ptr<GLColor> > colorlist(n_spec);
  QwtDoubleInterval wksp_interval(mWkspDataMin, mWkspDataMax);
  QwtDoubleInterval user_interval(mDataMinValue, mDataMaxValue);

  std::vector<double>::const_iterator val_end = integrated_values.end();
  int idx(0);
  for( std::vector<double>::const_iterator val_itr = integrated_values.begin(); val_itr != val_end;
      ++val_itr, ++idx )
  {
    unsigned char c_index(mColorMap.getTopCIndex());
    if( (*val_itr) < 0.0 )
    {
      mScaledValues[idx] = mColorMap.getLargestAllowedCIndex();
    }
    else
    {
      // Index to store
      short index = std::floor( mColorMap.normalize(user_interval, *val_itr)*max_ncols );
      if( index >= max_ncols )
      {
        index = max_ncols;
      }
      else if( index < 0 )
      {
        index = 0;
      }
      else {}
      mScaledValues[idx] = static_cast<unsigned char>(index);
      c_index = mColorMap.colorIndex(user_interval, *val_itr);

    }
    colorlist[idx] = mColorMap.getColor(c_index);
  }
  mInstrumentActor->setDetectorColors(colorlist);
}


//------------------------------------------------------------------------------------------------
/** Returns the sum of all bins in a given spectrum
 *
 * @param workspace workspace to use.
 * @param wksp_index index into the workspace to sum up.
 */
double Instrument3DWidget::integrateSingleSpectra(Mantid::API::MatrixWorkspace_sptr workspace, const int wksp_index)
{
	// If the index is not valid for this workspace
	if (wksp_index < 0 || wksp_index > workspace->getNumberHistograms())
		return 0.0;

	// Get Handle to data
	const Mantid::MantidVec& x=workspace->readX(wksp_index);
	const Mantid::MantidVec& y=workspace->readY(wksp_index);
	// If it is a 1D workspace, no need to integrate
	if (x.size()==2)
		return y[0];
	// Iterators for limits
	Mantid::MantidVec::const_iterator lowit=x.begin(),highit=x.end()-1;
	// If the first element is lower that the xmin then search for new lowit
	if ((*lowit) < mBinMinValue)
		lowit = std::lower_bound(x.begin(),x.end(),mBinMinValue);
	// If the last element is higher that the xmax then search for new lowit
	if ((*highit) > mBinMaxValue)
		highit = std::upper_bound(lowit,x.end(),mBinMaxValue);
	// Get the range for the y vector
	Mantid::MantidVec::difference_type distmin = std::distance(x.begin(), lowit);
	Mantid::MantidVec::difference_type distmax = std::distance(x.begin(), highit);
	double sum(0.0);
	if( distmin <= distmax )
	{
	  // Integrate
	  sum = std::accumulate(y.begin() + distmin,y.begin() + distmax,0.0);
	}
	return sum;
}


//------------------------------------------------------------------------------------------------
/**
 * Run a recount for the current workspace
 */
void Instrument3DWidget::recount()
{
  calculateColorCounts(mWorkspace);
  mInstrumentActor->refresh();
  update();
}



//------------------------------------------------------------------------------------------------
/**
 * For a change in the colour map, just update the color indices.
 */
void Instrument3DWidget::updateColorsForNewMap()
{

  const short max_ncols = mColorMap.getLargestAllowedCIndex() + 1;
  const short ncols = mColorMap.getTopCIndex() + 1;

  //Create a list of GLColor objects for every spectrum in the workspace
  std::vector<boost::shared_ptr<GLColor> > colorlist(mScaledValues.size());
  if( max_ncols == ncols )
  {
    std::vector<unsigned char>::const_iterator val_end = mScaledValues.end();
    int idx(0);
    for( std::vector<unsigned char>::const_iterator val_itr = mScaledValues.begin();
        val_itr != val_end; ++val_itr, ++idx )
    {
      colorlist[idx] = mColorMap.getColor(*val_itr);
    }
  }
  else
  {
    std::vector<unsigned char>::const_iterator val_end = mScaledValues.end();
    int idx(0);
    const double ratio = (double)ncols / max_ncols;
    for( std::vector<unsigned char>::const_iterator val_itr = mScaledValues.begin();
        val_itr != val_end; ++val_itr, ++idx )
    {
      short cache_value = static_cast<short>(*val_itr);
      short c_index = std::ceil((cache_value + 1)*ratio);
      if( c_index <= 0 ) c_index = 1;
      else if( c_index > ncols) c_index = ncols;
      else {}
      colorlist[idx] = mColorMap.getColor(static_cast<unsigned char>(c_index - 1));
    }
  }

  mInstrumentActor->setDetectorColors(colorlist);
  mInstrumentActor->refresh();
  update();
}


//------------------------------------------------------------------------------------------------
/**
 * Update the colors based on a change in the maximum data value
 */
void Instrument3DWidget::setMaxData(const double new_max)
{
  mDataMaxValue = new_max;
  setDataMaxEdited(true);
}


//------------------------------------------------------------------------------------------------
/**
 * Update the colors based on a change in the minimum data value
 */
void Instrument3DWidget::setMinData(const double new_min)
{
  mDataMinValue = new_min;
  setDataMinEdited(true);
}


//------------------------------------------------------------------------------------------------
/**
 * Mark the min data as bein user edited
 * @param If true the data min value has been set by the user
 */
void Instrument3DWidget::setDataMinEdited(bool state)
{
  mDataMinEdited = state;
}

//------------------------------------------------------------------------------------------------
/**
 * Mark the min data as bein user edited
 * @param If true the data min value has been set by the user
 */
void Instrument3DWidget::setDataMaxEdited(bool state)
{
  mDataMaxEdited = state;
}

//------------------------------------------------------------------------------------------------
/**
 * This method returns the Spectra Index list for the input dectector id list.
 * @param idDecVec is list of detector id's
 */
void Instrument3DWidget::createWorkspaceIndexList(const std::vector<int> & det_ids)
{
  if( det_ids.empty() ) return;
  m_workspace_indices.clear();
  m_detector_ids = det_ids;

  // There is no direct way of getting histogram index from the spectra id,
  // get the spectra axis and convert from index to spectra number and create
  // a map.
  const std::vector<int> spectraList = mWorkspace->spectraMap().getSpectra(m_detector_ids);
  Axis* spectraAxis = mWorkspace->getAxis(1);
  std::map<int,int> index_map;
  int n_hist = mWorkspace->getNumberHistograms();
  for (int i = 0; i < n_hist; ++i)
  {
    int current_spectrum = spectraAxis->spectraNo(i);
    index_map[current_spectrum] = i;
  }

  std::vector<int>::const_iterator spec_end = spectraList.end();
  std::vector<int>::const_iterator d_itr = m_detector_ids.begin();
  for( std::vector<int>::const_iterator spec_itr = spectraList.begin(); spec_itr != spec_end;
       ++spec_itr, ++d_itr )
  {
    if( (*d_itr) != -1 )
    {
      m_workspace_indices.push_back(index_map[*spec_itr]);
    }
    else
    {
      m_workspace_indices.push_back(-1);
    }
  }

}

//------------------------------------------------------------------------------------------------
/**
 * This method sets the Time bin values. the value has to be greater than zero
 * @param value input is the time bin value
 */
void Instrument3DWidget::setTimeBin(int value)
{
  if(value>0)
  {
    this->iTimeBin=value;
  }
}

//------------------------------------------------------------------------------------------------
/**
 * Returns workspace name
 */
QString Instrument3DWidget::getWorkspaceName() const
{
  return mWorkspaceName;
}

//------------------------------------------------------------------------------------------------
/**
 * Returns a reference to the constant colormap
 */
const MantidColorMap & Instrument3DWidget::getColorMap() const
{
  return mColorMap;
}

//------------------------------------------------------------------------------------------------
/**
 * Returns a reference to the colormap
 */
MantidColorMap & Instrument3DWidget::mutableColorMap()
{
  return mColorMap;
}


/**
 * This method takes the input name as the min value Colormap scale.
 */
void Instrument3DWidget::setColorMapMinValue(double minValue)
{
	this->mDataMinValue=minValue;
}

/**
 * This method takes the input name as the min value Colormap scale.
 */
void Instrument3DWidget::setColorMapMaxValue(double maxValue)
{
	this->mDataMaxValue = maxValue;
}

/**
 * This method returns min value. by default will be min value in the current timebin
 */
double Instrument3DWidget::getDataMinValue() const
{
	return this->mDataMinValue;
}

/**
 * This method returns the max value. by default will be max value in the current timebin
 */
double Instrument3DWidget::getDataMaxValue() const
{
	return this->mDataMaxValue;
}

/**
 * Returns the current minimum bin value
 */
double Instrument3DWidget::getBinMinValue() const
{
  return this->mBinMinValue;
}

double Instrument3DWidget::getBinMaxValue() const
{
    return this->mBinMaxValue;
}

/**
 * This method sets the Data mapping type for the color mapping.
 */
void Instrument3DWidget::setDataMappingType(DataMappingType dmType)
{
	mDataMapping=dmType;
}

void Instrument3DWidget::setDataMappingIntegral(double minValue,double maxValue)
{
  this->mBinMinValue = minValue;
  this->mBinMaxValue = maxValue;
  setDataMappingType(INTEGRAL);
  if( this->isVisible() )
  {
    calculateColorCounts(mWorkspace);
    mInstrumentActor->refresh();
    update();
  }
}

void Instrument3DWidget::setDataMappingSingleBin(int binNumber)
{
  this->iTimeBin=binNumber;
  setDataMappingType(SINGLE_BIN);
}

/**
 * Sets the default view to X direction positive
 */
void Instrument3DWidget::setViewDirectionXPositive()
{
	setViewDirection(XPOSITIVE);
}

/**
 * Sets the default view to Y direction positive
 */
void Instrument3DWidget::setViewDirectionYPositive()
{
	setViewDirection(YPOSITIVE);
}

/**
 * Sets the default view to Z direction positive
 */
void Instrument3DWidget::setViewDirectionZPositive()
{
	setViewDirection(ZPOSITIVE);
}

/**
 * Sets the default view to X direction negative
 */
void Instrument3DWidget::setViewDirectionXNegative()
{
	setViewDirection(XNEGATIVE);
}

/**
 * Sets the default view to Y direction negative
 */
void Instrument3DWidget::setViewDirectionYNegative()
{
	setViewDirection(YNEGATIVE);
}

/**
 * Sets the default view to Z direction negative
 */
void Instrument3DWidget::setViewDirectionZNegative()
{
	setViewDirection(ZNEGATIVE);
}

/**
 * Sets the slow rendering not using display list
 * NOTE: This method will ***NOT*** have any effect after the workspace name is set.
 */
void Instrument3DWidget::setSlowRendering()
{
	mFastRendering=false;
}

/**
 * Sets the fast rendering using display list
 * NOTE: This method will ***NOT*** have any effect after the workspace name is set.
 */
void Instrument3DWidget::setFastRendering()
{
	mFastRendering=true;
}


/**
 * Completely resets the data in the instrument widget. ready for new workspace
 */
void Instrument3DWidget::resetWidget()
{
	iTimeBin = 0;
	mWorkspaceName = QString();
	mBinMinValue = DBL_MAX;
	mBinMaxValue = -DBL_MAX;
	mDataMinValue = DBL_MAX;
	mDataMaxValue = -DBL_MAX;
	mDataMinEdited = false;
	mDataMaxEdited = false;
	mDataMapping = INTEGRAL;
	mAxisDirection = Mantid::Geometry::V3D(0.0,0.0,1.0);
	mAxisUpVector = Mantid::Geometry::V3D(0.0,1.0,0.0);
	mScaledValues.clear();
	GL3DWidget::resetWidget();
}

void Instrument3DWidget::setView(const V3D& pos,double xmax,double ymax,double zmax,double xmin,double ymin,double zmin)
{
	////get the centre of the bounding box
	//V3D boundCentre;
	//double xhalf=(xmax-xmin)/2.0;
	//double yhalf=(ymax-ymin)/2.0;
	//double zhalf=(zmax-zmin)/2.0;
	//boundCentre[0]=-1*(xmin+xhalf);
	//boundCentre[1]=-1*(ymin+yhalf);
	//boundCentre[2]=-1*(zmin+zhalf);
	////vector from center to bounding box center
	//V3D vcb=pos-boundCentre;
	//vcb.normalize();
	//V3D zaxis(0,0,1);
	////get the rotation about zaxis
	//Quat rotation(zaxis,vcb);
	//rotation.inverse();
	//_trackball->reset();
	//_trackball->setModelCenter(pos);
	//if(rotation!=Quat(0,0,0,0))
	//	_trackball->setRotation(rotation);
	//_trackball->rotateBoundingBox(xmin,xmax,ymin,ymax,zmin,zmax);//rotate the bounding box
	//_viewport->setOrtho(xmin,xmax,ymin,ymax,-zmin,-zmax);
	//_viewport->issueGL();
	//update();

	//Change the View to the axis orientation
	V3D s=mAxisDirection.cross_prod(mAxisUpVector);
	V3D u=s.cross_prod(mAxisDirection);
	double Mat[16];
	Mat[0]=s[0];              Mat[4]=s[1];              Mat[8]=s[2];               Mat[12]=0;
	Mat[1]=u[0];              Mat[5]=u[1];              Mat[9]=u[2];               Mat[13]=0;
	Mat[2]=-mAxisDirection[0];Mat[6]=-mAxisDirection[1];Mat[10]=-mAxisDirection[2];Mat[14]=0;
	Mat[3]=0;                 Mat[7]=0;                 Mat[11]=0;                 Mat[15]=1;
	Quat defaultView;
	defaultView.setQuat(Mat);
	defaultView.normalize();
	//get the rotation to make the center of the bounding box to the view
	V3D boundCentre;
	boundCentre[0]=(xmax+xmin)/2.0;
	boundCentre[1]=(ymax+ymin)/2.0;
	boundCentre[2]=(zmax+zmin)/2.0;
	V3D vcb=boundCentre-pos;
	vcb.normalize();
	V3D zaxis(0,0,1);
	Quat rotation(zaxis,vcb);
	rotation.inverse();
	if(rotation!=Quat(0,0,0,0))
		defaultView=rotation*defaultView;
	_trackball->reset();
	_trackball->setModelCenter(pos);
	if(defaultView!=Quat(0,0,0,0))
		_trackball->setRotation(defaultView);
	_trackball->rotateBoundingBox(xmin,xmax,ymin,ymax,zmin,zmax);//rotate the bounding box
	_viewport->setOrtho(xmin,xmax,ymin,ymax,-zmax,-zmin);
	_viewport->issueGL();
	update();
}

//------------------------------------------------------------------------------------------------
/**
 * This method draws the scene using color id. this method is called in pick mode
 */
void Instrument3DWidget::drawSceneUsingColorID()
{
	mInstrumentActor->drawUsingColorID();
}

/**
 * Draws the scene in low resolution. this method is called in interactive mode for faster response
 */
void Instrument3DWidget::setSceneLowResolution()
{
	mInstrumentActor->setObjectResolutionToLow();
}

/**
 * Draws the scene in high resolution.
 */
void Instrument3DWidget::setSceneHighResolution()
{
	mInstrumentActor->setObjectResolutionToHigh();
}

//------------------------------------------------------------------------------------------------
/**
 * Returns the boundig box of the scene
 * @param minBound :: output min point of the bounding box of scene
 * @param maxBound :: output max point of the bounding box of scene
 */
void Instrument3DWidget::getBoundingBox(Mantid::Geometry::V3D& minBound, Mantid::Geometry::V3D& maxBound)
{
	mInstrumentActor->getBoundingBox(minBound,maxBound);
}
