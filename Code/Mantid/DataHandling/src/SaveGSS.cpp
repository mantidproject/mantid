//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "Poco/File.h"
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
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Save),
    "The filename to use for the saved data");
  std::vector<std::string> Split(2);
  Split[0] = "True";
  Split[1] = "False";
  declareProperty("SplitFiles", "True", new Kernel::ListValidator(Split),
    "Save each spectrum in a different file (default true)" );
  declareProperty("Append",true,"If true and Filename already exists, append, else overwrite");
  declareProperty("Bank",1, "Start bank (spectrum) numbers at this number in the file. The bank number in the file will be the workspace index + StartAtBankNumber. Default = 1.");

  declareProperty("MultiplyByBinWidth", true, "Multiply the intensity (Y) by the bin width; default TRUE.");
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

  const int bank=getProperty("Bank");
  const bool MultiplyByBinWidth = getProperty("MultiplyByBinWidth");
  std::string split_string = getProperty("SplitFiles");
  bool split = (split_string == "True");

  std::ostringstream number;
  std::ofstream out;
  // Check whether to append to an already existing file or overwrite
  const bool append = getProperty("Append");
  using std::ios_base;
  ios_base::openmode mode = ( append ? (ios_base::out | ios_base::app) : ios_base::out );
  Progress p(this,0.0,1.0,nHist);
  for (int i=0;i<nHist;i++)
  {
    const MantidVec& X=inputWS->readX(i);
    const MantidVec& Y=inputWS->readY(i);
    const MantidVec& E=inputWS->readE(i);

    if (!split && i==0) // Assign only one file
    {
      const std::string file(filename+'.'+ext);
      Poco::File fileobj(file);
      const bool exists = fileobj.exists();
      out.open(file.c_str(),mode);
      if ( !exists || !append )  writeHeaders(out,inputWS);

    }
    else if (split)//Several files will be created with names: filename-i.ext
    {
      number << "-" << i;
      const std::string file(filename+number.str()+"."+ext);
      Poco::File fileobj(file);
      const bool exists = fileobj.exists();
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
      if(boost::math::isnan(fabs(bc4)) || boost::math::isinf(bc4)) bc4=0; //If X is zero for BANK

      //Write out the data header
      out << "# Data for spectrum :"<< i << std::endl;
      out << "BANK "
        << std::fixed << std::setprecision(0) << bank+i // First bank should be 1 for GSAS; this can be changeed
        << std::fixed << " " << datasize
        << std::fixed << " " << datasize
        << std::fixed << " " << "RALF"
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc2
        << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
        << std::fixed << " " << std::setprecision(5) << std::setw(7) << bc4
        << " FXYE"<<std::endl;

      //Do each Y entry
      for (int j = 0; j < datasize; j++)
      {
        //Calculate the error
        double Epos;
        if (MultiplyByBinWidth)
          Epos = E[j] * (X[j+1] - X[j]); // E[j]*X[j]*bc4;
        else
          Epos = E[j];
        if(Epos<0 || boost::math::isnan(Epos) || boost::math::isinf(Epos)) Epos=0; //Negative errors cannot be read by GSAS

        //The center of the X bin.
        out << std::fixed << std::setprecision(5) << std::setw(15) << 0.5*(X[j]+X[j+1]);

        // The Y value
        if (MultiplyByBinWidth)
          out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j] * (X[j+1] - X[j]);
        else
          out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j];

        //The error
        out << std::fixed << std::setprecision(8) << std::setw(18) << Epos << "\n";
      }
    } // End separate scope

    //Close at each iteration
    if (split)
    {
      out.close();
    }
    p.report();
  }
  // Close if single file
  if (!split)
  {
    out.close();
  }
  return;
}

/** Ensures that when a workspace group is passed as output to this workspace
 *  everything is saved to one file and the bank number increments for each
 *  group member.
 *  @param alg           Pointer to the algorithm
 *  @param propertyName  Name of the property
 *  @param propertyValue Value  of the property
 *  @param periodNum     Effectively a counter through the group members
 */
void SaveGSS::setOtherProperties(IAlgorithm* alg,const std::string& propertyName,const std::string& propertyValue,int periodNum)
{
  // We want to append subsequent group members to the first one
  if( propertyName == "Append")
  {
    if(periodNum!=1)
    {
      alg->setPropertyValue(propertyName,"1");
    }
    else alg->setPropertyValue(propertyName,propertyValue);
  }
  // We want the bank number to increment for each member of the group
  else if ( propertyName == "Bank" )
  {
    alg->setProperty("Bank",atoi(propertyValue.c_str())+periodNum-1);
  }
  else Algorithm::setOtherProperties(alg,propertyName,propertyValue,periodNum);
}

/**
 * Write the header information for the given workspace
 * @param os The stream to use to write the information
 * @param workspace A shared pointer to MatrixWorkspace
 */
void SaveGSS::writeHeaders(std::ostream& os, Mantid::API::MatrixWorkspace_const_sptr& workspace) const
{
  os << workspace->getTitle() << std::endl;
  os <<"# " << workspace->getNumberHistograms() << " Histograms\n";
  os <<"# File generated by Mantid:\n";
  os <<"# Instrument: " << workspace->getBaseInstrument()->getName() << std::endl;
  os <<"# From workspace named : " << workspace->getName();
  if (getProperty("MultiplyByBinWidth"))
    os << ", with Y multiplied by the bin widths.";
  os << std::endl;

  return;
}
