#include "ModifyData.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace Algorithms
{

// Algorithm must be declared
DECLARE_ALGORITHM(ModifyData);

using namespace Kernel;
using namespace API;
using DataObjects::Workspace1D_sptr;
using DataObjects::Workspace1D;
using DataObjects::Workspace2D_sptr;
using DataObjects::Workspace2D;


// Get a reference to the logger. It is used to print out information,
// warning, and error messages
Logger& ModifyData::g_log = Logger::get("ModifyData");

/**  Initialization code
 *     
 *   Properties have to be declared here before they can be used
*/
void ModifyData::init()
{
    
    // Declare a 2D input workspace property. 
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input));

    // Declare a 2D output workspace property. 
    declareProperty(new WorkspaceProperty<Workspace2D>("OutputWorkspace","",Direction::Output));

    // Switches between two ways of accessing the data in the input workspace
    declareProperty("UseVectors",false);

}

/** Executes the algorithm
 */
void ModifyData::exec() 
{ 
    g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl; 

    // Get the input workspace
    Workspace2D_sptr inputW = getProperty("InputWorkspace");

    // make output Workspace the same type and size as the input one
    Workspace2D_sptr outputW = boost::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(inputW));
    
    // Create vectors to hold result
    std::vector<double> newX;
    std::vector<double> newY;
    std::vector<double> newE;

    bool useVectors = getProperty("UseVectors");

    if ( useVectors ) 
    {
        g_log.information() << "Option 1. Original values:" << std::endl; 
        // Get the count of histograms in the input workspace
        int histogramCount = inputW->getNumberHistograms();
        // Loop over spectra
        for (int i = 0; i < histogramCount; ++i) 
        {
        
            // Retrieve the data into a vector
            const std::vector<double>& XValues = inputW->dataX(i);
            const std::vector<double>& YValues = inputW->dataY(i);
            const std::vector<double>& EValues = outputW->dataE(i);

            newX.clear();
            newY.clear();
            newE.clear();
            
            // Iterate over i-th spectrum and modify the data
            for(int j=0;j<inputW->blocksize();j++)
            {
                g_log.information() << "Spectrum " << i << " Point " << j << " values: "
                    << XValues[j] << ' ' << YValues[j] << ' ' << EValues[j] << std::endl;
                newX.push_back(XValues[j] + i + j);
                newY.push_back(YValues[j]*(2. + 0.1*j));
                newE.push_back(EValues[j]+0.1);
            }

            // Populate the new workspace
            outputW->setX(i,newX);
            outputW->setData(i,newY, newE);
        }
    }
    else
    {
        g_log.information() << "Option 2. Original values:" << std::endl; 
        // Iterate over the workspace and modify the data
        int count = 0;
        for(Workspace2D::const_iterator ti(*inputW); ti != ti.end(); ++ti)
        {
            // get the spectrum number
            int i = count / inputW->blocksize();
            // get the point number
            int j = count % inputW->blocksize();
            // Get the reference to a data point
            LocatedDataRef tr = *ti;
            g_log.information() << "Spectrum " << i << " Point " << j << " values: "
                << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
            newX.push_back(tr.X() + count);
            newY.push_back(tr.Y()*2);
            newE.push_back(tr.E()+0.1);

            // Populate the new workspace
            if ( j == inputW->blocksize() - 1)
            {
                outputW->setX(i,newX);
                outputW->setData(i,newY, newE);
                newX.clear();
                newY.clear();
                newE.clear();
            }
            count++;
        }

   }

  
    // Assign it to the output workspace property
    setProperty("OutputWorkspace",outputW);
    
    // Get the newly set workspace
    Workspace2D_sptr newW = getProperty("OutputWorkspace");
   
    // Check the new workspace
    g_log.information() << "New values:" << std::endl; 
    int count = 0;
    for(Workspace2D::const_iterator ti(*newW); ti != ti.end(); ++ti)
    {
        // Get the reference to a data point
        LocatedDataRef tr = *ti;
        g_log.information() << "Point number " << count++ << " values: "
            << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
    }

}

}
}

