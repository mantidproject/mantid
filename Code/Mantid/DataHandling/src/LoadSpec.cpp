//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpec.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>
#include <cstring>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadSpec)

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadSpec::LoadSpec() : Algorithm() {}

    /// Initialisation method.
    void LoadSpec::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "A Spec Ascii file");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created.");

      std::vector<std::string> units = UnitFactory::Instance().getKeys();
      units.insert(units.begin(),"MomemtumTransfer");
      declareProperty("Unit","Energy",new Kernel::ListValidator(units),
        "The unit to assign to the X axis (default: Energy)");
    }

    /** 
    *   Executes the algorithm.
    */
    void LoadSpec::exec()
    {
      std::string filename = getProperty("Filename");
      std::string separator = " "; //separator can be 1 or more spaces
      std::ifstream file(filename.c_str());

      file.seekg (0, std::ios::end);
      Progress progress(this,0,1,file.tellg());
      file.seekg (0, std::ios::beg);

      std::string str;

      std::vector<DataObjects::Histogram1D> spectra;
      size_t iLine=0;    // line number
      size_t ncols = 3;  // number of columns
      size_t nSpectra = 0; //only 3 columns, so only 1 here
      size_t nBins = 0; //number of rows
      std::string first_character;
      std::string axes_infos;

      spectra.resize(ncols);

      bool numeric = true;
      std::vector<double> input;

      while(getline(file,str))
	{   
	  progress.report(file.tellg());
	  first_character = str[0];
	  
	  //line with data, need to be parsed by white spaces
	  if (!str.empty() && str[0] != '#') {
	    
	    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	    boost::char_separator<char> sep(" ");
	    tokenizer tok(str, sep);
	    ncols = 0;
	    for (tokenizer::iterator beg=tok.begin(); beg!=tok.end(); ++beg) {
	      
	      std::stringstream ss;
	      ss << *beg;
	      double d;
	      ss >> d;
	      //std::cout << d << std::endl;
	      input.push_back(d);
	    }
	  } 
	  /*
	  else {
	    //if the second character is L, we need to keep that line that gives
	    //infos about axes labels and units
	    if (!str.empty() && str[1] == 'L') { 
	      axes_infos = str;}
	  }
	  */
	  
	} //end of read file
      
      for(int j=0; j<(input.size()-1); j++) {
	spectra[0].dataX().push_back(input[j]);
	j++;
	spectra[0].dataY().push_back(input[j]);
	j++;
	spectra[0].dataE().push_back(input[j]);
	nBins = j/3;
      }
      
      nSpectra = 1;
      MatrixWorkspace_sptr localWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>
        (WorkspaceFactory::Instance().create("Workspace2D",nSpectra,nBins,nBins));
      try {
	localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(getProperty("Unit"));
      } catch (Exception::NotFoundError) {
	// Asked for dimensionless workspace (obviously not in unit factory)
      }
      
      for(unsigned int i=0;i<nSpectra;i++)
	{
	  localWorkspace->dataX(i) = spectra[i].dataX();
	  localWorkspace->dataY(i) = spectra[i].dataY();
	  localWorkspace->dataE(i) = spectra[i].dataE();
	  // Just have spectrum number start at 1 and count up
	  localWorkspace->getAxis(1)->spectraNo(i) = i+1;
	}
      
      setProperty("OutputWorkspace",localWorkspace);
      
    }
    
  } // namespace DataHandling
} // namespace Mantid
