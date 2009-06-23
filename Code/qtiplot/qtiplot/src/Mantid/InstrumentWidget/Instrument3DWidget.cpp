#ifdef WIN32
#include <windows.h>
#endif
#include "GL3DWidget.h"
#include "MantidAPI/IInstrument.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "Instrument3DWidget.h"
#include "boost/shared_ptr.hpp"
#include "MantidObject.h"
#include "GLColorMap.h"
#include <QTimer>
#include <queue>
#include <map>
#include <math.h>
#include <float.h>
#include <QMessageBox>
#include <QString>
#include <iostream>
#include "MantidGeometry/Object.h"
#include "MantidGeometry/GeometryHandler.h"
#include "InstrumentActor.h"
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

static const QRgb BLACK=qRgb(0,0,0);
Instrument3DWidget::Instrument3DWidget(QWidget* parent):GL3DWidget(parent),mFastRendering(true),mInstrumentActor(0)
{
	iTimeBin=0;
	strWorkspaceName="";
	connect(this, SIGNAL(actorsPicked(const std::set<QRgb>&)), this, SLOT(fireDetectorsPicked(const std::set<QRgb>&)));
	connect(this, SIGNAL(actorHighlighted(QRgb)),this,SLOT(fireDetectorHighligted(QRgb)));
	DataMinValue=-DBL_MAX;
	DataMaxValue=DBL_MAX;
	mDataMapping=INTEGRAL;
	BinMinValue=-DBL_MAX;
	BinMinValue=DBL_MAX;
	mHaveBinMaxMin = false;
	mAxisDirection=Mantid::Geometry::V3D(0.0,0.0,1.0);
	mAxisUpVector=Mantid::Geometry::V3D(0.0,1.0,0.0);
	//mAxisDirection=Mantid::Geometry::V3D(1.0,0.0,0.0);
	//mAxisUpVector=Mantid::Geometry::V3D(0.0,0.0,1.0);
}

Instrument3DWidget::~Instrument3DWidget()
{
	makeCurrent();
}

/**
 * Set the default Axis direction of the model
 */
void Instrument3DWidget::setAxis(const Mantid::Geometry::V3D& direction,const Mantid::Geometry::V3D& up)
{
	mAxisDirection=direction;
	mAxisUpVector=up;
}

/**
 * This method is the slot when the detectors are picked using mouse. This method emits
 * signals the ids of the detector and the spectra index(not spectra number).
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorsPicked(const std::set<QRgb>& pickedColors)
{
	std::vector<int> detectorIds;

	int iDecId;
	for(std::set<QRgb>::const_iterator it=pickedColors.begin();it!=pickedColors.end();it++)
	{
		iDecId=mInstrumentActor->getDetectorIDFromColor(qRed((*it))*65536+qGreen((*it))*256+qBlue((*it)));
		if(iDecId!=-1)
		{
			detectorIds.push_back(iDecId);
		}

	}
	//convert detector ids to spectra index ids
	const std::vector<int>& spectraIndices = getSpectraIndexList(detectorIds);
	if(detectorIds.size()!=0)
	{
		if(detectorIds.size()==1)
		{
			//emit the detector id
			emit actionDetectorSelected(detectorIds.at(0));
			//emit the spectra id
			emit actionSpectraSelected(spectraIndices.at(0));
		}
		else // If more than one detector selected
		{
			//emit the detector ids
			emit actionDetectorSelectedList(detectorIds);
			//emit the spectra ids
			emit actionSpectraSelectedList(spectraIndices);

		}

	}
}

/**
 * This method is the slot when the detector is highlighted using mouse move. This method emits
 * signals the id of the detector and the spectra index(not spectra number).
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorHighligted(QRgb pickedColor)
{
	if(pickedColor==BLACK)
	{
		emit actionDetectorHighlighted(-1,-1,-1);
		return;
	}
	int iDecId=mInstrumentActor->getDetectorIDFromColor(qRed(pickedColor)*65536+qGreen(pickedColor)*256+qBlue(pickedColor));
	if(iDecId!=-1)
	{
		//convert detector id to spectra index id
		std::vector<int> idDecVec;
		idDecVec.push_back(iDecId);
		const std::vector<int>& indexList = getSpectraIndexList(idDecVec);
		MatrixWorkspace_sptr workspace;
		workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(strWorkspaceName));
		Axis *spectraAxis = workspace->getAxis(1);    // Careful, will throw if a Workspace1D!
		int spectrumNumber = spectraAxis->spectraNo(indexList[0]);
		//		const std::vector<double>& outputdata=workspace->readY(indexList[0]);
		std::vector<int> histIndexList;
		std::vector<double> values;
		histIndexList.push_back(indexList[0]);
		double minval,maxval;
		switch(mDataMapping)
		{
		case SINGLE_BIN:
			this->CollectTimebinValues(this->iTimeBin,histIndexList,minval,maxval,values);
			break;
		case INTEGRAL:
			this->CollectIntegralValues(histIndexList, 0, workspace->blocksize(),minval,maxval,values);
			break;
		}
		//emit the detector id, spectrum number and count
		emit actionDetectorHighlighted(iDecId,spectrumNumber,values[0]);
	}

}

/**
 * This method sets the workspace name input to the widget.
 * @param wsName input workspace name
 */
void Instrument3DWidget::setWorkspace(const std::string& wsName)
{

	strWorkspaceName=wsName;
    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
	//Get the workspace min bin value and max bin value
//  	BinMinValue=DBL_MAX;
//  	BinMaxValue=-DBL_MAX;

    if( !mHaveBinMaxMin )
    {
        int nHist = output->getNumberHistograms();
        for (int i = 0; i < nHist; ++i)
	{
		std::vector<double> values=output->readX(i);
		if(BinMinValue>values[0])
			BinMinValue=values[0];
		else if(BinMaxValue<values[0])
			BinMaxValue=values[0];
		if(BinMinValue>*(values.end()-1))
			BinMinValue=*(values.end()-1);
		else if(BinMaxValue<*(values.end()-1))
			BinMaxValue=*(values.end()-1);
	}
    }
	boost::shared_ptr<Mantid::API::IInstrument> ins = output->getInstrument();
	this->ParseInstrumentGeometry(ins);
	boost::shared_ptr<Mantid::Geometry::IObjComponent> sample=ins->getSample();
	if(sample!=NULL)
		_trackball->setModelCenter(sample->getPos());
	else
		_trackball->setModelCenter(Mantid::Geometry::V3D(0.0,0.0,0.0));
	defaultProjection(); // Calculate and set projection
	AssignColors();
}

/**
 * This method parses the instrument information and creates the actors relating to the detectors.
 */
void Instrument3DWidget::ParseInstrumentGeometry(boost::shared_ptr<Mantid::API::IInstrument> ins)
{
	makeCurrent();
	boost::shared_ptr<GLActorCollection> scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection);
	boost::shared_ptr<std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*> > iObjectMap(new std::map<const boost::shared_ptr<const Mantid::Geometry::Object>,MantidObject*>());
	mInstrumentActor = new InstrumentActor(ins,mFastRendering);
	scene->addActor(mInstrumentActor);	 
	this->setActorCollection(scene);
}

/**
 * This method returns the vector list of all detector id's corresponding to the actor collections.
 * The id's are in the same order as the indexs of the actor in the actor collections. If dectector is not assigned
 * to the actor then -1 is set.
 * @return std::vector<int> list of detector id's
 */
std::vector<int> Instrument3DWidget::getDetectorIDList() const
{
	std::vector<int> idDecVec;
	if(mInstrumentActor)
		mInstrumentActor->getDectorIDList(idDecVec);
	return idDecVec;
}

/**
 * This method returns the Spectra Index list for the input dectector id list.
 * @param idDecVec is list of detector id's
 */
std::vector<int> Instrument3DWidget::getSpectraIndexList(const std::vector<int>& idDecVec) const
{
	if(strWorkspaceName=="")return std::vector<int>();
	MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(strWorkspaceName));

	std::map<int,int> indexMap;
	const std::vector<int>& spectraList = output->spectraMap().getSpectra(idDecVec);

	// There is no direct way of getting histogram index from the spectra id,
	// get the spectra axis and convert form index to spectra number and create
	// a map.
	Axis* spectraAxis = output->getAxis(1);

	for (int i = 0; i < output->getNumberHistograms(); ++i)
	{
		int currentSpec = spectraAxis->spectraNo(i);
		indexMap[currentSpec]=i;
	}
	std::vector<int> indexList;
	for(int i=0;i<spectraList.size();i++)
	{
		if(idDecVec[i]!=-1)
			indexList.push_back(indexMap[spectraList[i]]);
		else
			indexList.push_back(-1);
	}
	return indexList;
}

/**
 * This method assigns colors to the dectors using the values
 * @param minval input minimum value for scaling of the colormap
 * @param maxval input maximum value for scaling of the colormap
 * @param values input values that coorespond to each detector
 * @param colorMap input color map class which is used for looking up the color to be assigned based on the value of detector.
 */
void Instrument3DWidget::setColorForDetectors(double minval,double maxval,const std::vector<double>& values,const GLColorMap& colorMap)
{
	std::vector<boost::shared_ptr<GLColor> > iColorList;
	int noOfColors=colorMap.getNumberOfColors();
	std::vector<double>::size_type nvals = values.size();
  for(std::vector<double>::size_type i = 0; i < nvals; i++)
	{
		int cIndex;
		if(maxval-minval<0.00000001)
		{
		  cIndex = 0;
		}
		else
		{
		  cIndex=floor(((values[i]-minval)/(maxval-minval))*(noOfColors-1));
		}
		if(cIndex<0)
		{
		  cIndex = noOfColors - 1;
		}
		else if(cIndex>(noOfColors-1))
		{
		  cIndex=(noOfColors-1);
		}
		iColorList.push_back(colorMap.getColor(cIndex));

	} // Looping through the dectors/Actors list
	mInstrumentActor->setDetectorColors(iColorList);
}

/**
 * This method collects the values of the given timebin,spectra list and calculates min and max values and collection of values
 * with the spectra list.
 * @param timebin input time bin value
 * @param histogramIndexList input list of historgram index
 * @param minval output minimum value of the histogram time bin value
 * @param maxval output maximum value of the histogram time bin value
 * @param valuesList output list of histogram values corresponding to timebin value
 */
void Instrument3DWidget::CollectTimebinValues(int timebin,const std::vector<int>& histogramIndexList, double& minval,double& maxval,std::vector<double>& valuesList)
{
	MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(strWorkspaceName));
	//Get the spectra timebin out and find min and max
	valuesList.clear();
	minval=DBL_MAX;
	maxval=-DBL_MAX;
	for(int i=0;i<histogramIndexList.size();i++){
		if(histogramIndexList[i]!=-1)
		{
			double value=output->readY(histogramIndexList[i])[timebin];
			valuesList.push_back(value);
			if(value<minval)minval=value;
			if(value>maxval)maxval=value;
		}
		else
		{
			valuesList.push_back(-DBL_MAX);
		}
	}
}

/**
 * This method collects the integral values of the spectra list and calculates min and max values and collection of values
 * with the spectra list.
 * @param histogramIndexList input list of historgram index
 * @param startbin is the starting bin number to integrate
 * @param endbin is the ending bin number to integrate
 * @param minval output minimum value of the histogram time bin value
 * @param maxval output maximum value of the histogram time bin value
 * @param valuesList output list of histogram values corresponding to timebin value
 */
void Instrument3DWidget::CollectIntegralValues(const std::vector<int>& histogramIndexList, int startbin,int endbin,double& minval,double& maxval, std::vector<double>& valuesList)
{
	MatrixWorkspace_sptr output;
    output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(strWorkspaceName));
	//Get the spectra timebin out and find min and max
	valuesList.clear();
	minval=DBL_MAX;
	maxval=-DBL_MAX;
	for(int i=0;i<histogramIndexList.size();i++){
        if(histogramIndexList[i]!=-1)
		{
			double value=0.0;
			const std::vector<double>& outputdata=output->readY(histogramIndexList[i]);
			const std::vector<double>& binvalues=output->readX(histogramIndexList[i]);
			for(int timebin=startbin;timebin<endbin;timebin++){
				if(BinMinValue<=binvalues[timebin]&&BinMaxValue>=binvalues[timebin]){
					if(timebin<outputdata.size())
						value+=outputdata[timebin];
				}
			}
			valuesList.push_back(value);
			if(value<minval)minval=value;
			if(value>maxval)maxval=value;
		}
		else
		{
			valuesList.push_back(-DBL_MAX);
		}
	}
}

/**
 * This method assigns the colors to the detectors using the time bin values.
 */
void Instrument3DWidget::AssignColors()
{
	std::vector<int> detectorList = this->getDetectorIDList();
	if(detectorList.size()==0)return; ///< to check whether any detectors are present
	std::vector<int> histIndexList = this->getSpectraIndexList(detectorList);
	std::vector<double> values;
	double minval,maxval;
	MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(strWorkspaceName));
	switch(mDataMapping)
	{
	case SINGLE_BIN:
		this->CollectTimebinValues(this->iTimeBin,histIndexList,minval,maxval,values);
		break;
	case INTEGRAL:
		this->CollectIntegralValues(histIndexList, 0, output->blocksize(),minval,maxval,values);
		break;
	}
	if(DataMinValue==-DBL_MAX)
		DataMinValue=minval;
	if(DataMaxValue==DBL_MAX)
		DataMaxValue=maxval;
	this->setColorForDetectors(DataMinValue,DataMaxValue,values,this->mColorMap);
	mInstrumentActor->refresh();
	updateGL();
}

/**
 * This method takes the input name as the filename and reads the file for the color index values.
 * NOTE: This method can only read 256 color index with RGB values
 */
void Instrument3DWidget::setColorMapName(const QString & name)
{
  mColorMap.setColorMapFile(name.toStdString());
  AssignColors();
}

/**
 * This method sets the Time bin values. the value has to be greater than zero
 * @param value input is the time bin value
 */
void Instrument3DWidget::setTimeBin(int value)
{
	if(value>0)
	{
		this->iTimeBin=value;
		AssignColors();
	}
}


/**
 * Returns workspace name
 */
std::string Instrument3DWidget::getWorkspaceName() const
{
	return strWorkspaceName;
}

/**
 * Returns Colormap
 */
GLColorMapQwt Instrument3DWidget::getColorMap()const
{
	return mColorMap;
}

/**
 * This method takes the input name as the min value Colormap scale.
 */
void Instrument3DWidget::setColorMapMinValue(double minValue)
{
	this->DataMinValue=minValue;
	AssignColors();
}

/**
 * This method takes the input name as the min value Colormap scale.
 */
void Instrument3DWidget::setColorMapMaxValue(double maxValue)
{
	this->DataMaxValue=maxValue;
	AssignColors();
}

/**
 * This method returns min value. by default will be min value in the current timebin
 */
double Instrument3DWidget::getDataMinValue() const
{
	return this->DataMinValue;
}

/**
 * This method returns the max value. by default will be max value in the current timebin
 */
double Instrument3DWidget::getDataMaxValue() const
{
	return this->DataMaxValue;
}

/**
 * Returns the current minimum bin value
 */
double Instrument3DWidget::getBinMinValue() const
{
  return this->BinMinValue;
}

double Instrument3DWidget::getBinMaxValue() const
{
    return this->BinMaxValue;
}

/**
 * This method sets the Data mapping type for the color mapping.
 */
void Instrument3DWidget::setDataMappingType(DataMappingType dmType)
{
	mDataMapping=dmType;
	AssignColors();
}

void Instrument3DWidget::setDataMappingIntegral(double minValue,double maxValue)
{
	this->BinMinValue=minValue;
	this->BinMaxValue=maxValue;
	mHaveBinMaxMin = true;
	setDataMappingType(INTEGRAL);
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
	iTimeBin=0;
	strWorkspaceName="";
	DataMinValue=-DBL_MAX;
	DataMaxValue=DBL_MAX;
	mHaveBinMaxMin = false;
	mDataMapping=INTEGRAL;
	mAxisDirection=Mantid::Geometry::V3D(0.0,0.0,1.0);
	mAxisUpVector=Mantid::Geometry::V3D(0.0,1.0,0.0);
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

/**
 * Returns the boundig box of the scene
 * @param minBound :: output min point of the bounding box of scene
 * @param maxBound :: output max point of the bounding box of scene
 */ 
void Instrument3DWidget::getBoundingBox(Mantid::Geometry::V3D& minBound, Mantid::Geometry::V3D& maxBound)
{
	mInstrumentActor->getBoundingBox(minBound,maxBound);
}
