#include "ModifyData.h"

namespace Mantid
{
namespace Algorithms
{

// Algorithm must be declared
DECLARE_ALGORITHM(ModifyData)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
*/
void ModifyData::init()
{

    // Declare a 2D input workspace property.
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));

    // Declare a 2D output workspace property.
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output));

    // Switches between two ways of accessing the data in the input workspace
    declareProperty("UseVectors",false);

}

/** Executes the algorithm
 */
void ModifyData::exec()
{
		// g_log is a reference to the logger. It is used to print out information,
		// warning, and error messages
    g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl;

    // Get the input workspace
    MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

    // make output Workspace the same type and size as the input one
    MatrixWorkspace_sptr outputW = WorkspaceFactory::Instance().create(inputW);

    bool useVectors = getProperty("UseVectors");

    if ( useVectors )
    {
        g_log.information() << "Option 1. Original values:" << std::endl;
        // Get the count of histograms in the input workspace
        size_t histogramCount = inputW->getNumberHistograms();
        // Loop over spectra
        for (size_t i = 0; i < histogramCount; ++i)
        {
            // Retrieve the data into a vector
            const MantidVec& XValues = inputW->readX(i);
            const MantidVec& YValues = inputW->readY(i);
            const MantidVec& EValues = inputW->readE(i);
            MantidVec& newX = outputW->dataX(i);
            MantidVec& newY = outputW->dataY(i);
            MantidVec& newE = outputW->dataE(i);

            // Iterate over i-th spectrum and modify the data
            for(size_t j=0; j<inputW->blocksize(); j++)
            {
                g_log.information() << "Spectrum " << i << " Point " << j << " values: "
                    << XValues[j] << ' ' << YValues[j] << ' ' << EValues[j] << std::endl;
                newX[j] = XValues[j] + i + j;
                newY[j] = YValues[j]*(2. + 0.1*j);
                newE[j] = EValues[j]+0.1;
            }
        }
    }
    else
    {
        g_log.information() << "Option 2. Original values:" << std::endl;
        // Iterate over the workspace and modify the data
        int count = 0;
        MatrixWorkspace::iterator ti_out(*outputW);
        for(MatrixWorkspace::const_iterator ti(*inputW); ti != ti.end(); ++ti,++ti_out)
        {
            // get the spectrum number
            size_t i = count / inputW->blocksize();
            // get the point number
            size_t j = count % inputW->blocksize();
            // Get the reference to a data point
            LocatedDataRef tr = *ti;
            LocatedDataRef tr_out = *ti_out;
            g_log.information() << "Spectrum " << i << " Point " << j << " values: "
                << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
            tr_out.X() = tr.X() + count;
            tr_out.Y() = tr.Y()*2;
            tr_out.E() = tr.E()+0.1;

            count++;
        }

   }


    // Assign it to the output workspace property
    setProperty("OutputWorkspace",outputW);

    // Get the newly set workspace
    MatrixWorkspace_const_sptr newW = getProperty("OutputWorkspace");

    // Check the new workspace
    g_log.information() << "New values:" << std::endl;
    int count = 0;
    for(MatrixWorkspace::const_iterator ti(*newW); ti != ti.end(); ++ti)
    {
        // Get the reference to a data point
        LocatedDataRef tr = *ti;
        g_log.information() << "Point number " << count++ << " values: "
            << tr.X() << ' ' << tr.Y() << ' ' << tr.E() << std::endl;
    }

}

}
}

