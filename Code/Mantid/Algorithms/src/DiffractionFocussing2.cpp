//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <cfloat>
#include <fstream>
#include "MantidKernel/VectorHelper.h"
#include <iterator>
#include <numeric>

namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(DiffractionFocussing2)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;

// Get a reference to the logger
Logger& DiffractionFocussing2::g_log = Logger::get("DiffractionFocussing2");

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void DiffractionFocussing2::init()
{
  declareProperty(new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input));
  declareProperty(new API::WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output));

  declareProperty("GroupingFileName","",new FileValidator(std::vector<std::string>(1,"cal")));
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void DiffractionFocussing2::exec()
{
  // retrieve the properties
  std::string groupingFileName=getProperty("GroupingFileName");

  // Get the input workspace
  inputW = getProperty("InputWorkspace");
  nPoints=inputW->readX(0).size();
  nHist=inputW->getNumberHistograms();

  readGroupingFile(groupingFileName);

  determineRebinParameters();

  MatrixWorkspace_sptr out=API::WorkspaceFactory::Instance().create(inputW,nGroups,nPoints,nPoints-1);

  // Now the real work
  int group;
  std::vector<bool> flags(nGroups,true); //Flag to determine whether the X for a group has been set
  std::vector<double> ynew,enew;         // Temp vectors for rebinning

  for (int i=0;i<nHist;i++)
  {
  	//Check whether this spectra is in a valid group
  	group=spectra_group[i];
  	if (group==-1) // Not in a group
  		continue;
  	//Get reference to its old X,Y,and E.
  	const std::vector<double>& Xin=inputW->readX(i);
  	const std::vector<double>& Yin=inputW->readY(i);
  	const std::vector<double>& Ein=inputW->readE(i);
  	// Get the group
  	group2xvectormap::iterator it=group2xvector.find(group);
  	group2xvectormap::difference_type dif=std::distance(group2xvector.begin(),it);
  	const std::vector<double>& Xout=((*it).second);
  	// Assign the new X axis only once (i.e when this group is encountered the first time
  	if (flags[dif])
  	{
  		out->dataX(static_cast<int>(dif))=Xout;
  		flags[dif]=false;
  	}
  	// Get the references to Y and E output and rebin
  	std::vector<double>& Yout=out->dataY(static_cast<int>(dif));
  	std::vector<double>& Eout=out->dataE(static_cast<int>(dif));
  	try
  	{
  		rebin(Xin,Yin,Ein,Xout,ynew,enew,false);
  	}catch(...)
  	{
  		std::ostringstream mess;
  		mess << "Error in rebinning process for spectrum:" << i;
  		std::runtime_error(mess.str());
  		mess.str("");
  	}
  	// Add the new Y and new E for this spectrum to the output group
  	std::transform(Yout.begin(),Yout.end(),ynew.begin(),Yout.begin(),std::plus<double>());
  	std::transform(Eout.begin(),Eout.end(),enew.begin(),Eout.begin(),SumSquares<double>());
  	//
  	progress(static_cast<double>(i)/101.0);
  }

  // Now propagate the errors.
  // Pointer to sqrt function
  typedef double (*uf)(double);
  uf rs=std::sqrt;
  for (int i=0;i<nGroups;i++)
  {
  	std::vector<double>& Eout=out->dataE(i);
  	std::transform(Eout.begin(),Eout.end(),Eout.begin(),rs);
  }

  progress(1.0);
  //Do some cleanup because destructor not called.
  spectra_group.clear();
  group2xvector.clear();
  group2minmax.clear();

  setProperty("OutputWorkspace",out);
  return;
}

/// Reads in the file with the grouping information
/// @param groupingFilename The file that contains the group information
///
void DiffractionFocussing2::readGroupingFile(const std::string& groupingFileName)
{
  std::ifstream grFile(groupingFileName.c_str());
  if (!grFile.is_open())
  {
    g_log.error() << "Unable to open grouping file " << groupingFileName << std::endl;
    throw Exception::FileError("Error reading .cal file",groupingFileName);
  }

  udet2group.clear();
  std::string str;
  while(getline(grFile,str))
  {
	//Comment
    if (str.empty() || str[0] == '#') continue;
    std::istringstream istr(str);
    int n,udet,sel,group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    if ((sel) && (group>0))
    {
    	udet2group[udet]=group; //Register this udet
    	groups.insert(group);   //Register this group
    }
  }
  nGroups=groups.size(); // Number of unique groups
  grFile.close();

  return;
}
/// Determine the rebinning parameters, i.e Xmin, Xmax and logarithmic step for each group
///
void DiffractionFocussing2::determineRebinParameters()
{

	std::ostringstream mess;

	udet2groupmap::iterator udetit;
	group2minmaxmap::iterator gpit;
	// Register all groups
	std::set<int>::iterator it=groups.begin();
	for (;it!=groups.end();it++)
		group2minmax[(*it)]=std::make_pair<double,double>(1e14,-1e14);

	spectra_group.resize(nHist);
	API::Axis* spectra_Axis=inputW->getAxis(1);
	int group;

	for (int i=0;i<nHist;i++) //  Iterate over all histograms to find X boundaries for each group
	{
		group=validateSpectrumInGroup(spectra_Axis->spectraNo(i));
		spectra_group[i]=group;
		if (group==-1)
			continue;
		gpit=group2minmax.find(group);
		double min=((*gpit).second).first;
		double max=((*gpit).second).second;
		const std::vector<double>& X=inputW->readX(i);
		if (X.front()< (min)) //New Xmin found
			((*gpit).second).first=X.front();
		if (X.back()> (max))  //New Xmax found
			((*gpit).second).second=X.back();
	}

	double Xmin,Xmax,step;

	//Iterator over all groups to create the new X vectors
	for (gpit=group2minmax.begin();gpit!=group2minmax.end();gpit++)
	{
		Xmin=((*gpit).second).first;
		Xmax=((*gpit).second).second;
		if (Xmax<Xmin) // Should never happen
		{
			mess << "Fail to determine X boundaries for group" << (*gpit).first;
			throw std::runtime_error(mess.str());
		}
		step=(log(Xmax)-log(Xmin))/(nPoints-1);
		mess << "Found Group:" << ((*gpit).first)
					<< "(Xmin,Xmax,log step):" << ((*gpit).second).first
					<< "," << ((*gpit).second).second << "," <<	step;
		g_log.information(mess.str());
		mess.str("");
		std::vector<double> xnew(nPoints); //New X vector
		xnew[0]=Xmin;
		for (int j=1;j<nPoints;j++)
		{
			xnew[j]=Xmin*(1.0+step);
			Xmin=xnew[j];
		}
		group2xvector[(*gpit).first]=xnew; //Register this vector in the map
	}
		// Not needed anymore
		udet2group.clear();
		groups.clear();
		return;
}

///Verify that all the contributing detectors to a spectrum belongs to the same group
/// @param spectrum_number The spectrum number in the workspace
/// @return Group number if successful otherwise return -1
int DiffractionFocussing2::validateSpectrumInGroup(int spectrum_number)
{
	// Get the spectra to detector map
	const API::SpectraDetectorMap& spectramap=inputW->spectraMap();
	std::vector<int> dets=spectramap.getDetectors(spectrum_number);
	if (dets.empty()) // Not in group
		return -1;

	std::vector<int>::const_iterator it=dets.begin();
	udet2groupmap::const_iterator mapit=udet2group.find((*it)); //Find the first udet
	if (mapit==udet2group.end()) // The first udet that contributes to this spectra is not assigned to a group
		return -1;
	int group=(*mapit).second;
	int new_group;
	for (it+1;it!=dets.end();it++) // Loop other all other udets
	{
		mapit=udet2group.find((*it));
		if (mapit==udet2group.end())	// Group not assigned
			return -1;
		new_group=(*mapit).second;
		if (new_group!=group)         // At least one udet does not belong to the same group
			return -1;
	}
	return group;
}


} // namespace Algorithm
} // namespace Mantid
