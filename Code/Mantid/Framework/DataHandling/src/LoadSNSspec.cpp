//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSNSspec.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include <fstream>
#include <cstring>
#include <boost/tokenizer.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadSNSspec)
      //register the algorithm into loadalgorithm factory
    DECLARE_LOADALGORITHM(LoadSNSspec)
    
    /// Sets documentation strings for this algorithm
    void LoadSNSspec::initDocs()
    {
      this->setWikiSummary("Loads data from a text file and stores it in a 2D [[workspace]] ([[Workspace2D]] class). ");
      this->setOptionalMessage("Loads data from a text file and stores it in a 2D workspace (Workspace2D class).");
    }
    

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadSNSspec::LoadSNSspec()  {}

    /// Initialisation method.
    void LoadSNSspec::init()
    {
      std::vector<std::string> exts;
	  exts.push_back(".dat");
	  exts.push_back(".txt");

	  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
			"A SNS Spec Ascii file");
	  declareProperty(new WorkspaceProperty<>("OutputWorkspace",
			"",Direction::Output), "The name of the workspace that will be created.");

	  std::vector<std::string> units = UnitFactory::Instance().getKeys();
	  declareProperty("Unit","Energy",new Kernel::ListValidator(units),
			"The unit to assign to the X axis (default: Energy)");
    }

    /**
    *   Executes the algorithm.
    */
    void LoadSNSspec::exec()
    {
	  std::string filename = getProperty("Filename");
	  std::string separator = " "; //separator can be 1 or more spaces
	  std::ifstream file(filename.c_str());

	  file.seekg (0, std::ios::end);
	  Progress progress(this,0,1,file.tellg());
	  file.seekg (0, std::ios::beg);

	  std::string str;

	  std::vector<DataObjects::Histogram1D> spectra;
	  //size_t iLine=0;    // line number
	  size_t ncols = 3;  // number of columns
	  size_t nSpectra = 0;
	  size_t nBins = 0; //number of rows
	  std::string first_character;
	  std::string axes_infos;

	  //bool numeric = true;
	  std::vector<double> input;

	  //determine the number of lines starting by #L
	  //as there is one per set of data
	  int spectra_nbr = 0;
	  while(getline(file,str))
	  {
     if (str.empty() ) continue;
		 if (str[0] == '#' && str[1] == 'L')
		 {
		   spectra_nbr++;
		 }
	  }

	  spectra.resize(spectra_nbr);
	  file.clear(); //end of file has been reached so we need to clear file state
	  file.seekg (0, std::ios::beg); //go back to beginning of file

	  int working_with_spectrum_nbr = -1; //spectrum number
	  while(getline(file,str))
	  {
	    progress.report(file.tellg());

	    //line with data, need to be parsed by white spaces
	    if (!str.empty() && str[0] != '#')
	    {
	      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		  boost::char_separator<char> sep(" ");
		  tokenizer tok(str, sep);
		  ncols = 0;
		  for (tokenizer::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
		  {
		    std::stringstream ss;
		    ss << *beg;
		    double d;
		    ss >> d;
  		    input.push_back(d);
		  }
	    }

	    if (str.empty())
	    {
		  if (working_with_spectrum_nbr != -1)
		  {
		    for(int j=0; j<static_cast<int>(input.size()-1); j++)
		    {
		      spectra[working_with_spectrum_nbr].dataX().push_back(input[j]);
			  j++;
			  spectra[working_with_spectrum_nbr].dataY().push_back(input[j]);
			  j++;
			  spectra[working_with_spectrum_nbr].dataE().push_back(input[j]);
			  nBins = j/3;
		    }
		  }
		  working_with_spectrum_nbr++;
		  input.clear();
	    }

	  } //end of read file

	  try
	  {
	    if (spectra_nbr == 0)
	      throw "Undefined number of spectra";

		if (working_with_spectrum_nbr == (spectra_nbr-1))
		{
		  for(int j=0; j<static_cast<int>(input.size()-1); j++)
		  {
		    spectra[working_with_spectrum_nbr].dataX().push_back(input[j]);
			j++;
			spectra[working_with_spectrum_nbr].dataY().push_back(input[j]);
			j++;
			spectra[working_with_spectrum_nbr].dataE().push_back(input[j]);
			nBins = j/3;
		  }
	    }
	  }
	  catch (...)
	  {
   	  }

	  try
	  {
	    nSpectra = spectra_nbr;
	    MatrixWorkspace_sptr localWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>
	    (WorkspaceFactory::Instance().create("Workspace2D",nSpectra,nBins,nBins));

	    localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(getProperty("Unit"));

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
	  catch (Exception::NotFoundError &)
	  {
	    // Asked for dimensionless workspace (obviously not in unit factory)
	  }

    }
    /**This method does a quick file check by checking the no.of bytes read nread params and header buffer
 *  @param filePath- path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
    bool LoadSNSspec::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
       std::string extn=extension(filePath);
      bool bascii(false);
      (!extn.compare("txt"))?bascii=true:bascii=false;

      bool is_ascii (true);
      for(size_t i=0; i<nread; i++)
      {
        if (!isascii(header.full_hdr[i]))
          is_ascii =false;
      }
      return(is_ascii|| bascii?true:false);
    }

/**checks the file by opening it and reading few lines 
 *  @param filePath :: name of the file including its path
 *  @return an integer value how much this algorithm can load the file 
 */
    int LoadSNSspec::fileCheck(const std::string& filePath)
    {  
      std::ifstream file(filePath.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filePath);
        throw Exception::FileError("Unable to open file: " , filePath);
      }

      int bret=0;
      int axiscols=0;
      int datacols=0;
      std::string str;
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer; 
      boost::char_separator<char> sep(" ");   
      bool bsnsspec(false);
      while(!file.eof())
      {
        //read line
        std::getline(file,str);
        if(str.empty())
        {
          continue;
        }
        try
        {
          //if it's comment line
          if (str.at(0)=='#' ) 
          {  
            if(str.at(1) =='L')  
            {
              tokenizer tok(str, sep); 
              for (tokenizer::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
              {		 
                ++axiscols;
              }
              //if the file contains a comment line starting with "#L" followed
              //by three columns this could be loadsnsspec file
              if(axiscols>2)
              {                
                bsnsspec=true;
              }
            }
          }
          else 
          {  
            //check first data line is a 3 column line
            tokenizer tok(str, sep); 
            for (tokenizer::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
            {		 
              ++datacols;
            } 
            break;
          }
        }
        catch(std::out_of_range& )
        {
        } 
      }
      if(bsnsspec && datacols==3 )//three column data
      {
        bret=80;
      }
      return bret;
    }
  } // namespace DataHandling
} // namespace Mantid
