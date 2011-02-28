#include "Instrument3DWidget.h"
#include "InstrumentActor.h"
#include "InstrumentWindow.h"
#include "MantidObject.h"
#include "OpenGLError.h"
#include "UnwrappedSurface.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Algorithm.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Timer.h"

#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

#include <QTimer>
#include <QMessageBox>
#include <QString>
#include <QMenu>
#include <QAction>
#include <QSet>
#include <QTemporaryFile>
#include <QDir>
#include <QApplication>
#include <QFileDialog>

#include <map>
#include <cmath>
#include <cfloat>
#include <numeric>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

static const bool SHOWTIMING = false;

/** A class for creating grouping xml files
  */
class DetXMLFile//: public DetectorCallback
{
public:
  enum Option {List,Sum};
  /// Create a grouping file to extract all detectors in detector_list excluding those in dets
  DetXMLFile(const std::vector<int> detector_list, const QSet<int>& dets, const QString& fname)
  {
    m_fileName = fname;
    m_delete = false;
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    std::vector<int>::const_iterator idet = detector_list.begin();
    for(; idet != detector_list.end(); ++idet)
    {
      if (!dets.contains(*idet))
      {
        out <<  *idet << ',';
      }
    }
    out << "\"/> </group> \n</detector-grouping>\n";
  }

  /// Create a grouping file to extract detectors in dets. Option List - one group - one detector,
  /// Option Sum - one group which is a sum of the detectors
  /// If fname is empty create a temporary file
  DetXMLFile(const QSet<int>& dets, Option opt = List, const QString& fname = "")
  {
    if (dets.empty())
    {
      m_fileName = "";
      return;
    }

    if (fname.isEmpty())
    {
      QTemporaryFile mapFile;
      mapFile.open();
      m_fileName = mapFile.fileName() + ".xml";
      mapFile.close();
      m_delete = true;
    }
    else
    {
      m_fileName = fname;
      m_delete = false;
    }

    switch(opt)
    {
    case Sum: makeSumFile(dets); break;
    case List: makeListFile(dets); break;
    }

  }

  /// Make grouping file where each detector is put into its own group
  void makeListFile(const QSet<int>& dets)
  {
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
    foreach(int det,dets)
    {
      out << "<group name=\"" << det << "\"> <detids val=\"" << det << "\"/> </group> \n";
    }
    out << "</detector-grouping>\n";
  }

  /// Make grouping file for putting the detectors into one group (summing the detectors)
  void makeSumFile(const QSet<int>& dets)
  {
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    foreach(int det,dets)
    {
      out << det << ',';
    }
    out << "\"/> </group> \n</detector-grouping>\n";
  }

  ~DetXMLFile()
  {
    if (m_delete)
    {
      QDir dir;
      dir.remove(m_fileName);
    }
  }

  /// Return the name of the created grouping file
  const std::string operator()()const{return m_fileName.toStdString();}

private:
  QString m_fileName; ///< holds the grouping file name
  bool m_delete;      ///< if true delete the file on destruction
};

Instrument3DWidget::Instrument3DWidget(InstrumentWindow* parent):
  GL3DWidget(parent),m_instrumentWindow(parent),mFastRendering(true), iTimeBin(0), mDataMapping(INTEGRAL),
  mColorMap(), mInstrumentActor(NULL), mAxisDirection(Mantid::Geometry::V3D(0.0,0.0,1.0)),
  mAxisUpVector(Mantid::Geometry::V3D(0.0,1.0,0.0)), mDataMinValue(DBL_MAX), mDataMaxValue(-DBL_MAX),
  mBinMinValue(DBL_MAX), mBinMaxValue(-DBL_MAX),
  mBinEntireRange(true),
  mDataMinEdited(false), mDataMaxEdited(false),
  mWkspDataMin(DBL_MAX), mWkspDataMax(-DBL_MAX), mWkspBinMin(DBL_MAX), mWkspBinMax(-DBL_MAX), 
  mWorkspaceName(""), mWorkspace(), mScaledValues(0)
{
  connect(this, SIGNAL(actorsPicked(const std::set<QRgb>&)), this, SLOT(fireDetectorsPicked(const std::set<QRgb>&)));
  connect(this, SIGNAL(actorHighlighted(QRgb)),this,SLOT(fireDetectorHighligted(QRgb)));
  connect(this, SIGNAL(actorHighlighted(int)),this,SLOT(fireDetectorHighligted(int)));
  connect(this, SIGNAL(increaseSelection(QRgb)),this,SLOT(detectorsHighligted(QRgb)));

  m_ExtractDetsToWorkspaceAction = new QAction("Extract to new workspace",this);
  connect(m_ExtractDetsToWorkspaceAction,SIGNAL(activated()),this,SLOT(extractDetsToWorkspace()));

  m_SumDetsToWorkspaceAction = new QAction("Sum to new workspace",this);
  connect(m_SumDetsToWorkspaceAction,SIGNAL(activated()),this,SLOT(sumDetsToWorkspace()));

  m_createIncludeGroupingFileAction = new QAction("Include",this);
  connect(m_createIncludeGroupingFileAction,SIGNAL(activated()),this,SLOT(createIncludeGroupingFile()));

  m_createExcludeGroupingFileAction = new QAction("Exclude",this);
  connect(m_createExcludeGroupingFileAction,SIGNAL(activated()),this,SLOT(createExcludeGroupingFile()));
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
 * @param pickedActor :: the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorsPicked(const std::set<QRgb>& pickedColors)
{
  if (m_instrumentWindow->blocked())
  {
    return;
  }
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

  // fill in m_detector_ids and m_workspace_indices with the selected detector ids
  createWorkspaceIndexList(detectorIds, true);

  emit detectorsSelected();
}
//------------------------------------------------------------------------------------------------
/**
 * This method is the slot when the detector is highlighted using mouse move. This method emits
 * signals the id of the detector and the spectra index(not spectra number).
 * @param pickedActor :: the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorHighligted(QRgb pickedColor)
{
  if (m_instrumentWindow->blocked())
  {
    return;
  }
  //get the data for the detector currently under the cursor
  const int iDetId = mInstrumentActor->getDetectorIDFromColor(qRed(pickedColor)*65536 + qGreen(pickedColor)*256 + qBlue(pickedColor));

  //retrieve information about the selected detector
  m_detInfo.setDet(iDetId);
  //send this detector information off
  emit actionDetectorHighlighted(m_detInfo);
}
/**
 * This method is the slot when the detector is highlighted using mouse move. This method emits
 * signals the id of the detector and the spectra index(not spectra number).
 * @param pickedActor :: the input passed by the the signal.
 */
void Instrument3DWidget::detectorsHighligted(QRgb pickedColor)
{
  if (m_instrumentWindow->blocked())
  {
    return;
  }
  //get the data for the detector currently under the cursor
  const int iDetId = mInstrumentActor->getDetectorIDFromColor(qRed(pickedColor)*65536 + qGreen(pickedColor)*256 + qBlue(pickedColor));

  //retrieve information about the selected detector
  m_detInfo.setEndRange(iDetId);
  //send this detector information off
  emit actionDetectorHighlighted(m_detInfo);
}

void Instrument3DWidget::fireDetectorHighligted(int detID)
{
  if (m_instrumentWindow->blocked())
  {
    return;
  }
  m_detInfo.setDet(detID);
  emit actionDetectorHighlighted(m_detInfo);
}
//------------------------------------------------------------------------------------------------
/**
 * This method sets the workspace name input to the widget.
 * @param wsName :: input workspace name
 */
void Instrument3DWidget::setWorkspace(const QString& wsName)
{
  Timer timer;
  makeCurrent();

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
  boost::shared_ptr<Mantid::Geometry::IInstrument> ins = output->getInstrument();
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
  calculateColorCounts(output, true);

  if (SHOWTIMING) std::cout << "Instrument3DWidget::setWorkspace() took " << timer.elapsed() << " seconds\n";
}


//------------------------------------------------------------------------------------------------
/**
 * This method parses the instrument information and creates the actors relating to the detectors.
 */
void Instrument3DWidget::ParseInstrumentGeometry(boost::shared_ptr<Mantid::Geometry::IInstrument> ins)
{
  Timer timer;
  makeCurrent();
  boost::shared_ptr<GLActorCollection> scene = boost::shared_ptr<GLActorCollection>(new GLActorCollection);
  mInstrumentActor = new InstrumentActor(ins, mFastRendering);
  scene->addActor(mInstrumentActor);
  this->setActorCollection(scene);
  if (SHOWTIMING) std::cout << "Instrument3DWidget::ParseInstrumentGeometry() took " << timer.elapsed() << " seconds\n";
}

//------------------------------------------------------------------------------------------------
/**
 * Calculate the minimum and maximum values of the bins for the set workspace
 */
void Instrument3DWidget::calculateBinRange()
{
  Mantid::API::MatrixWorkspace_sptr workspace = mWorkspace;
  Timer timer;

  // Value has not been preset?
  if (( std::fabs(mWkspBinMin - DBL_MAX)/DBL_MAX < 1e-08 ) && ( (mWkspBinMax + DBL_MAX)/DBL_MAX < 1e-08 ))
  {
    //Then we need to calculate
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
    if ( (std::fabs(mBinMinValue - DBL_MAX)/DBL_MAX < 1e-08) && 
	 ( (mBinMaxValue + DBL_MAX)/DBL_MAX < 1e-08) )
    {
      mBinMinValue = mWkspBinMin;
      mBinMaxValue = mWkspBinMax;
    }
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


  if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateBinRange() took " << timer.elapsed() << " seconds\n";

}

//------------------------------------------------------------------------------------------------
/**
 * Integrate the workspace. This calculates the total counts
 * in all spectra and makes the color list for each pixel, using
 * the current color map.
 * @param workspace :: new workspace being set.
 * @param firstCalculation :: set to true when changing the workspace; false is simply changing the color scale
 *
 */
void Instrument3DWidget::calculateColorCounts(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, bool firstCalculation)
{
  Timer timer;
  if( !workspace ) return;

  // This looks like a strange way of doing this but the CompAssemblyActor needs the colours in the same
  // order as it fills its detector lists!
  if (detector_list.size() == 0)
  {
    Timer timerID;
    //Only load the detector ID list once per instance
    mInstrumentActor->getDetectorIDList(detector_list);
    if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateColorCounts(): mInstrumentActor->getDetectorIDList() took " << timerID.elapsed() << " seconds\n";
  }

  if( detector_list.empty() ) return;

  //Make the workspace index list, and force a new one if needed.
  createWorkspaceIndexList(detector_list, firstCalculation);


  //TODO: Make this part in parallel if possible!

  Timer timer2;

  const int n_spec = m_workspace_indices.size();
  std::vector<double> integrated_values( n_spec, -1.0 );

  //Use the workspace function to get the integrated spectra
  mWorkspace->getIntegratedSpectra(m_specIntegrs, (this->mBinMinValue), (this->mBinMaxValue), (this->mBinEntireRange));

  mWkspDataMin = DBL_MAX;
  mWkspDataMax = -DBL_MAX;

  //Now we need to convert to a vector where each entry is the sum for the detector ID at that spot (in integrated_values).
  for (int i=0; i < n_spec; i++)
  {
    int widx = m_workspace_indices[i];
    if( widx != -1 )
    {
      double sum = m_specIntegrs[widx];
      integrated_values[i] = sum;
        if( sum < mWkspDataMin )
          mWkspDataMin = sum;
        else if( sum > mWkspDataMax )
          mWkspDataMax = sum;
    }
  }


  if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateColorCounts():Integrating workspace took " << timer2.elapsed() << " seconds\n";

  // No preset value
  if( mDataMinEdited == false )
  {
    mDataMinValue = mWkspDataMin;
  }

  if( mDataMaxEdited == false )
  {
    mDataMaxValue = mWkspDataMax;
  }

  Timer timerColLists;

  const short max_ncols = mColorMap.getLargestAllowedCIndex() + 1;
  mScaledValues = std::vector<unsigned char>(n_spec, 0);
  std::vector<boost::shared_ptr<GLColor> > colorlist(n_spec);
  QwtDoubleInterval wksp_interval(mWkspDataMin, mWkspDataMax);
  QwtDoubleInterval user_interval(mDataMinValue, mDataMaxValue);

  std::vector<double>::iterator val_end = integrated_values.end();
  int idx(0);
  for( std::vector<double>::iterator val_itr = integrated_values.begin(); val_itr != val_end;
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
  if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateColorCounts(): making the colorlist took " << timerColLists.elapsed() << " seconds\n";

  Timer timerCols;
  mInstrumentActor->setDetectorColors(colorlist);
  if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateColorCounts(): mInstrumentActor->setDetectorColors() took " << timerCols.elapsed() << " seconds\n";

  if (SHOWTIMING) std::cout << "Instrument3DWidget::calculateColorCounts() took " << timer.elapsed() << " seconds\n";
}
//------------------------------------------------------------------------------------------------
/**
 * Run a recount for the current workspace
 */
void Instrument3DWidget::recount()
{
  calculateColorCounts(mWorkspace, false);
  mInstrumentActor->refresh();
  redrawUnwrapped();
  update();
}



//------------------------------------------------------------------------------------------------
/**
 * For a change in the colour map, just update the color indices.
 */
void Instrument3DWidget::updateColorsForNewMap()
{
  Timer timer;

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

  if (SHOWTIMING) std::cout << "Instrument3DWidget::updateColorsForNewMap() took " << timer.elapsed() << " seconds\n";
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
 * @param If :: true the data min value has been set by the user
 */
void Instrument3DWidget::setDataMinEdited(bool state)
{
  mDataMinEdited = state;
}

//------------------------------------------------------------------------------------------------
/**
 * Mark the min data as bein user edited
 * @param If :: true the data max value has been set by the user
 */
void Instrument3DWidget::setDataMaxEdited(bool state)
{
  mDataMaxEdited = state;
}

//------------------------------------------------------------------------------------------------
/**
 * This method returns the workspace index list for the input dectector id list:
 *  i.e. the detector at index i of det_ids has a spectrum at workspace index given in
 *      m_workspace_indices[i].
 *
 * @param det_ids :: is list of detector id's
 * @param forceNew :: set to true to force the creation of a new list; otherwise, the old one will be reused
 * if possible.
 */
void Instrument3DWidget::createWorkspaceIndexList(const std::vector<int> & det_ids, bool forceNew)
{
  Timer timer;

  if( det_ids.empty() ) return;
  if (!forceNew)
  {
    //Don't force a new one, and the sizes match. We assume it is good.
    if (m_workspace_indices.size() == det_ids.size())
      return;
  }
  m_workspace_indices.resize(det_ids.size());
  m_detector_ids = det_ids;

  //the DetInfo object will collect information about selected detectors from the pointers and references passed
  m_detInfo = DetInfo(mWorkspace, &m_specIntegrs);

  //the DetInfo object can convert from detector IDs to spectra indices and so this creates a vector of spectra indices
  std::transform(m_detector_ids.begin(), m_detector_ids.end(),
                 m_workspace_indices.begin(), m_detInfo);
  /*
  std::vector<int>::const_iterator detIDsIn = m_detector_ids.begin();
  std::vector<int>::iterator specIndsOut = m_workspace_indices.begin();
  std::vector<int>::const_iterator end = m_detector_ids.end();
  for( ; detIDsIn != end; ++detIDsIn, ++specIndsOut)
  {
    *specIndsOut = m_detInfo.getIndexOf(*detIDsIn);
  }*/

  if (SHOWTIMING) std::cout << "Instrument3DWidget::createWorkspaceIndexList() took " << timer.elapsed() << " seconds\n";

}

//------------------------------------------------------------------------------------------------
/**
 * This method sets the Time bin values. the value has to be greater than zero
 * @param value :: input is the time bin value
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

/** Returns the current minimum bin value
 */
double Instrument3DWidget::getBinMinValue() const
{
  return this->mBinMinValue;
}

/** Returns the current maximum bin value
 */
double Instrument3DWidget::getBinMaxValue() const
{
    return this->mBinMaxValue;
}

/** Returns the current value for integrating all the bins (entire range)
 */
bool Instrument3DWidget::getBinEntireRange() const
{
    return this->mBinEntireRange;
}

/**
 * This method sets the Data mapping type for the color mapping.
 */
void Instrument3DWidget::setDataMappingType(DataMappingType dmType)
{
	mDataMapping=dmType;
}

void Instrument3DWidget::setDataMappingIntegral(double minValue,double maxValue, bool entireRange)
{
  this->mBinMinValue = minValue;
  this->mBinMaxValue = maxValue;
  this->mBinEntireRange = entireRange;
  setDataMappingType(INTEGRAL);
  if( this->isVisible() )
  {
    calculateColorCounts(mWorkspace, false);
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
        _viewport->setOrtho(xmin,xmax,ymin,ymax,-zmax,-zmin,false);
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

/** Set pointers to the workspace data that is needed to obtain information about
*  detectors
*  @param DetID :: id number of the detector to retrieve information for
*  @param workspace :: the workspace with counts data for the detector
*  @param counts :: integral of the number of counts in each spectrum
*  @throw runtime_error if there was an error creating the spectra index to detector index map
*/
Instrument3DWidget::DetInfo::DetInfo(Mantid::API::MatrixWorkspace_const_sptr workspace, const std::vector<double> * const counts) :
    m_workspace(workspace),
    m_integrals(counts),
    m_firstDet(ERROR_FLAG), m_lastDet(ERROR_FLAG)
{
  if (m_workspace)
  {
    m_detID_to_wi_map = boost::shared_ptr<const IndexToIndexMap>(
        m_workspace->getDetectorIDToWorkspaceIndexMap(false));
  }
}
/// set the object to contain data for only one detector
void Instrument3DWidget::DetInfo::setDet(const int detID)
{
  m_firstDet = detID;
  m_lastDet = NO_INDEX;
}
/** specify a range of detectors by giving the id of a detector at the end of the range
*  @param detID :: id number of detector to add
*/
void Instrument3DWidget::DetInfo::setEndRange(const int detID)
{
  if (is_good(detID))
  {
    m_lastDet = detID;
  }
  else
  {
    m_lastDet = m_firstDet = ERROR_FLAG;
  }
}
/** Returns a string containing all this object's data in a human readable
* form
* @return all this objects data labeled and formated for user display
*/
QString Instrument3DWidget::DetInfo::display() const
{
  std::ostringstream out;
  out << "Detector ID:  " << m_firstDet;

  if (is_good(m_firstDet))
  {
    printSpectrum(getIndexOf(m_firstDet), out);
    out << std::endl;
    printLocation(out);
  }
  else
  {// reserve a fixed width for displaying any data
    out << std::endl;
  }

  return QString::fromStdString(out.str());
}

int Instrument3DWidget::DetInfo::getWorkspaceIndex()const
{
  return is_good(m_firstDet) ? getIndexOf(m_firstDet) : -1;
}

/** Writes the information about the spectrum whose index is passed to it
*  in a human readble form to the stream provide, or nothing on error
*  @param[in] index the index number of the spectrum to display
*  @param[out] output information will be writen to this stream
*/
void Instrument3DWidget::DetInfo::printSpectrum(const int index, std::ostringstream & output) const
{
  if (is_good(index))
  {
    int spectrumNumber(NO_INDEX);
    try
    {
      spectrumNumber = m_workspace->getAxis(1)->spectraNo(index);
    }
    catch (...)
    {
      //if some information couldn't be retrieved, default values will be displayed
    }

    if (index != spectrumNumber && is_good(spectrumNumber) )
      output << "    Spectrum number: " << spectrumNumber << "  workspace index: " << index;
    else
      output << "    Spectrum number: " << spectrumNumber;
    output << "    Count:  ";

    if (m_integrals)
    {
      output << m_integrals->operator[](index);
    }
    else
    {
      output << "-";
    }
  }
}
/** Writes the location of any detectors whose information is retrievable
*  in a human readable form
*  @param[out] output information will be writen to this stream
*/
void Instrument3DWidget::DetInfo::printLocation(std::ostringstream & output) const
{
  // display location information for the detector, ignoring missing detectors
  std::string units("");
  try
  {
    IDetector_const_sptr locInfo =
      m_workspace->getInstrument()->getDetector(m_firstDet);
    V3D pos = locInfo->getPos();
    output << "position:  ";
    printV(pos, output);
    units = " m";

    locInfo = m_workspace->getInstrument()->getDetector(m_lastDet);
    V3D endPos = locInfo->getPos();
    output << " -> ";
    printV(endPos, output);
    output << " = ";
    printV(pos-endPos, output);
  }
  catch(Exception::NotFoundError &)
  {
    //don't display data when there is an error retreiving it
  }
  output << units;
}
/** Writes a position vector in a nice way
*  @param[in] pos coordinates to print
*  @param[out] output information will be writen to this stream
*/
void Instrument3DWidget::DetInfo::printV(Mantid::Geometry::V3D pos, std::ostringstream & out) const
{
  out << "(" << pos.X() << "," << pos.Y() << "," << pos.Z() << ")";
}
/** Returns the index number of the spectrum generated by the detector whose ID number
*  was passed or DetInfo::NO_INDEX on error
*  @param someDetID :: an ID of a detector that exists in the workspace
*  @return the index number of the spectrum associated with that detector or -1
*/
int Instrument3DWidget::DetInfo::getIndexOf(const int someDetID) const
{
  IndexToIndexMap::const_iterator it(m_detID_to_wi_map->find(someDetID));
  if ( it != m_detID_to_wi_map->end() )
  {
    return it->second;
  }
  else return NO_INDEX;
}

void Instrument3DWidget::showUnwrappedContextMenu()
{
  QMenu context(this);

  context.addAction(m_ExtractDetsToWorkspaceAction);
  context.addAction(m_SumDetsToWorkspaceAction);
  QMenu *gfileMenu = context.addMenu("Create grouping file");
  gfileMenu->addAction(m_createIncludeGroupingFileAction);
  gfileMenu->addAction(m_createExcludeGroupingFileAction);

  context.exec(QCursor::pos());
}

void Instrument3DWidget::showInfo()
{
  if (!m_unwrappedSurface) return;

  QSet<int> dets;
  m_unwrappedSurface->getPickedDetector(dets);

  QString msg;
  if (dets.size() == 0) return;
  else if (dets.size() == 1)
  {
    msg = "Detector ID " + QString::number(*dets.begin());
  }
  else
  {
    msg = "Selected " + QString::number(dets.size()) + " detectors";
  }
  QMessageBox::information(this,"MantidPlot",msg);
}

/**
  * Extract selected detectors to a new workspace
  */
void Instrument3DWidget::extractDetsToWorkspace()
{
  if (!m_unwrappedSurface) return;
  QSet<int> dets;
  m_unwrappedSurface->getPickedDetector(dets);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  DetXMLFile mapFile(dets);
  std::string fname = mapFile();

  if (!fname.empty())
  {
    Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
    alg->setPropertyValue("InputWorkspace",getWorkspaceName().toStdString());
    alg->setPropertyValue("MapFile",fname);
    alg->setPropertyValue("OutputWorkspace",getWorkspaceName().toStdString()+"_selection");
    alg->execute();
  }

  QApplication::restoreOverrideCursor();
}

/**
  * Sum selected detectors to a new workspace
  */
void Instrument3DWidget::sumDetsToWorkspace()
{
  if (!m_unwrappedSurface) return;
  QSet<int> dets;
  m_unwrappedSurface->getPickedDetector(dets);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  DetXMLFile mapFile(dets,DetXMLFile::Sum);
  std::string fname = mapFile();

  if (!fname.empty())
  {
    Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
    alg->setPropertyValue("InputWorkspace",getWorkspaceName().toStdString());
    alg->setPropertyValue("MapFile",fname);
    alg->setPropertyValue("OutputWorkspace",getWorkspaceName().toStdString()+"_sum");
    alg->execute();
  }

  QApplication::restoreOverrideCursor();
}

void Instrument3DWidget::createIncludeGroupingFile()
{
  if (!m_unwrappedSurface) return;
  QSet<int> dets;
  m_unwrappedSurface->getPickedDetector(dets);

  QString fname = QFileDialog::getSaveFileName(this,"Save grouping file");
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(dets,DetXMLFile::Sum,fname);
  }

}

void Instrument3DWidget::createExcludeGroupingFile()
{
  if (!m_unwrappedSurface) return;
  QSet<int> dets;
  m_unwrappedSurface->getPickedDetector(dets);

  QString fname = QFileDialog::getSaveFileName(this,"Save grouping file");
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(detector_list,dets,fname);
  }
}

