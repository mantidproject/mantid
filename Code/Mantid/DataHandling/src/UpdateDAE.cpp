#include "MantidDataHandling/UpdateDAE.h"
#include <MantidDataObjects/Workspace2D.h>
#include <MantidKernel/ArrayProperty.h>
#include <MantidAPI/FrameworkManager.h>
#include <Poco/Thread.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(UpdateDAE);

void UpdateDAE::init()
{
      declareProperty(new WorkspaceProperty<Workspace2D>("Workspace","",Direction::Input));
      
      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("UpdateRate",10, mustBePositive);
}

void UpdateDAE::exec()
{
    Workspace2D_sptr ws = getProperty("Workspace");

    Mantid::API::Algorithm *loader = static_cast<Mantid::API::Algorithm*>(Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadDAE"));
    // set propetries of LoadDAE from the input workspace's properties
    bool loadedFromDAE = false;
    const WorkspaceHistory& hist = ws->getHistory();
    const std::vector< AlgorithmHistory > &  alg_hist = hist.getAlgorithmHistories();
    for(std::vector< AlgorithmHistory >::const_iterator alg=alg_hist.begin();alg!=alg_hist.end();alg++)
        if (alg->name() == "LoadDAE")
        {
            loadedFromDAE = true;
            const std::vector< PropertyHistory >& prop_hist = alg->getProperties();
            for(std::vector< PropertyHistory >::const_iterator prop=prop_hist.begin();prop!=prop_hist.end();prop++)
            if (!prop->isDefault())
            {
                loader->setPropertyValue(prop->name(),prop->value());
            }

        }
    if (!loadedFromDAE)
    {
        g_log.error("Input Workspace has not been created with LoadDAE");
        throw std::runtime_error("Input Workspace has not been created with LoadDAE");
    }

    Poco::Thread *thread = Poco::Thread::current();
    if (thread == 0)
    {
        g_log.error("Cannot execute UpdateDAE in the main thread.");
        throw std::runtime_error("Cannot execute UpdateDAE in the main thread.");
    }

    int rate = getProperty("UpdateRate");
    rate *= 1000;// in milliseconds
     for(;;)
    {
        loader->execute();
        thread->sleep(rate);
        interruption_point();
		
    }
}

