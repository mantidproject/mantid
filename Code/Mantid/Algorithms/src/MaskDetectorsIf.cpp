//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskDetectorsIf.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <fstream>
#include <iomanip>


namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MaskDetectorsIf)

using namespace Kernel;
using DataObjects::Workspace1D;

/// Constructor
MaskDetectorsIf::MaskDetectorsIf() :
  API::Algorithm()
{}

/// Destructor
MaskDetectorsIf::~MaskDetectorsIf()
{}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MaskDetectorsIf::init()
{
	declareProperty(new API::WorkspaceProperty<>("InputWorkspace","",Direction::Input),
			 "A 1D Workspace that contains values to select against " );
	std::vector<std::string> select_mode(2);
	  select_mode[0] = "SelectIf";
	  select_mode[1] = "DeselectIf";
	  declareProperty("Mode", "SelectIf", new Kernel::ListValidator(select_mode),
	    "Mode to select or deselect detectors based on comparison with values" );
	std::vector<std::string> select_operator(6);
	  select_operator[0] = "Equal";
	  select_operator[1] = "NotEqual";
	  select_operator[2] = "Greater";
	  select_operator[3] = "GreaterEqual";
	  select_operator[4] = "Less";
	  select_operator[5] = "LessEqual";
	  declareProperty("Operator", "Equal", new Kernel::ListValidator(select_operator),
	  	    "Unary operator to compare to given values" );
	  declareProperty("Value",0.0);
	  declareProperty(new API::FileProperty("InputCalFile","", API::FileProperty::Load, ".cal"),
			  "The name of the CalFile with grouping data" );
	  declareProperty(new API::FileProperty("OutputCalFile","", API::FileProperty::Save, ".cal"),
			  "The name of the CalFile with grouping data" );
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw std::runtime_error If the rebinning process fails
 */
void MaskDetectorsIf::exec()
{
	retrieveProperties();
	const int nspec=inputW->getNumberHistograms();
	const API::SpectraDetectorMap& spectramap = inputW->spectraMap();

	for (int i=0;i<nspec;++i)
	{
		// Get the spectrum number
		const int spec = inputW->getAxis(1)->spectraNo(i);
		// Get the list of udets contributing to this spectra
		std::vector<int> dets = spectramap.getDetectors(spec);
		if (dets.empty())
			continue;
		else
		{
			double val=inputW->readY(i)[0];
			if (compar_f(val,value))
			{
			for (unsigned int j=0;j<dets.size();++j)
				umap.insert(std::make_pair<int,bool>(dets[j],select_on));
			}
		}
		double p=static_cast<double>(i)/nspec;
		progress(p,"Generating detector map");
	}
	std::string oldf=getProperty("InputCalFile");
	std::string newf=getProperty("OutputCalFile");
	progress(0.99,"Creating new cal file");
	createNewCalFile(oldf,newf);
	return;
}

/**
 * Get the input properties and store them in the object variables
 */
void MaskDetectorsIf::retrieveProperties()
{
	inputW=getProperty("InputWorkspace");

	//
	value=getProperty("Value");

	// Get the selction mode (select if or deselect if)
	std::string select_mode=getProperty("Mode");
	if (select_mode=="SelectIf")
		select_on=true;
	else
		select_on=false;

	// Select function object based on the type of comparison operator
	std::string select_operator=getProperty("Operator");

	if (select_operator=="LessEqual")
		 compar_f=std::less_equal<double>();
	 else if (select_operator=="Less")
		 compar_f=std::less<double>();
	 else if (select_operator=="GreaterEqual")
		 compar_f=std::greater_equal<double>();
	 else if (select_operator=="Greater")
		 compar_f=std::greater<double>();
	 else if (select_operator=="Equal")
		 compar_f=std::equal_to<double>();
	else if (select_operator=="NotEqual")
		 compar_f=std::not_equal_to<double>();

		 return;
}

/**
 * Create a new cal file based on the old file
 * @param oldfile The old cal file path
 * @param newfile The new cal file path
 */
void MaskDetectorsIf::createNewCalFile(const std::string& oldfile, const std::string& newfile)
{
  std::ifstream oldf(oldfile.c_str());
  if (!oldf.is_open())
  {
    g_log.error() << "Unable to open grouping file " << oldfile << std::endl;
    throw Exception::FileError("Error reading .cal file",oldfile);
  }
  std::ofstream newf(newfile.c_str());
  if (!newf.is_open())
  {
    g_log.error() << "Unable to open grouping file " << newfile << std::endl;
    throw Exception::FileError("Error reading .cal file",newfile);
  }
  std::string str;
  while(getline(oldf,str))
  {
    //Comment or empty lines get copied into the new cal file
    if (str.empty() || str[0] == '#')
    {
      newf << str << std::endl;
      continue;
    }
    std::istringstream istr(str);
    int n,udet,sel,group;
    double offset;
    istr >> n >> udet >> offset >> sel >> group;
    udet2valuem::iterator it=umap.find(udet);
    bool selection;

    if (it==umap.end())
      selection = (sel==0) ? false : true;
    else
      selection = (*it).second;

    newf << std::fixed << std::setw(9) << n <<
      std::fixed << std::setw(15) << udet <<
      std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
      std::fixed << std::setw(8) << selection <<
      std::fixed << std::setw(8) << group  << std::endl;
  }
  oldf.close();
  newf.close();
  return;
}

} // namespace Algorithm
} // namespace Mantid
