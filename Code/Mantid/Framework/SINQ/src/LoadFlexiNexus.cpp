/*WIKI*

== Description ==
This algorithm is a flexible NeXus file loader. Data loading is 
driven by a dictionary. Correspondingly the algorithm takes as 
arguments: a filename, a path to a dictionary file and an output 
workspace name. 

The dictionary itself is a list of key=value pairs, one per line.  
Normally dictionary entries take the form key-path-into-Nexus-file. 
The default action is to store the data found at path-into-NeXus-file 
under key key in the Run information of the result workspace. But some 
keys are interpreted specially:
;data=path-into-nexus-file
: This is a required entry. path-into-nexus-file is the path in the NeXus file to the data which is the main bulk workspace data. Usually the counts. From the dimensionality of this data the type of result workspace is determined. If the rank is <= 2, then a Workspace2D is created, else a MDHistoWorkspace.
;x-axis=path-into-nexus-file
: The data found under the path into the NeXus file will be used as axis 0 on the dataset
;x-axis-name=text
: The text specified will become the name of the axis 0
;y-axis=path-into-nexus-file
: The data found under the path into the NeXus file will be used as axis 1 on the dataset
;y-axis-name=text
: The text specified will become the name of the axis 1
;z-axis=path-into-nexus-file
: The data found under the path into the NeXus file will be used as axis 2 on the dataset
;z-axis-name=text
: The text specified will become the name of the axis 0
;title=path-into-nexus-file or text
: If the value contains a / then it is interpreted as a path into the NeXus file, the value of which will be stored as workspace title. Else the text value will be stored as the title name directly.
;sample=path-into-nexus-file or text
: If the value contains a / then it is interpreted as a path into the NeXus file, the value of which will be stored as workspace sample. Else the text value will be stored as the sample name directly.

Please note that the dimensions on the MDHistoWorkspace are inverted when compared with the ones in the NeXus file. This is a fix which allows to efficiently transfer the NeXus data in C storage order into the MDHistoWorkspace which has fortran storage order.

*WIKI*/
#include "MantidSINQ/LoadFlexiNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadFlexiNexus)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace::NeXus;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages


void LoadFlexiNexus::init()
{

      std::vector<std::string> exts;
      exts.push_back(".hdf");
      exts.push_back(".h5");
      exts.push_back("");


      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "A NeXus file");

      std::vector<std::string> exts2;
      exts2.push_back(".txt");
      exts2.push_back(".dic");
      exts2.push_back("");

      declareProperty(new FileProperty("Dictionary", "", FileProperty::Load, exts2),
        "A Dictionary for controlling NeXus loading");

      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

      /**
       * bloody hack to set the workspace name from python.....
       */
      declareProperty("WSName", "NULL","Optional WS Name");

}

void LoadFlexiNexus::exec()
{
  
  std::string filename = getProperty("Filename");
  std::string dictname = getProperty("Dictionary");
  g_log.information() << "Running FlexiNexus for " << filename << " with  " << dictname << std::endl;
  
  loadDictionary(getProperty("Dictionary"));

  /* print the dictionary for debugging..........
  for(std::map<std::string, std::string>::const_iterator it = dictionary.begin();
      it!= dictionary.end(); it++){
    std::cout << it->first << "\t" << it->second << std::endl;
  }
  */
  
  /*
   * This is a bloody hack to get the output workspace name into the
   * parameter list to be used from python.
   */
  std::string wName = getProperty("WSName");
  if(wName != "NULL"){
	Property *p = getPointerToProperty("OutputWorkspace");
	p->setValue(wName);
  }


  File fin(filename);
  readData(&fin);

}

void LoadFlexiNexus::loadDictionary(std::string dictFile)
{
  std::ifstream in(dictFile.c_str(), std::ifstream::in);
  std::string line, key, value;

  while(std::getline(in,line)){
	  // skip comments
    if(line[0] == '#'){
      continue;
    }
    // skip empty lines
    if(line.length() < 2){
    	continue;
    }
    std::istringstream l(line);
    std::getline(l,key,'=');
    std::getline(l,value,'\n');
    boost::algorithm::trim(key);
    boost::algorithm::trim(value);
    dictionary[key] = value;
  }
  in.close();
}


void LoadFlexiNexus::readData(NeXus::File *fin)
{
	Mantid::DataObjects::Workspace2D_sptr ws;
	std::map<std::string,std::string>::const_iterator it;

	if((it = dictionary.find("data")) == dictionary.end()){
		throw std::runtime_error("Required dictionary element data not found");
	}

	// inspect the data element and create WS matching dims
	if(!safeOpenpath(fin, it->second)){
		throw std::runtime_error("data NeXus path not found!");
	}

	Info inf = fin->getInfo();
	size_t rank = inf.dims.size();
	boost::shared_array<int> data;

	if(rank <= 2){
		load2DWorkspace(fin);
	} else {
		loadMD(fin);
	}
}

void LoadFlexiNexus::load2DWorkspace(NeXus::File *fin)
{
	Mantid::DataObjects::Workspace2D_sptr ws;
	int nSpectra, spectraLength;
    std::vector<int> data;

    // read the data first
    fin->getDataCoerce(data);

    Info inf = fin->getInfo();
	if(inf.dims.size() == 1){
		nSpectra = 1;
		spectraLength = static_cast<int>(inf.dims[0]);
	} else {
		nSpectra = static_cast<int>(inf.dims[0]);
		spectraLength = static_cast<int>(inf.dims[1]);
	}

	// need to locate x-axis data too.....
	std::map<std::string,std::string>::const_iterator it;
	std::vector<double> xData;
	if((it = dictionary.find("x-axis")) == dictionary.end()){
		xData.reserve(spectraLength);
		for(int i = 0; i < spectraLength; i++){
			xData[i] = (double)i;
		}
	} else {
		if(safeOpenpath(fin,it->second)){
			fin->getDataCoerce(xData);
		}
	}

	// need to locate y-axis data too.....
	std::vector<double> yData;
	if((it = dictionary.find("y-axis")) == dictionary.end()){
		yData.reserve(nSpectra);
		for(int i = 0; i < nSpectra; i++){
			yData[i] = (double)i;
		}
	} else {
		if(safeOpenpath(fin,it->second)){
			fin->getDataCoerce(yData);
		}
	}

	// fill the data.......
	ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
	(WorkspaceFactory::Instance().create("Workspace2D",nSpectra,spectraLength,spectraLength));
	for(int i = 0; i < nSpectra; i++){
		Mantid::MantidVec& Y = ws->dataY(i);
		for(int j = 0; j < spectraLength; j++){
			Y[j] = data[spectraLength*i+j];
		}
		// Create and fill another vector for the errors, containing sqrt(count)
		Mantid::MantidVec& E = ws->dataE(i);
		std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
		ws->setX(i, xData);
//Xtof		ws->getAxis(1)->spectraNo(i)= i;
		ws->getAxis(1)->setValue(i,yData[i]);
	}
    ws->setYUnit("Counts");


	// assign an x-axis-name
	if((it = dictionary.find("x-axis-name")) == dictionary.end()){
		const std::string xname("no axis name found");
		ws->getAxis(0)->title() = xname;
	} else {
		const std::string xname(it->second);
		ws->getAxis(0)->title() = xname;
	}

	addMetaData(fin,ws, (ExperimentInfo_sptr) ws);

	// assign the workspace
	setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(ws));

}
  
void LoadFlexiNexus::loadMD(NeXus::File *fin)
{
    std::vector<double> data;
	std::map<std::string,std::string>::const_iterator it;

    // read the data first
    fin->getDataCoerce(data);

	Info inf = fin->getInfo();
/*
    std::vector<MDHistoDimension_sptr> dimensions;
    for(int k = 0; k < inf.dims.size(); ++k)
    {
      dimensions.push_back(makeDimension(fin,k,inf.dims[k]));
    }
*/
    std::vector<MDHistoDimension_sptr> dimensions;
    for(int k = static_cast<int>(inf.dims.size())-1; k >=0; k--)
    {
      dimensions.push_back(makeDimension(fin,k,static_cast<int>(inf.dims[k])));
    }

    MDHistoWorkspace_sptr ws (new MDHistoWorkspace(dimensions));

//    indexMaker = new size_t[inf.dims.size()];
//    size_t *shitDim = new size_t[inf.dims.size()];
//    for(int i = 0; i < inf.dims.size(); i++){
//    	shitDim[i] = inf.dims[i];
//    }
//    Utils::NestedForLoop::SetUpIndexMaker(inf.dims.size(), indexMaker, shitDim);
//    delete shitDim;

    signal_t *dd = ws->getSignalArray();
    signal_t *ddE=  ws->getErrorSquaredArray();

    // assign tata
    for(size_t i = 0; i < data.size(); i++){
    	dd[i] = data[i];
    	ddE[i] = dblSqrt(data[i]);
    }

    if(ws->getNumExperimentInfo() == 0){
    	ws->addExperimentInfo((ExperimentInfo_sptr)new ExperimentInfo());
    }
    addMetaData(fin,ws,ws->getExperimentInfo(0));

    // assign the workspace
	setProperty("OutputWorkspace",ws);

	//delete indexMaker;
}
int LoadFlexiNexus::calculateCAddress(int *pos, int* dim, int rank)
{
	int result, mult;
	int i, j;

	result = (int)pos[rank - 1];
	for(i = 0; i < rank -1; i++){
		mult = 1;
		for(j = rank -1; j > i; j--){
			mult *= dim[j];
		}
		if((int)pos[i] < dim[i] && (int)pos[i] > 0){
			result += mult*(int)pos[i];
		}
	}
	return result;
}
int LoadFlexiNexus::calculateF77Address(int *, int)
{
    // --- Unused code ---
	// int result = 0;

	// for(int i = 0; i < rank; i++){
	// 	result += pos[i]*indexMaker[i];
	// }
	// return result;
  return 0;
}

double LoadFlexiNexus::dblSqrt(double in)
{
  return sqrt(in);
}

MDHistoDimension_sptr LoadFlexiNexus::makeDimension(NeXus::File *fin, int index, int length)
{
	static char *axisNames[] = {"x","y","z"};
	std::map<std::string,std::string>::const_iterator it;

	// get a name
	std::string name(axisNames[index]);
	name +=  "axis-name";
	if((it = dictionary.find(name)) != dictionary.end() ){
		name = it->second;
	} else {
		name = axisNames[index];
	}

	// get axis data
	std::string dataName(axisNames[index]);
	dataName += "-axis";
	std::vector<double> dData;
	if((it = dictionary.find(dataName)) == dictionary.end()){
		for(int i = 0; i < length; i++){
			dData.reserve(length);
			dData[i] = (double)i;
		}
	} else {
		if(safeOpenpath(fin,it->second)){
			fin->getDataCoerce(dData);
		} else {
			for(int i = 0; i < length; i++){
				dData.reserve(length);
				dData[i] = (double)i;
			}
		}
	}
	coord_t min, max, tmp;
	min = static_cast<coord_t>(dData[0]);
	max = static_cast<coord_t>(dData[length-1]);
	if(min > max){
		tmp = max;
		max = min;
		min = tmp;
		g_log.notice("WARNING: swapped axis values on " + name);
	}
    return MDHistoDimension_sptr(new MDHistoDimension(name, name, "", min, max, length));
}
	void LoadFlexiNexus::addMetaData(NeXus::File *fin, Workspace_sptr ws, ExperimentInfo_sptr info)
	{
		std::map<std::string,std::string>::const_iterator it;

		// assign a title
		if((it = dictionary.find("title")) == dictionary.end()){
			const std::string title("No title found");
			ws->setTitle(title);
		} else {
			if(it->second.find('/') == it->second.npos){
				const std::string title(it->second);
				ws->setTitle(title);
			} else {
				if(safeOpenpath(fin,it->second)){
					const std::string title = fin->getStrData();
					ws->setTitle(title);
				}
			}
		}

		// assign a sample name
		std::string sample;
		if((it = dictionary.find("sample")) == dictionary.end()){
			sample = "No sample found";
		} else {
			if(it->second.find('/') == it->second.npos){
				sample = it->second;
			} else {
				if(safeOpenpath(fin,it->second)) {
					sample = fin->getStrData();
				} else {
					sample = "Sampe plath not found";
				}
			}
		}
		info->mutableSample().setName(sample);

		/**
		 * load all the extras into the Run information
		 */
		Run& r = info->mutableRun();
		std::set<std::string> specialMap = populateSpecialMap();
		for(it = dictionary.begin(); it != dictionary.end(); it++){
			if(specialMap.find(it->first) == specialMap.end()){
				// not in specials!
				if(it->second.find('/') == it->second.npos){
					r.addProperty(it->first, it->second, true);
				} else {
					if(safeOpenpath(fin, it->second)){
						NeXus::Info inf = fin->getInfo();
						if(inf.type == ::NeXus::CHAR){
							std::string data = fin->getStrData();
							r.addProperty(it->first,data, true);
						} else if(inf.type == ::NeXus::FLOAT32 || inf.type == ::NeXus::FLOAT64){
							std::vector<double> data;
							fin->getDataCoerce(data);
							r.addProperty(it->first,data, true);
						} else {
							std::vector<int> data;
							fin->getDataCoerce(data);
							r.addProperty(it->first,data, true);
						}
					}
				}
			}
		}

		// const std::vector<Kernel::Property *> props = r.getProperties();
		// for(int i = 0; i < props.size(); i++){
		//   Property *prop = props[i];
		//   getLogger().error("Found property: " + prop->name() + " with value: " + prop->value());
                // }

	}
std::set<std::string> LoadFlexiNexus::populateSpecialMap()
{
	std::set<std::string> specialMap;

	specialMap.insert("title");
	specialMap.insert("data");
	specialMap.insert("sample");
	specialMap.insert("x-axis");
	specialMap.insert("x-axis-name");
	specialMap.insert("y-axis");
	specialMap.insert("y-axis-name");
	specialMap.insert("z-axis");
	specialMap.insert("z-axis-name");

	return specialMap;
}

int LoadFlexiNexus::safeOpenpath(NeXus::File *fin, std::string path)
{
	try {
		fin->openPath(path);
	} catch(NeXus::Exception & ne){
		getLogger().error("NeXus path " + path + " kaputt");
		return 0;
	}
	return 1;
}
void LoadFlexiNexus::initDocs()
{
    this->setWikiSummary("Loads a NeXus file directed by a dictionary file");
}
