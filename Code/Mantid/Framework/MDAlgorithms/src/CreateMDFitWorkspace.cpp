//#include <exception>
//#include "MantidMDAlgorithms/CreateMDFitWorkspace.h"
//#include "MantidMDDataObjects/MDFitWorkspace.h"
//#include "MantidGeometry/MDGeometry/MDPoint.h"
//#include "MantidAPI/IMDIterator.h"
//#include "MantidKernel/Property.h"
//
//#include "MantidGeometry/muParser_Silent.h"
//#include <Poco/StringTokenizer.h>
//#include <boost/scoped_ptr.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/lambda/bind.hpp>
//#include <cstdlib>
//#include <fstream>
//
// using namespace Mantid::API;
// using namespace Mantid::Kernel;
// using namespace Mantid::Geometry;
// using namespace Mantid::MDDataObjects;
// using namespace boost::lambda;
//
// namespace Mantid
//{
//  namespace MDAlgorithms
//  {
//    using namespace Mantid::API;
//
//    // Register the class into the algorithm factory
//    DECLARE_ALGORITHM(CreateMDFitWorkspace)
//
//    void CreateMDFitWorkspace::init()
//    {
//      declareProperty(new
//      WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
//      "Name of the output Workspace");
//      declareProperty("Dimensions","","Dimensions description");
//      declareProperty("Formula","","muparser function to generate data in the
//      workspace. Variables are x,y,z,t");
//      declareProperty("MaxPoints",1,"Maximum number of points per cell.
//      MDPoints are generated randomly");
//      declareProperty("Noise",0.0,"Random noise to add to the signal");
//    }
//
//    void CreateMDFitWorkspace::exec()
//    {
//      std::string dimensions = getPropertyValue("Dimensions");
//      Poco::StringTokenizer tkz(dimensions, ";",
//      Poco::StringTokenizer::TOK_IGNORE_EMPTY |
//      Poco::StringTokenizer::TOK_TRIM);
//      size_t nDim = tkz.count();
//      if (nDim > 4)
//      {
//        throw std::invalid_argument("Maximum dimensionality of a workspace
//        created by CreateMDFitWorkspace is 4");
//      }
//      MDFitWorkspace_sptr ws = MDFitWorkspace_sptr(new MDFitWorkspace(
//      (unsigned int)(nDim)));
//      int i = 0;
//      for(Poco::StringTokenizer::Iterator it = tkz.begin();it != tkz.end();
//      it++, i++ )
//      {
//        ws->setDimension(i,*it);
//      }
//
//      std::string varNames[] = {"x","y","z","t"};
//      std::vector<double> vars(4);
//
//      mu::Parser fun;
//
//      for(size_t i = 0; i < vars.size(); ++i)
//      {
//        fun.DefineVar(varNames[i],&vars[i]);
//      }
//
//      try
//      {
//        fun.SetExpr(getPropertyValue("Formula"));
//      }
//      catch(...)
//      {
//        throw std::invalid_argument("Formula cannot be parsed my muParser");
//      }
//
//      int maxPoints = getProperty("MaxPoints");
//      double noise = getProperty("Noise");
//      boost::scoped_ptr<IMDIterator> it( ws->createIterator() );
//      do
//      {
//        for(size_t i = 0; i < nDim; ++i)
//        {
//          vars[i] = it->getCoordinate(int(i));
//        }
//        double signal = fun.Eval() + noise;
//        double error = 1;
//        int n = rand() % maxPoints;
//        if (n < 1) n = 1;
//        std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > points(n);
//        for(int i = 1; i < n; ++i)
//        {
//          double s = double(rand()) / RAND_MAX * signal * 0.75;
//          points[i].reset(new
//          MDPoint(s,error,std::vector<Coordinate>(),IDetector_sptr(),Instrument_sptr()));
//          signal -= s;
//        }
//        points[0].reset(new
//        MDPoint(signal,error,std::vector<Coordinate>(),IDetector_sptr(),Instrument_sptr()));
//        size_t i = it->getPointer();
//        ws->setCell(i,points);
//      }
//      while(it->next());
//
//      setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(ws));
//
//      // test
//
//      if (nDim <= 2)
//      {
//        std::string fname = "C:/Users/hqs74821/Work/InstrumentBug/" +
//        getPropertyValue("OutputWorkspace") + ".csv";
//        std::ofstream fil(fname.c_str());
//        size_t nx = ws->getXDimension()->getNBins();
//        it.reset( ws->createIterator() );
//        do
//        {
//          size_t i = it->getPointer();
//          if (nDim == 2)
//          {
//            fil <<  ws->getCell(i).getSignal();
//            fil << (((i+1) % nx == 0)? '\n' : ',');
//          }
//          else
//          {
//            fil << it->getCoordinate(0) << ',' << ws->getCell(i).getSignal()
//            << std::endl;
//          }
//        }while(it->next());
//      }
//
//    }
//
//  } // MDAlgorithms
//} // Mantid
