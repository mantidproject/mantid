#ifdef WIN32
#include <windows.h>
#endif
#include "GL3DWidget.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"  
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/CompAssembly.h"
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
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Instrument3DWidget::Instrument3DWidget(QWidget* parent):GL3DWidget(parent)
{
	iTimeBin=0;
	strWorkspaceName="";
	connect(this, SIGNAL(actorsPicked(std::vector<GLActor*>)), this, SLOT(fireDetectorsPicked(std::vector<GLActor*>)));
	connect(this, SIGNAL(actorHighlighted(GLActor*)),this,SLOT(fireDetectorHighligted(GLActor*)));
	DataMinValue=-DBL_MAX;
	DataMaxValue=DBL_MAX;
	mDataMapping=INTEGRAL;
}

Instrument3DWidget::~Instrument3DWidget()
{
}

/**
 * This method is the slot when the detectors are picked using mouse. This method emits
 * signals the ids of the detector and the spectra index(not spectra number). 
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorsPicked(std::vector<GLActor*> pickedActor)
{
	std::vector<int> detectorIds;

	for(std::vector<GLActor*>::iterator it=pickedActor.begin();it!=pickedActor.end();it++)
	{
		boost::shared_ptr<GLObject> tmpGLObject=(*it)->getRepresentation();
		if(tmpGLObject->type()=="MantidObject")
		{
			//type cast to the mantid object
			MantidObject* tmpMantidObject=dynamic_cast<MantidObject*>(tmpGLObject.get());
			//get the component
			ObjComponent* tmpObjComp=tmpMantidObject->getComponent();
			//check the component type if its detector or not
			if(tmpObjComp->type()=="PhysicalComponent" ||tmpObjComp->type()=="DetectorComponent")
			{
				Mantid::Geometry::Detector*  iDec=(dynamic_cast<Mantid::Geometry::Detector *>(tmpObjComp));
				detectorIds.push_back(iDec->getID());
			}

		}
	}
	//convert detector ids to spectra index ids
	std::vector<int> spectraIndices = getSpectraIndexList(detectorIds);
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
void Instrument3DWidget::fireDetectorHighligted(GLActor* pickedActor)
{
	if(pickedActor==NULL)
	{
		emit actionDetectorHighlighted(-1,-1,-1);
		return;
	}
	boost::shared_ptr<GLObject> tmpGLObject=pickedActor->getRepresentation();
	if(tmpGLObject->type()=="MantidObject")
	{
		//type cast to the mantid object
		MantidObject* tmpMantidObject=dynamic_cast<MantidObject*>(tmpGLObject.get());
		//get the component
		ObjComponent* tmpObjComp=tmpMantidObject->getComponent();
		//check the component type if its detector or not
		if(tmpObjComp->type()=="PhysicalComponent" ||tmpObjComp->type()=="DetectorComponent")
		{
			Mantid::Geometry::Detector*  iDec=(dynamic_cast<Mantid::Geometry::Detector *>(tmpObjComp));
			//convert detector id to spectra index id
			std::vector<int> idDecVec;
			idDecVec.push_back(iDec->getID());
			std::vector<int> indexList = getSpectraIndexList(idDecVec);
			Workspace_sptr workspace;
			workspace = AnalysisDataService::Instance().retrieve(strWorkspaceName);
			Axis *spectraAxis = workspace->getAxis(1);    // Careful, will throw if a Workspace1D!
			int spectrumNumber = spectraAxis->spectraNo(indexList[0]);
			std::vector<double> outputdata=workspace->readY(indexList[0]);
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
			emit actionDetectorHighlighted(iDec->getID(),spectrumNumber,values[0]);
		}

	}
}

/**
 * This method sets the workspace name input to the widget.
 * @param wsName input workspace name
 */
void Instrument3DWidget::setWorkspace(std::string wsName)
{
	strWorkspaceName=wsName;
    // Get back the saved workspace
    Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(wsName);
	//Get the workspace min bin value and max bin value
	BinMinValue=DBL_MAX;
	BinMaxValue=-DBL_MAX;
	for (int i = 0; i < output->getNumberHistograms(); ++i)
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
	boost::shared_ptr<Mantid::API::Instrument> ins = output->getInstrument();
	this->ParseInstrumentGeometry(ins);
	defaultProjection(); // Calculate and set projection
	AssignColors();
}

/**
 * This method parses the instrument information and creates the actors relating to the detectors.
 */
void Instrument3DWidget::ParseInstrumentGeometry(boost::shared_ptr<Mantid::API::Instrument> ins)
{
	boost::shared_ptr<GLActorCollection> scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection);
	makeCurrent();
	std::queue<Component *> CompList;
	CompList.push(ins.get());
	boost::shared_ptr<GLColor> col(new GLColor(1.0,0.5,0.0,1.0));
	while(!CompList.empty())
	{
		Component* tmp = CompList.front();
		CompList.pop();
		//std::cout<<" Component: "<<tmp->getName()<<std::endl;
		//std::cout<<" Component Type:"<<tmp->type()<<std::endl;
		if(tmp->type()=="PhysicalComponent" ||tmp->type()=="DetectorComponent"){
			boost::shared_ptr<MantidObject> obj(new MantidObject(dynamic_cast<ObjComponent*>(tmp)));
			GLActor* actor1=new GLActor();
			actor1->setRepresentation(obj);
			actor1->setPos(0.0,0.0,0.0);
			actor1->setColor(col);
			scene->addActor(actor1);
		} else if(tmp->type()=="Instrument"){
			Instrument *tmpIns=dynamic_cast<Instrument*>(tmp);
			for(int idx=0;idx<tmpIns->nelements();idx++)
			{
				CompList.push((*tmpIns)[idx]);
			}
		} else if(tmp->type()=="CompAssembly"){
			CompAssembly *tmpAssem=dynamic_cast<CompAssembly*>(tmp);
			for(int idx=0;idx<tmpAssem->nelements();idx++)
			{
				CompList.push((*tmpAssem)[idx]);
			}
		} 
	}	
	this->setActorCollection(scene);
}

/**
 * This method returns the vector list of all detector id's corresponding to the actor collections. 
 * The id's are in the same order as the indexs of the actor in the actor collections. If dectector is not assigned
 * to the actor then -1 is set.
 * @return std::vector<int> list of detector id's
 */
std::vector<int> Instrument3DWidget::getDetectorIDList()
{
	std::vector<int> idDecVec;
    Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(strWorkspaceName);
	int count=scene->getNumberOfActors();
	/* Here we are only filtering the detectors to be passed in the detector list */
	/* Skipping the monitors because they have high values of neutron count*/
	for(int i=0;i<count;i++){
		GLActor* tmpActor=scene->getActor(i);
		if(tmpActor->getRepresentation()->type()=="MantidObject"){
			boost::shared_ptr<GLObject> mObj = tmpActor->getRepresentation();
			Mantid::Geometry::ObjComponent* objComp= (dynamic_cast<MantidObject*>(mObj.get()))->getComponent();
			if(objComp->type()=="PhysicalComponent" ||objComp->type()=="DetectorComponent"){
				Mantid::Geometry::Detector*  iDec=(dynamic_cast<Mantid::Geometry::Detector *>(objComp));
				if(!iDec->isMonitor())
					idDecVec.push_back(iDec->getID());
				else
					idDecVec.push_back(-1);
			}else{
				idDecVec.push_back(-1);
			}
		}else{
			idDecVec.push_back(-1);
		}
	}	
	return idDecVec;
}

/**
 * This method returns the Spectra Index list for the input dectector id list.
 * @param idDecVec is list of detector id's
 */
std::vector<int> Instrument3DWidget::getSpectraIndexList(std::vector<int> idDecVec)
{
	Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(strWorkspaceName);
	boost::shared_ptr<SpectraDetectorMap> specMap=output->getSpectraMap(); 
	
	std::map<int,int> indexMap;
	std::vector<int> spectraList=specMap->getSpectra(idDecVec);//dynamic_cast<std::vector<Mantid::Geometry::IDetector*>(idDecVec));

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
void Instrument3DWidget::setColorForDetectors(double minval,double maxval,std::vector<double> values,GLColorMap colorMap)
{
	int count=scene->getNumberOfActors();
	if(count!=values.size())std::cout<<"Error: The detectors "<<count<<" are not equal to values "<<values.size()<<std::endl;
	for(int i=0;i<count;i++)
	{
		GLActor* tmpActor=scene->getActor(i);
		int cIndex=floor(((values[i]-minval)/(maxval-minval))*255);
		if(cIndex<0)
		{
			cIndex=0;
		}
		else if(cIndex>255)
		{
			cIndex=255;
		}
		tmpActor->setColor(colorMap.getColor(cIndex));
	} // Looping through the dectors/Actors list
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
void Instrument3DWidget::CollectTimebinValues(int timebin, std::vector<int> histogramIndexList, double& minval,double& maxval, std::vector<double>& valuesList)
{
	Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(strWorkspaceName);
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
void Instrument3DWidget::CollectIntegralValues(std::vector<int> histogramIndexList, int startbin,int endbin,double& minval,double& maxval, std::vector<double>& valuesList)
{
	Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(strWorkspaceName);
	//Get the spectra timebin out and find min and max
	valuesList.clear();
	minval=DBL_MAX;
	maxval=-DBL_MAX;
	for(int i=0;i<histogramIndexList.size();i++){
		if(histogramIndexList[i]!=-1)
		{
			double value=0.0;
			std::vector<double> outputdata=output->readY(histogramIndexList[i]);
			std::vector<double> binvalues=output->readX(histogramIndexList[i]);
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
	std::vector<int> histIndexList = this->getSpectraIndexList(detectorList);
	std::vector<double> values;	
	double minval,maxval;
	Workspace_sptr output;
    output = AnalysisDataService::Instance().retrieve(strWorkspaceName);
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
	//std::cout<<"Min and Max Values: "<<minval<<" "<<maxval<<std::endl;
	this->setColorForDetectors(DataMinValue,DataMaxValue,values,this->mColorMap);
	scene->refresh();
	updateGL();
}

/**
 * This method takes the input name as the filename and reads the file for the color index values.
 * NOTE: This method can only read 256 color index with RGB values
 */
void Instrument3DWidget::setColorMapName(std::string name)
{
	mColorMap.setColorMapFile(name);
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
std::string Instrument3DWidget::getWorkspaceName()
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
double Instrument3DWidget::getDataMinValue()
{
	return this->DataMinValue;
}

/**
 * This method returns the max value. by default will be max value in the current timebin
 */
double Instrument3DWidget::getDataMaxValue()
{
	return this->DataMaxValue;
}

/**
 * This method sets the Data mapping type for the color mapping.
 */
void Instrument3DWidget::setDataMappingType(DataMappingType dmType)
{
	mDataMapping=dmType;
	AssignColors();
}

void Instrument3DWidget::setDataMappingIntegral(int minValue,int maxValue)
{
	this->BinMinValue=minValue;
	this->BinMaxValue=maxValue;
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

void Instrument3DWidget::setView(V3D pos,double xmax,double ymax,double zmax,double xmin,double ymin,double zmin)
{
	//get the centre of the bounding box
	V3D boundCentre;
	double xhalf=(xmax-xmin)/2.0;
	double yhalf=(ymax-ymin)/2.0;
	double zhalf=(zmax-zmin)/2.0;
	boundCentre[0]=-1*(xmin+xhalf);
	boundCentre[1]=-1*(ymin+yhalf);
	boundCentre[2]=-1*(zmin+zhalf);

	double vxmin,vxmax,vymin,vymax,vzmin,vzmax;
	_viewport->getProjection(vxmin,vxmax,vymin,vymax,vzmin,vzmax);

	//vector from center to bounding box center
	V3D vcb=pos-boundCentre;
	vcb.normalize();
	//get the rotation about zaxis
	V3D zaxis(0,0,-1);
	double angle=vcb.angle(zaxis);
	V3D axis=vcb.cross_prod(zaxis);
	axis.normalize();
	double s=sin(angle/2);
	Quat rotation(angle,axis);
	_trackball->setRotation(rotation);
	_trackball->_scaleFactor=1.0;
	_trackball->setTranslation(V3D(0.0,0.0,0.0));
	V3D minval(xmin,ymin,zmin);
	V3D maxval(xmax,ymax,zmax);
	rotation.rotate(minval);
	rotation.rotate(maxval);
	//_viewport->setOrtho(xmin,xmax,ymin,ymax,zmin*-1,zmax*-1);
	_viewport->setOrtho(minval[0],maxval[0],minval[1],maxval[1],minval[2]*-1,maxval[2]*-1);
	_viewport->issueGL();
	update();	
}