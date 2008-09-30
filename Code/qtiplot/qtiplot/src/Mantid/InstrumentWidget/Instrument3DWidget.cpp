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
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Instrument3DWidget::Instrument3DWidget(QWidget* parent):GL3DWidget(parent)
{
	iTimeBin=0;
	strWorkspaceName="";
	connect(this, SIGNAL(actorPicked(GLActor*)), this, SLOT(fireDetectorPicked(GLActor*)));
}

Instrument3DWidget::~Instrument3DWidget()
{
}

/**
 * This method is the slot when the detector is picked using mouse. This method emits
 * signals the id of the detector and the spectra index(not spectra number). 
 * @param pickedActor the input passed by the the signal.
 */
void Instrument3DWidget::fireDetectorPicked(GLActor* pickedActor)
{
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
			//emit the detector id
			emit actionDetectorSelected(iDec->getID());
			//convert detector id to spectra index id
			std::vector<int> idDecVec;
			idDecVec.push_back(iDec->getID());
			std::vector<int> indexList = getSpectraIndexList(idDecVec);
			//emit the spectra id
			emit actionSpectraSelected(indexList[0]);
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
	boost::shared_ptr<Mantid::API::Instrument> ins = output->getInstrument();
	this->ParseInstrumentGeometry(ins);
}

/**
 * This method parses the instrument information and creates the actors relating to the detectors.
 */
void Instrument3DWidget::ParseInstrumentGeometry(boost::shared_ptr<Mantid::API::Instrument> ins)
{
	boost::shared_ptr<GLActorCollection> scene=boost::shared_ptr<GLActorCollection>(new GLActorCollection);
	std::queue<Component *> CompList;
	CompList.push(ins.get());
	while(!CompList.empty())
	{
		Component* tmp = CompList.front();
		CompList.pop();
		std::cout<<" Component: "<<tmp->getName()<<std::endl;
		std::cout<<" Component Type:"<<tmp->type()<<std::endl;
		if(tmp->type()=="PhysicalComponent" ||tmp->type()=="DetectorComponent"){
			boost::shared_ptr<MantidObject> obj(new MantidObject(dynamic_cast<ObjComponent*>(tmp)));
			boost::shared_ptr<GLColor> col(new GLColor(1.0,0.5,0.0,1.0));
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
	AssignColors();
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
 * This method assigns the colors to the detectors using the time bin values.
 */
void Instrument3DWidget::AssignColors()
{
	std::vector<int> detectorList = this->getDetectorIDList();
	std::vector<int> histIndexList = this->getSpectraIndexList(detectorList);
	std::vector<double> values;	
	double minval,maxval;
	this->CollectTimebinValues(this->iTimeBin,histIndexList,minval,maxval,values);
	std::cout<<"Min and Max Values: "<<minval<<" "<<maxval<<std::endl;
	this->setColorForDetectors(minval,maxval,values,this->mColorMap);
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