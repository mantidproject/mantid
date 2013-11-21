/*WIKI* 


*WIKI*/
#include "MantidCrystal/OptimizeLatticeParameters.h"
#include "MantidCrystal/GSLFunctions.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>

using namespace Mantid::Geometry;

namespace Mantid
{
  namespace Crystal
  {
    Kernel::Logger& OptimizeLatticeParameters::g_log =
                        Kernel::Logger::get("OptimizeLatticeParameters");

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(OptimizeLatticeParameters)
    
    /// Sets documentation strings for this algorithm
    void OptimizeLatticeParameters::initDocs()
    {
      this->setWikiSummary("Optimize lattice parameters for cell type.");
      this->setOptionalMessage("Optimize lattice parameters for cell type.");
    }
    
    using namespace Kernel;
    using namespace API;
    using std::size_t;
    using namespace DataObjects;

    /// Constructor
    OptimizeLatticeParameters::OptimizeLatticeParameters()
    {
    }

    /// Destructor
    OptimizeLatticeParameters::~OptimizeLatticeParameters()
    {}
  
    static double gsl_costFunction(const gsl_vector *v, void *params)
    {
      std::string *p = (std::string *)params;
      std::string inname = p[0];
      std::string cell_type = p[1];
      std::vector<double>Params;
      for ( size_t i = 0; i < v->size; i++ )Params.push_back(gsl_vector_get(v,i));
      Mantid::Crystal::OptimizeLatticeParameters u;
      return u.optLattice(inname, cell_type, Params);
    }
  
    //-----------------------------------------------------------------------------------------
    /** Initialisation method. Declares properties to be used in algorithm.
     */
    void OptimizeLatticeParameters::init()
    {

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::InOut),
        "An input PeaksWorkspace with an instrument.");
    std::vector<std::string> cellTypes;
    cellTypes.push_back("Cubic" );
    cellTypes.push_back("Tetragonal" );
    cellTypes.push_back("Orthorhombic");
    cellTypes.push_back("Rhombohedral");
    cellTypes.push_back("Monoclinic ( a unique )");
    cellTypes.push_back("Monoclinic ( b unique )");
    cellTypes.push_back("Monoclinic ( c unique )");
    cellTypes.push_back("Triclinic");
    declareProperty("CellType", cellTypes[0],boost::make_shared<StringListValidator>(cellTypes),
      "Select the cell type.");

    declareProperty("OutputChi2", 0.0,Direction::Output);

      //Disable default gsl error handler (which is to call abort!)
      gsl_set_error_handler_off();
    }

    //-----------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void OptimizeLatticeParameters::exec()
    {
      std::string par[6];
      std::string inname = getProperty("InputWorkspace");
      par[0] = inname;
      std::string type = getProperty("CellType");
      par[1] = type;
      PeaksWorkspace_sptr ws = getProperty("InputWorkspace");

      const gsl_multimin_fminimizer_type *T =
      gsl_multimin_fminimizer_nmsimplex;
      gsl_multimin_fminimizer *s = NULL;
      gsl_vector *ss, *x;
      gsl_multimin_function minex_func;
    
      // finally do the optimization
    
      size_t nopt = 1;
      if(type.compare(0,2,"Te")==0)nopt = 2;
      else if(type.compare(0,2,"Or")==0)nopt = 3;
      else if(type.compare(0,2,"Rh")==0)nopt = 2;
      else if(type.compare(0,2,"He")==0)nopt = 2;
      else if(type.compare(0,2,"Mo")==0)nopt = 4;
      else if(type.compare(0,2,"Tr")==0)nopt = 6;
      size_t iter = 0;
      int status = 0;
     
      /* Starting point */
      x = gsl_vector_alloc (nopt);
      const DblMatrix UB = ws->sample().getOrientedLattice().getUB();
      std::vector<double>lat(6);
      IndexingUtils::GetLatticeParameters( UB, lat);
      // initialize parameters for optimization
      gsl_vector_set (x, 0, lat[0]);
      if(type.compare(0,2,"Te")==0)gsl_vector_set (x, 1, lat[2]);
      else if(type.compare(0,2,"Or")==0)
      {
        gsl_vector_set (x, 1, lat[1]);
        gsl_vector_set (x, 2, lat[2]);
      }
      else if(type.compare(0,2,"Rh")==0)gsl_vector_set (x, 1, lat[3]);
      else if(type.compare(0,2,"He")==0)gsl_vector_set (x, 1, lat[2]);
      else if(type.compare(0,2,"Mo")==0)
      {
        gsl_vector_set (x, 1, lat[1]);
        gsl_vector_set (x, 2, lat[2]);
        if(type.compare(13,1,"a")==0)gsl_vector_set (x, 3, lat[3]);
        else if(type.compare(13,1,"b")==0)gsl_vector_set (x, 3, lat[4]);
        else if(type.compare(13,1,"c")==0)gsl_vector_set (x, 3, lat[5]);
      }
      else if(type.compare(0,2,"Tr")==0)for ( size_t i = 1; i < nopt; i++ )gsl_vector_set (x, i, lat[i]);
    
      /* Set initial step sizes to 0.001 */
      ss = gsl_vector_alloc (nopt);
      gsl_vector_set_all (ss, 0.001);
    
      /* Initialize method and iterate */
      minex_func.n = nopt;
      minex_func.f = &Mantid::Crystal::gsl_costFunction;
      minex_func.params = &par;
    
      s = gsl_multimin_fminimizer_alloc (T, nopt);
      gsl_multimin_fminimizer_set (s, &minex_func, x, ss);
    
      do
      {
        iter++;
        status = gsl_multimin_fminimizer_iterate(s);
        if (status)
          break;
    
        double size = gsl_multimin_fminimizer_size (s);
        status = gsl_multimin_test_size (size, 1e-4);
    
      }
      while (status == GSL_CONTINUE && iter < 5000);
    
      // Output summary to log file
      std::string report = gsl_strerror(status);
      g_log.notice() <<
        " Method used = " << " Simplex" << 
        " Iteration = " << iter << 
        " Status = " << report << 
        " Chisq = " << s->fval ; 
      gsl_vector_free(x);
      gsl_vector_free(ss);
      gsl_multimin_fminimizer_free (s);
      setProperty("OutputChi2", s->fval);
      // Show the modified lattice parameters
      OrientedLattice o_lattice = ws->mutableSample().getOrientedLattice();
      g_log.notice() << o_lattice << "\n";

    }


   //-----------------------------------------------------------------------------------------
    /**
     * Calls Gaussian1D as a child algorithm to fit the offset peak in a spectrum
     * @param mosaic
     * @param rcrystallite
     * @param inname
     * @param corrOption
     * @param pointOption
     * @param tofParams
     * @return
     */
    double OptimizeLatticeParameters::optLattice(std::string inname, std::string cell_type, std::vector<double> & params)
    {
      PeaksWorkspace_sptr ws = boost::dynamic_pointer_cast<PeaksWorkspace>
           (AnalysisDataService::Instance().retrieve(inname));
      const std::vector<Peak> &peaks = ws->getPeaks();
      size_t n_peaks = ws->getNumberPeaks();
      std::vector<V3D> q_vector;
      std::vector<V3D> hkl_vector;

      for ( size_t i = 0; i < params.size(); i++ )params[i]=std::abs(params[i]);
      for ( size_t i = 0; i < n_peaks; i++ )
      {
        q_vector.push_back(peaks[i].getQSampleFrame());
        hkl_vector.push_back(peaks[i].getHKL());
      }
      std::vector<double> lattice_parameters;
      lattice_parameters.assign (6,0);
      if (cell_type == "Cubic")
      {
        lattice_parameters[0] = params[0];       
        lattice_parameters[1] = params[0];
        lattice_parameters[2] = params[0];      
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = 90;
        lattice_parameters[5] = 90;
      }
      else if (cell_type == "Tetragonal")
      {
        lattice_parameters[0] = params[0];    
        lattice_parameters[1] = params[0];
        lattice_parameters[2] = params[1];   
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = 90;
        lattice_parameters[5] = 90;
      }
      else if (cell_type == "Orthorhombic")
      {
        lattice_parameters[0] = params[0];  
        lattice_parameters[1] = params[1];
        lattice_parameters[2] = params[2]; 
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = 90;
        lattice_parameters[5] = 90;
      }
      else if (cell_type == "Rhombohedral")
      {
        lattice_parameters[0] = params[0];
        lattice_parameters[1] = params[0];
        lattice_parameters[2] = params[0];  
    
        lattice_parameters[3] = params[1];
        lattice_parameters[4] = params[1];
        lattice_parameters[5] = params[1];
      }
      else if (cell_type == "Hexagonal")
      {
        lattice_parameters[0] = params[0]; 
        lattice_parameters[1] = params[0];
        lattice_parameters[2] = params[1];
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = 90;
        lattice_parameters[5] = 120;
      }
      else if (cell_type == "Monoclinic ( a unique )")
      {
        lattice_parameters[0] = params[0];
        lattice_parameters[1] = params[1];
        lattice_parameters[2] = params[2];
    
        lattice_parameters[3] = params[3];
        lattice_parameters[4] = 90;
        lattice_parameters[5] = 90;
      }
      else if (cell_type == "Monoclinic ( b unique )")
      {
        lattice_parameters[0] = params[0];
        lattice_parameters[1] = params[1];
        lattice_parameters[2] = params[2];
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = params[3];
        lattice_parameters[5] = 90;
      }
      else if (cell_type == "Monoclinic ( c unique )")
      {
        lattice_parameters[0] = params[0];
        lattice_parameters[1] = params[1];
        lattice_parameters[2] = params[2];
    
        lattice_parameters[3] = 90;
        lattice_parameters[4] = 90;
        lattice_parameters[5] = params[3];
      }
      else if (cell_type == "Triclinic")
      {
        lattice_parameters[0] = params[0];
        lattice_parameters[1] = params[1];
        lattice_parameters[2] = params[2]; 
    
        lattice_parameters[3] = params[3];
        lattice_parameters[4] = params[4];
        lattice_parameters[5] = params[5];
      }
    
    
      Mantid::API::IAlgorithm_sptr alg = createChildAlgorithm("CalculateUMatrix");
      alg->setPropertyValue("PeaksWorkspace", inname);
      alg->setProperty("a",lattice_parameters[0]);
      alg->setProperty("b",lattice_parameters[1]);
      alg->setProperty("c",lattice_parameters[2]);
      alg->setProperty("alpha",lattice_parameters[3]);
      alg->setProperty("beta",lattice_parameters[4]);
      alg->setProperty("gamma",lattice_parameters[5]);
      alg->executeAsChildAlg();
    
      ws = alg->getProperty("PeaksWorkspace");
      OrientedLattice latt=ws->mutableSample().getOrientedLattice();
      DblMatrix UB = latt.getUB();
    
      double result = 0;
      for ( size_t i = 0; i < hkl_vector.size(); i++ ) 
      {
         V3D error = UB * hkl_vector[i] - q_vector[i] / (2.0 * M_PI);
         result += error.norm();
      }
      return result;
      }
  } // namespace Algorithm
} // namespace Mantid
