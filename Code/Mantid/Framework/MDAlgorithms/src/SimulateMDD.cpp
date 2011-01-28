//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/SimulateMDD.h"
#include "MantidMDAlgorithms/TobyFitSimulate.h"
#include <math.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>
#include <algorithm>

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
    namespace MDAlgorithms
    {
        // Register the class into the algorithm factory
        DECLARE_ALGORITHM(SimulateMDD)
        // Constructor
        SimulateMDD::SimulateMDD() //: m_sobol(false), m_randSeed(12345678),m_randGen(NULL)
        {
        }

        SimulateMDD::~SimulateMDD()
        {}

        void SimulateMDD::init()
        {
            //declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "Name of the input Workspace");
            // for testing just use a dummy string for the input workspace name
            declareProperty("InputMDWorkspace","",Direction::Input );
            //

            // declare functions - for now only use background model
            // may have different backgrounds for different cuts, which will require an extension
            // 
            declareProperty("ForegroundModel","",Direction::Input );
            declareProperty("ForegroundModel_p1",0.0, "First param of bg model", Direction::Input);
            declareProperty("ForegroundModel_p2",0.0, "Second param of bg model", Direction::Input);
            declareProperty("ForegroundModel_p3",0.0, "Third param of bg model", Direction::Input);
            declareProperty("BackgroundModel","",Direction::Input );
            declareProperty("BackgroundModel_p1",0.0, "Background bp1", Direction::Input);
            declareProperty("BackgroundModel_p2",0.0, "Background bp2", Direction::Input);
            declareProperty("BackgroundModel_p3",0.0, "Background bp3", Direction::Input);
            declareProperty("BackgroundModel_p4",0.0, "Background bp4", Direction::Input);
            declareProperty("BackgroundModel_p5",0.0, "Background bp5", Direction::Input);
            declareProperty("BackgroundModel_p6",0.0, "Background bp6", Direction::Input);
            declareProperty("BackgroundModel_p7",0.0, "Background bp7", Direction::Input);
            declareProperty("BackgroundModel_p8",0.0, "Background bp8", Direction::Input);
            // In future, should be a new MDData workspace with the simulated results
            declareProperty("OutputMDWorkspace","",Direction::Output);
            declareProperty("Residual", -1.0, Direction::Output);
        }

        void SimulateMDD::exec()
        {
            std::string inputMDwrkspc = getProperty("InputMDWorkspace");
            std::string bgmodel = getProperty("BackgroundModel");
            const double bgparaP1 = getProperty("BackgroundModel_p1");
            const double bgparaP2 = getProperty("BackgroundModel_p2");
            const double bgparaP3 = getProperty("BackgroundModel_p3");
            const double bgparaP4 = getProperty("BackgroundModel_p4");
            const double bgparaP5 = getProperty("BackgroundModel_p5");
            const double bgparaP6 = getProperty("BackgroundModel_p6");
            const double bgparaP7 = getProperty("BackgroundModel_p7");
            const double bgparaP8 = getProperty("BackgroundModel_p8");

            std::string fgmodel = getProperty("ForegroundModel");
            const double fgparaP1 = getProperty("ForegroundModel_p1");
            const double fgparaP2 = getProperty("ForegroundModel_p2");
            const double fgparaP3 = getProperty("ForegroundModel_p3");
            std::string inputMDoutput = getProperty("OutputMDWorkspace");

            //boost::shared_ptr<Mantid::API::Workspace> inputWS = getProperty("InputWorkspaceTMP");
            imdwCut = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(AnalysisDataService::Instance().retrieve(inputMDwrkspc));
            //g_log.warning("No FakeCut data yet available in SimulateMDD");
            std::vector<double> cellBg;
            SimBackground(cellBg, bgmodel,bgparaP1,bgparaP2,bgparaP3,bgparaP4,bgparaP5,bgparaP6,bgparaP7,bgparaP8);
            TobyFitSimulate* tfSim = new TobyFitSimulate();
            tfSim->SimForeground(imdwCut,fgmodel,fgparaP1,fgparaP2,fgparaP3);

            double residual=0;
            // TO DO add in foreground component
            double weightSq;
            for(size_t i=0;i<cellBg.size();i++) {
                weightSq=1./pow(imdwCut->getCell(i).getError(),2);
                residual+=pow(cellBg[i]-imdwCut->getCell(i).getSignal(),2)*weightSq;
            };
            setProperty("Residual", residual);
        }
        void SimulateMDD::SimBackground(std::vector<double>& cellBg, std::string bgmodel,const double bgparaP1, const double bgparaP2, const double bgparaP3,
                                        const double bgparaP4, const double bgparaP5, const double bgparaP6,
                                        const double bgparaP7, const double bgparaP8)
        {
            int ncell= imdwCut->getXDimension()->getNBins();
            // loop over cells of cut
            for(int i=0; i<ncell ; i++ ){
                double bgsum=0.;
                double eps,phi;
                const Mantid::Geometry::SignalAggregate& newCell = imdwCut->getCell(i);
                std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > myPoints = newCell.getContributingPoints();
                // Assume that the energy (centre point) is in coordinate.t of first point vertex
                if( ! bgmodel.compare("QuadEnTrans")) {
                    for(size_t j=0; j<myPoints.size(); j++){
                        // TO DO: this is expensive way to get eps, should use pointer
                        std::vector<Mantid::Geometry::coordinate> vertexes = myPoints[j]->getVertexes();
                        eps=vertexes[0].t;
                        bgsum+=bgparaP1+eps*(bgparaP2+eps*bgparaP3);
                    }
                }
                else if( ! bgmodel.compare("ExpEnTrans")) {
                    for(size_t j=0; j<myPoints.size(); j++){
                        std::vector<Mantid::Geometry::coordinate> vertexes = myPoints[j]->getVertexes();
                        eps=vertexes[0].t;
                        bgsum+=bgparaP1+bgparaP2*exp(-eps/bgparaP3);
                    }
                }
                else if( ! bgmodel.compare("QuadEnTransAndPhi")) {
                    double dphi,deps;
                    for(size_t j=0; j<myPoints.size(); j++){
                        std::vector<Mantid::Geometry::coordinate> vertexes = myPoints[j]->getVertexes();
                        eps=vertexes[0].t;
                        deps=eps-bgparaP1;
                        phi=0.; // TO DO: get phi of detector
                        dphi=phi-bgparaP2;
                        bgsum+=bgparaP3+bgparaP4*deps+bgparaP5*dphi+bgparaP6*deps*deps+2.*bgparaP7*deps*dphi+bgparaP8*dphi*dphi;
                    }
                }
                else if( ! bgmodel.compare("ExpEnTransAndPhi")) {
                    for(size_t j=0; j<myPoints.size(); j++){
                        std::vector<Mantid::Geometry::coordinate> vertexes = myPoints[j]->getVertexes();
                        eps=vertexes[0].t;
                        phi=0.; // TO DO: get phi of detector
                        bgsum+=bgparaP3*exp(-(eps-bgparaP1)/bgparaP4 - (phi-bgparaP2)/bgparaP5 );
                    }
                }
                else {
                    //g_log.error("undefined background model: "+bgmodel);
                }
                // Would pre size cellBg vector, but no getNCell method for IMDWorkspace
                if(cellBg.size()<i+1)
                    cellBg.push_back(bgsum);
                else
                    cellBg[i]+=bgsum;
            }
        }
        //



    } // namespace Algorithms
} // namespace Mantid
