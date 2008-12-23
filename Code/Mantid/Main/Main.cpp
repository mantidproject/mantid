#include <iostream>
#include <iomanip>
#include "Benchmark.h"
#include "UserAlgorithmTest.h"
#include "MantidAPI/FrameworkManager.h"
//#include "MantidAPI/Workspace.h"
//#include "MantidDataObjects/Workspace1D.h" 
//#include "MantidDataObjects/Workspace2D.h" 

#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/TableRow.h"
#include "MantidDataObjects/ColumnFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/ParametrizedComponent.h"
#include "MantidGeometry/ParObjComponent.h"
#include "MantidGeometry/ParameterMap.h"

#include <boost/timer.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

void test();

int main()
{

  FrameworkManagerImpl& fm = FrameworkManager::Instance();

//  UserAlgorithmTest userTest;
 // userTest.RunAllTests();
  
 // Benchmark b;
 // b.RunPlusTest();
    
#if defined _DEBUG
  //NOTE:  Any code in here is temporary for debugging purposes only, nothing is safe!
  //load a raw file
    IAlgorithm* loader = fm.createAlgorithm("LoadRaw");
    loader->setPropertyValue("Filename", "../../../Test/Data/HET15869.RAW");

    std::string outputSpace = "outer";
    loader->setPropertyValue("OutputWorkspace", outputSpace);    

    loader->execute();

    IAlgorithm* focus = fm.createAlgorithm("DiffractionFocussing");
    focus->setPropertyValue("GroupingFileName", "../../../Test/Data/offsets_2006_cycle064.cal");

    std::string resultSpace = "result";
    focus->setPropertyValue("InputWorkspace", outputSpace); 
    focus->setPropertyValue("OutputWorkspace", resultSpace);    

    focus->execute();


#endif


  FrameworkManager::Instance().clear();
  exit(0);//*/


   // test();

}

std::vector< boost::shared_ptr<ParObjComponent> > getP(const std::vector< boost::shared_ptr<IObjComponent> >& plts,ParameterMap& map);

void test()
{
    //create a workspace with some sample data
    std::string wsName = "LoadInstrumentTestHET";
    int histogramNumber = 2584;
    int timechannels = 100;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    //loop to create data
    for (int i = 0; i < histogramNumber; i++)
    {
      std::vector<double> timeChannelsVec(timechannels);
      std::vector<double> v(timechannels);
      // Create and fill another vector for the errors
      std::vector<double> e(timechannels);
      //timechannels
      for (int j = 0; j < timechannels; j++)
      {
        timeChannelsVec[j] = j*100;
        v[j] = (i+j)%256;
        e[j] = (i+j)%78;
      }
      // Populate the workspace.
      ws2D->setX(i, timeChannelsVec);
      ws2D->setData(i, v, e);
    }

    //put this workspace in the data service
    AnalysisDataService::Instance().add(wsName, ws2D);

    try
    {
        LoadInstrument loader;
        loader.initialize();
        std::string inputFile = "../../../Test/Instrument/MER_Definition.xml";
        loader.setPropertyValue("Filename", inputFile);
        loader.setPropertyValue("Workspace", wsName);

        loader.execute();

        Workspace_sptr output;
        output = AnalysisDataService::Instance().retrieve(wsName);
        boost::shared_ptr<IInstrument> i = output->getInstrument();
        std::cerr<<"Name="<<i->getName()<<'\n';

        boost::timer tim;
        std::vector< boost::shared_ptr<IObjComponent> > plts = i->getPlottable();
        for(size_t i=0;i<plts.size();i++)
        {
            V3D pos = plts[i]->getPos();
            Quat rot = plts[i]->getRotation();
        }
        std::cerr<<"Number of plottables: "<<plts.size()<<' '<<tim.elapsed()<<'\n';


        int jj = 0;
        boost::shared_ptr<ParameterMap> pmap = output->InstrumentParameters();
        for(std::vector< boost::shared_ptr<IObjComponent> >::const_iterator o = plts.begin();o!=plts.end();o++)
            if ((o-plts.begin()) % 10000 == 0)
            {
                pmap->addV3D(&**o,"pos",V3D(jj++,20,30));
                pmap->addQuat(&**o,"rot",Quat(2,0,0,0));
            }
            std::cerr<<"map size: "<<pmap->size()<<'\n';
        boost::timer tim1;
        boost::shared_ptr<IInstrument> ii = output->getInstrument();
        std::vector< boost::shared_ptr<IObjComponent> > pplts = ii->getPlottable();
        for(size_t i=0;i<pplts.size();i++)
        {
            IObjComponent* pc = pplts[i].get();
            V3D pos = pc->getPos();
            Quat rot = pc->getRotation();
            /*if (!(pos == plts[i]->getPos()))
            {
                std::cerr<<++ii<<'\n';
                std::cerr<<rot<<plts[i]->getRotation()<<'\n';
                std::cerr<<pos<<plts[i]->getPos()<<'\n';
            }//*/
        }
        std::cerr<<"Number of plottables: "<<pplts.size()<<' '<<tim1.elapsed()<<'\n';
    }
    catch(std::exception& e)
    {
        std::cerr<<'\n'<<e.what()<<"\n\n";
    }

}

std::vector< boost::shared_ptr<ParObjComponent> > getP(const std::vector< boost::shared_ptr<IObjComponent> >& plts,ParameterMap& map)
{
    std::vector< boost::shared_ptr<ParObjComponent> > res;
    for(std::vector< boost::shared_ptr<IObjComponent> >::const_iterator o = plts.begin();o!=plts.end();o++)
    {
        res.push_back(boost::shared_ptr<ParObjComponent>(new ParObjComponent(dynamic_cast<ObjComponent*>(&**o),&map)));
    }
    return res;
}