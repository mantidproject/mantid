//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveGSS.h"
#include "MantidKernel/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveGSS)

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveGSS::init()
{
  // Data must be in TOF
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,
    new API::WorkspaceUnitValidator<>("TOF")),
    "The input workspace, which must be in time-of-flight");
  declareProperty(new Kernel::FileProperty("Filename", "", Kernel::FileProperty::Save),
    "The filename to use for the saved data");
  std::vector<std::string> Split(2);
  Split[0] = "True";
  Split[1] = "False";
  declareProperty("SplitFiles", "True", new Kernel::ListValidator(Split),
    "Save each spectrum in a different file (default true)" );
  declareProperty("Append",true,"If true and Filename already exists, append, else overwrite");
  declareProperty("Bank",1);

}

/**
 * Execute the algorithm
 */
void SaveGSS::exec()
{
  using namespace Mantid::API;
  //Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int nHist=inputWS->getNumberHistograms();

  std::string filename = getProperty("Filename");
  std::string inputWSName = getProperty("InputWorkspace");
 
  std::size_t pos=filename.find_first_of(".");
  std::string ext;
  if (pos!=std::string::npos) //Remove the extension
  {
    ext=filename.substr(pos+1,filename.npos);
    filename=filename.substr(0,pos);
  }

  int bank=getProperty("Bank");
  std::string split=getProperty("SplitFiles");

  std::ostringstream number;
  std::ofstream out;
  // Check whether to append to an already existing file or overwrite
  const bool append = getProperty("Append");
  using std::ios_base;
  ios_base::openmode mode = ( append ? (ios_base::out | ios_base::app) : ios_base::out );
    
  std::string period("1");
  int periodNum=1;
 // std::string::size_type index = inputWSName.find_last_of("_");
 // if (index != std::string::npos)
 // {
	//std::string::size_type len= inputWSName.length();
	//std::string ref=inputWSName.substr(index + 1,len-index);
	//std::string grpHeaderName=inputWSName.substr(0,index);
	//if(AnalysisDataService::Instance().doesExist(grpHeaderName))
	//{
	//	//g_log.error()<<"group header name is "<<grpHeaderName<<std::endl;
	//	Workspace_sptr wsSptr=AnalysisDataService::Instance().retrieve(grpHeaderName);
	//	WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
	//	if(wsGrpSptr)
	//	{
	//	 periodNum=wsGrpSptr->getPeriodNumber(inputWSName);
	//	}
	//}
 //   period = ref;
 // }
  periodNum=getPeriodNumber(inputWSName);
  Progress p(this,0.0,1.0,nHist);
  for (int i=0;i<nHist;i++)
  {
    const MantidVec& X=inputWS->readX(i);
    const MantidVec& Y=inputWS->readY(i);
    const MantidVec& E=inputWS->readE(i);

    if (split=="False" && i==0) // Assign only one file
    {
      const std::string file(filename+'.'+ext);
	  Poco::File fileobj(file);
      const bool exists = fileobj.exists();
	  //if (!period.compare("1"))
	  if(periodNum==1)
	  {	  if (!append)
		  {	 //for period =1 if append is false delete the file
			 if(exists)fileobj.remove();
		  }
	  }
	  //else if (period.compare("1"))
	  else if(periodNum>1)
	  {  //if the period is not equal to one set the append mode
		  mode=ios_base::app;
	  }
      out.open(file.c_str(),mode);
      if ( !exists || !append )  writeHeaders(out,inputWS);

    }
    else if (split=="True")//Several files will be created with names: filename-i.ext
    {
      number << "-" << i;
      const std::string file(filename+number.str()+"."+ext);
	  Poco::File fileobj(file);
      const bool exists = fileobj.exists();
	 // if (!period.compare("1"))
	  if(periodNum==1)//for period =1 if append is false delete the file
	  {	  if (!append)
		  {	 if(exists)fileobj.remove();
		   }
	  }
	  else if (periodNum>1)
	  {  //if the period is not equal to one set the append mode
		  mode=ios_base::app;
	  }
      out.open(file.c_str(),mode);
      number.str("");
      if ( !exists || !append ) writeHeaders(out,inputWS);
    }

    { // New scope
      if (!out.is_open())
      {
        g_log.information("Could not open filename: "+filename);
        throw std::runtime_error("Could not open filename: "+filename);
      }
      const int datasize = Y.size();
      double bc1=X[0]*32;
      double bc2=(X[1]-X[0])*32;
      // Logarithmic step
      double bc4=(X[1]-X[0])/X[0];
      out << "# Data for spectrum :"<< i << std::endl;
      out << "BANK "
        << std::fixed << std::setprecision(0) << bank // First bank should be 1 for GSAS
        << std::fixed << " " << datasize
        << std::fixed << " " << datasize
        << std::fixed << " " << "RALF"
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc2
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
        << std::fixed << " " << std::setprecision(5) << std::setw(7) << bc4
        << " FXYE"<<std::endl;
      for (int j = 0; j < datasize; j++)
      {
        out << std::fixed << std::setprecision(5) << std::setw(15) << 0.5*(X[j]+X[j+1])
          << std::fixed << std::setprecision(8) << std::setw(18) << Y[j]*X[j]*bc4
          << std::fixed << std::setprecision(8) << std::setw(18) << E[j]*X[j]*bc4 << "\n";
      }
    } // End separate scope

    //Close at each iteration
    if (split=="True")
    {
      out.close();
    }
    p.report();
  }
  // Close if single file
  if (split=="False")
  {
    out.close();
  }
  return;
}

/**
 * Write the header information for the given workspace
 * @param os The stream to use to write the information
 * @param workspace A shared pointer to MatrixWorkspace
 */
void SaveGSS::writeHeaders(std::ostream& os, Mantid::API::MatrixWorkspace_const_sptr& workspace) const
{
  os << workspace->getTitle() << std::endl;
  os <<"# File generated by Mantid:\n";
  os <<"# Instrument: " << workspace->getBaseInstrument()->getName() << std::endl;

  return;
}


