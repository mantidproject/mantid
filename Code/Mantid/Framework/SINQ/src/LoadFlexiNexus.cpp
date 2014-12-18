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
    xData.resize(spectraLength);
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
    yData.resize(nSpectra);
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
    ws->getSpectrum(i)->setSpectrumNo(static_cast<specid_t>(yData[i]));
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

  // read the data first
  fin->getDataCoerce(data);

  Info inf = fin->getInfo();

  std::vector<MDHistoDimension_sptr> dimensions;
  for(int k = static_cast<int>(inf.dims.size())-1; k >=0; k--)
  {
    dimensions.push_back(makeDimension(fin,k,static_cast<int>(inf.dims[k])));
  }

  MDHistoWorkspace_sptr ws (new MDHistoWorkspace(dimensions));


  signal_t *dd = ws->getSignalArray();
  signal_t *ddE=  ws->getErrorSquaredArray();

  // assign data
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

}
int LoadFlexiNexus::calculateCAddress(int *pos, int* dim, int rank)
{
  int result = (int)pos[rank - 1];
  for(int i = 0; i < rank -1; i++){
    int mult = 1;
    for(int j = rank -1; j > i; j--){
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
  static const char *axisNames[] = {"x","y","z"};
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
    dData.resize(length);
    for(int i = 0; i < length; i++){
      dData[i] = (double)i;
    }
  } else {
    if(safeOpenpath(fin,it->second)){
      fin->getDataCoerce(dData);
    } else {
      dData.resize(length);
      for(int i = 0; i < length; i++){
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
  for(it = dictionary.begin(); it != dictionary.end(); ++it){
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
  } catch(NeXus::Exception &){
    getLogger().error("NeXus path " + path + " kaputt");
    return 0;
  }
  return 1;
}
