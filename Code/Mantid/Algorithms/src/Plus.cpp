//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <cmath>
#include "MantidAlgorithms/plus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 

DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,plus)
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory

    using namespace Mantid::Kernel;
    using Mantid::DataObjects::Workspace1D;
    using Mantid::DataObjects::Workspace2D;

    // Get a reference to the logger
    Logger& plus::g_log = Logger::get("plus");

    /** Initialisation method. 
    * Defines input and output workspaces
    * 
    */
    void plus::init()
    {
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_1","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_2","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));    
    }

    /** Creates an output workspace to fill with the calculated data
    * @param m_inputWorkspace The input workspace to use as a template
    * @returns An new workspace with the same dimensions as the input workspace, all values set to 0.
    */
    Workspace* plus::createOutputWorkspace(Workspace* m_inputWorkspace)
    {
      Workspace1D* d1_out = dynamic_cast<Workspace1D*>(m_inputWorkspace);
      if (d1_out != 0)
      {    
        // create another input workspace1d with other values
        std::vector<double> x(m_inputWorkspace->size(),0),sig(m_inputWorkspace->size(),0),err(m_inputWorkspace->size(),0);
        d1_out = new Workspace1D;
        d1_out->setData(sig,err);
        d1_out->setX(x);
        return d1_out; 
      }
      else
      {
        Workspace2D* d2_out = dynamic_cast<Workspace2D*>(m_inputWorkspace);     
        //do w2d things
        std::vector<double> x(m_inputWorkspace->blocksize(),0),y(m_inputWorkspace->blocksize(),0),e(m_inputWorkspace->blocksize(),0);
        d2_out = new Workspace2D;
        int len=m_inputWorkspace->size()/m_inputWorkspace->blocksize();
        d2_out->setHistogramNumber(len);
        for (int i = 0; i < len; i++)
        {
          d2_out->setX(i,x);
          d2_out->setData(i,y,e);
        }
        return d2_out;
      }
    }  
    /** Executes the algorithm
    * 
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void plus::exec()
    {
      // get input workspace, dynamic cast not needed
      Property* p1 = getProperty("InputWorkspace_1");
      Property* p2 = getProperty("InputWorkspace_2");

      WorkspaceProperty<Workspace> *wp1 = dynamic_cast< WorkspaceProperty<Workspace>* >(p1);
      WorkspaceProperty<Workspace> *wp2 = dynamic_cast< WorkspaceProperty<Workspace>* >(p2);  
      Workspace* in_work1 = *wp1;
      Workspace* in_work2 = *wp2;

      Workspace* out_work = createOutputWorkspace(in_work1);

      triple_iterator<Workspace> ti_out(*out_work);
      triple_iterator<Workspace> ti_in1(*in_work1);
      triple_iterator<Workspace> ti_in2(*in_work2);
      std::transform(ti_in1.begin(),ti_in1.end(),ti_in2.begin(),ti_out.begin(),plus_fn());

      std::transform(ti_in1.begin(),ti_in1.end(),ti_in2.begin(),ti_out.begin(),plus_fn());

      // Assign it to the output workspace property
      Property* p3 = getProperty("OutputWorkspace");
      WorkspaceProperty<Workspace> *out = dynamic_cast< WorkspaceProperty<Workspace>* >(p3);
      *out = out_work;

      return;
    }

    /** Finalisation method. Does nothing at present.
    *
    */
    void plus::final()
    {
    }

    /* Performs the addition with Gausian errors within the transform function
    * @param a The triple ref of the first workspace data item
    * @param b The triple ref of the second workspace data item
    * @returns A triple ref of the result with Gausian errors
    */
    TripleRef<double&>
      plus::plus_fn::operator() (const TripleRef<double&>& a,const TripleRef<double&>& b) const 
    {           
      double ret_sig(a[1]+b[1]);
      //gaussian errors for the moment
      double ret_err(sqrt((a[2]*a[2])+(b[2]*b[2])));     
      return TripleRef<double&>(a[0],ret_sig,ret_err);      
    }
  }
}
