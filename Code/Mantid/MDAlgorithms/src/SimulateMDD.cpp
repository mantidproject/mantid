//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/SimulateMDD.h"
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
    namespace MDAlgorithms
    {
        // Register the class into the algorithm factory
        DECLARE_ALGORITHM(SimulateMDD)
        void SimulateMDD::init()
        {
            //declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "Name of the input Workspace");
            // for testing just use a dummy string for the input workspace name
            declareProperty("InputMDWorkspace","",Direction::Input );
            // declare functions - for now only use background model
            // may have different backgrounds for different cuts, which will require an extension
            // 
            declareProperty("ForegroundModel","",Direction::Input );
            declareProperty("ForegroundModel_p1",0.0, "First param of bg model", Direction::Input);
            declareProperty("ForegroundModel_p2",0.0, "Second param of bg model", Direction::Input);
            declareProperty("ForegroundModel_p3",0.0, "Third param of bg model", Direction::Input);
            declareProperty("BackgroundModel","",Direction::Input );
            declareProperty("BackgroundModel_p1",0.0, "First param of bg model", Direction::Input);
            declareProperty("BackgroundModel_p2",0.0, "Second param of bg model", Direction::Input);
            declareProperty("BackgroundModel_p3",0.0, "Third param of bg model", Direction::Input);
            // In future, should be a new MDData workspace with the simulated results
            declareProperty("OutputMDWorkspace","",Direction::Output);
        }

        void SimulateMDD::exec()
        {
            std::string dummyMDwrkspc = getProperty("InputMDWorkspace");
            std::string bgmodel = getProperty("BackgroundModel");
            const double bgpara_p1 = getProperty("BackgroundModel_p1");
            const double bgpara_p2 = getProperty("BackgroundModel_p2");
            const double bgpara_p3 = getProperty("BackgroundModel_p3");
            std::string fgmodel = getProperty("ForegroundModel");
            const double fgpara_p1 = getProperty("ForegroundModel_p1");
            const double fgpara_p2 = getProperty("ForegroundModel_p2");
            const double fgpara_p3 = getProperty("ForegroundModel_p3");
            std::string dummyMDoutput = getProperty("OutputMDWorkspace");

            myCut = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(AnalysisDataService::Instance().retrieve(dummyMDwrkspc));
            //g_log.warning("No FakeCut data yet available in SimulateMDD");
            SimBackground(bgmodel,bgpara_p1,bgpara_p2,bgpara_p3);

        }
        void SimulateMDD::SimBackground(std::string bgmodel,const double bgpara_p1, const double bgpara_p2, const double bgpara_p3)
        {
            // Assume that the energy (centre point) is in coordinate.t of first point vertex
            if( ! bgmodel.compare("QuadEnTrans")) {
                int ncell= myCut->getXDimension()->getNBins();
                for(int i=0; i<ncell ; i++ ){
                    double bgsum=0.;
                    boost::shared_ptr<const Mantid::Geometry::MDCell> newCell = myCut->getCell(i);
                    std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > myPoints = newCell->getContributingPoints();
                    for(int j=0; j<myPoints.size(); j++){
                        std::vector<Mantid::Geometry::coordinate> vertexes = myPoints[j]->getVertexes();
                        double eps=vertexes.at(0).t;
                        bgsum+=bgpara_p1+eps*(bgpara_p2+eps*bgpara_p3);
                    }
                    //pnt->setSignal(bgsum);
                }
            }
            else if( ! bgmodel.compare("ExpEnTrans")) {
            }
            else if( ! bgmodel.compare("QuadEnTransAndPhi")) {
            }
            else if( ! bgmodel.compare("ExpEnTransAndPhi")) {
            }
            else {
                //g_log.error("undefined background model: "+bgmodel);
            }
        }
        /*
        FakeCut *SimulateMDD::generateFakeCut()
        {
            fpvec.push_back(* new FakePixel( 0.1,  0.,  0.,  0.5, 0.1, 45.0 , 45.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec.push_back(* new FakePixel( 0.2,  0.,  0.,  0.5, 0.1, 46.0 , 45.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec.push_back(* new FakePixel( 0.3,  0.,  0.,  0.5, 0.1, 47.0 , 45.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec2.push_back(* new FakePixel( 0.1,  1.,  0.,  0.5, 0.1, 45.0 , 35.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec2.push_back(* new FakePixel( 0.2,  1.,  0.,  0.5, 0.1, 46.0 , 35.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec2.push_back(* new FakePixel( 0.3,  1.,  0.,  0.5, 0.1, 47.0 , 35.0 ,
                10.0, 0.1, 0.1, 0.1));
            fpvec2.push_back(* new FakePixel( 0.4,  1.,  0.,  0.5, 0.1, 48.0 , 35.0 ,
                10.0, 0.1, 0.1, 0.1));

            mypnt1 = new FakePoint( 10.0, 1.0, &fpvec, 0.1, 0.1) ;
            mypnt2 = new FakePoint( 12.0, 2.0, &fpvec2, 0.3, 0.1) ;
            mypnts.push_back(*mypnt1);
            mypnts.push_back(*mypnt2);

            return  new FakeCut(mypnts);
        }
        */

    } // namespace Algorithms
} // namespace Mantid
