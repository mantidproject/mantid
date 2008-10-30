//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebunch.h"
#include "MantidAPI/Workspace.h"

#include <sstream>
#include <numeric>
#include <math.h>

namespace Mantid
{
	namespace Algorithms
	{

		// Register the class into the algorithm factory
		DECLARE_ALGORITHM(Rebunch)

		using namespace Kernel;
		using API::WorkspaceProperty;
		using API::Workspace_const_sptr;
		using API::Workspace;

		// Get a reference to the logger
		Logger& Rebunch::g_log = Logger::get("Rebunch");

		/** Initialisation method. Declares properties to be used in algorithm.
		*
		*/
		void Rebunch::init()
		{
			declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
			declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));

			BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
			mustBePositive->setLower(1);
			declareProperty("n_bunch",1, mustBePositive);

		}

		/** Executes the rebin algorithm
		*
		*  @throw runtime_error Thrown if
		*/
		void Rebunch::exec()
		{
			// retrieve the properties
			int n_bunch=getProperty("n_bunch");

			// Get the input workspace
			Workspace_const_sptr inputW = getProperty("InputWorkspace");

			bool dist = inputW->isDistribution();

			// workspace independent determination of length
			int histnumber = inputW->size()/inputW->blocksize();

			/*
			const std::vector<double>& Xold = inputW->dataX(0);
			const std::vector<double>& Yold = inputW->dataY(0);
			int size_x=Xold.size();
			int size_y=Yold.size();
			*/
			int size_x = inputW->dataX(0).size();
			int size_y = inputW->dataY(0).size();

			//signal is the same length for histogram and point data
			int ny=(size_y/n_bunch);
			if(size_y%n_bunch >0)ny+=1;
			// default is for hist
			int nx=ny+1;
			bool point=false;
			if (size_x==size_y)
			{
				point=true;
				nx=ny;
			}

			// make output Workspace the same type is the input, but with new length of signal array
			API::Workspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,nx,ny);

            int progress_step = histnumber / 100;
            if (progress_step == 0) progress_step = 1;
			for (int hist=0; hist <  histnumber;hist++)
			{
				const API::IErrorHelper* e_ptr= inputW->errorHelper(hist);
				if(dynamic_cast<const API::GaussianErrorHelper*>(e_ptr) ==0)
				{
					g_log.error("Can only rebunch Gaussian data");
					throw std::invalid_argument("Invalid input Workspace");
				}


				// get const references to input Workspace arrays (no copying)
				const std::vector<double>& XValues = inputW->dataX(hist);
				const std::vector<double>& YValues = inputW->dataY(hist);
				const std::vector<double>& YErrors = inputW->dataE(hist);

				//get references to output workspace data (no copying)
				std::vector<double>& XValues_new=outputW->dataX(hist);
				std::vector<double>& YValues_new=outputW->dataY(hist);
				std::vector<double>& YErrors_new=outputW->dataE(hist);

				// output data arrays are implicitly filled by function
				if(point)
				{
					rebunch_point(XValues,YValues,YErrors,XValues_new,YValues_new,YErrors_new,n_bunch);
				}
				else
				{
					rebunch_hist(XValues,YValues,YErrors,XValues_new,YValues_new,YErrors_new,n_bunch, dist);
				}


				//copy oer the spectrum No and ErrorHelper
				//        outputW->getAxis()->spectraNo(hist)=inputW->getAxis()->spectraNo(hist);
				outputW->setErrorHelper(hist,inputW->errorHelper(hist));
                if (hist % progress_step == 0)
                {
                    progress(double(hist)/histnumber);
                    interruption_point();
                }
			}
			outputW->isDistribution(dist);

            // Copy units
            if (outputW->getAxis(0)->unit().get())
                outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
            try
            {
                if (inputW->getAxis(1)->unit().get())
                    outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
            }
            catch(Exception::IndexError) {
                // OK, so this isn't a Workspace2D
            }

			// Assign it to the output workspace property
			setProperty("OutputWorkspace",outputW);

			return;
		}

		/** Rebunches histogram data data according to n_bunch input
		*
		* @param xold - old x array of data
		* @param xnew - new x array of data
		* @param yold - old y array of data
		* @param ynew - new y array of data
		* @param eold - old error array of data
		* @param enew - new error array of data
		* @param n_bunch - number of data points to bunch together for each new point
		* @param distribution - flag defining if distribution data (1) or not (0)
		* @throw runtime_error Thrown if algorithm cannot execute
		* @throw invalid_argument Thrown if input to function is incorrect
		**/
		void Rebunch::rebunch_hist(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
			std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, const int n_bunch, const bool distribution)
		{
			int i,j;
			double width;
			int size_x=xold.size();
			int size_y=yold.size();
			double ysum,esum;
			int hi_index=size_x-1;
			int wbins=size_y/n_bunch;
			int rem=size_y%n_bunch;


			int i_in=0;
			j=0;
			while (j < wbins )
			{
				ysum=0.0;
				esum=0.0;
				for(i=1;i <= n_bunch;i++)
				{
					if(distribution)
					{
						width=xold[i_in+1]-xold[i_in];
						ysum+=yold[i_in]*width;
						esum+=eold[i_in]*eold[i_in]*width*width;
						i_in++;
					}
					else
					{
						ysum+=yold[i_in];
						esum+=eold[i_in]*eold[i_in];
						i_in++;
					}
				}
				//average contributing x values
				ynew[j]=ysum;
				enew[j]=sqrt(esum);
				j++;
			}
			if(rem != 0)
			{
				ysum=0.0;
				esum=0.0;
				for(i=1;i <= rem;i++)
				{
					if(distribution)
					{
						width=xold[i_in+1]-xold[i_in];
						ysum+=yold[i_in]*width;
						esum+=eold[i_in]*eold[i_in]*width*width;
						i_in++;
					}
					else
					{
						ysum+=yold[i_in];
						esum+=eold[i_in]*eold[i_in];
						i_in++;
					}
				}
				ynew[j]=ysum;
				enew[j]=sqrt(esum);
			}

			j=0;
			xnew[j]=xold[0];
			j++;
			for(i=n_bunch;i<hi_index;i+=n_bunch)
			{
				xnew[j]=xold[i];
				j++;
			}
			xnew[j]=xold[hi_index];

			if(distribution)
				for(i=0;i<ynew.size();i++)
				{
					width=xnew[i+1]-xnew[i];
					ynew[i]=ynew[i]/width;
					enew[i]=enew[i]/width;
				}
		}

		/** Rebunches point data data according to n_bunch input
		*
		* @param xold - old x array of data
		* @param xnew - new x array of data
		* @param yold - old y array of data
		* @param ynew - new y array of data
		* @param eold - old error array of data
		* @param enew - new error array of data
		* @param n_bunch - number of data points to bunch together for each new point
		* @throw runtime_error Thrown if algorithm cannot execute
		* @throw invalid_argument Thrown if input to function is incorrect
		**/

		void Rebunch::rebunch_point(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
			std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, const int n_bunch)
		{
			int i,j;
			int size_y=yold.size();
			double xsum,ysum,esum;
			int wbins=size_y/n_bunch;
			int rem=size_y%n_bunch;


			int i_in=0;
			j=0;
			while (j < wbins )
			{
				xsum=0.0;
				ysum=0.0;
				esum=0.0;
				for(i=1;i <= n_bunch;i++)
				{
					xsum+=xold[i_in];
					ysum+=yold[i_in];
					esum+=eold[i_in]*eold[i_in];
					i_in++;
				}
				//average contributing x values
				xnew[j]=xsum/(double)n_bunch;
				ynew[j]=ysum/(double)n_bunch;
				enew[j]=sqrt(esum)/(double)n_bunch;
				j++;
			}
			if(rem != 0)
			{
				xsum=0.0;
				ysum=0.0;
				esum=0.0;
				for(i=1;i <= rem;i++)
				{
					xsum+=xold[i_in];
					ysum+=yold[i_in];
					esum+=eold[i_in]*eold[i_in];
					i_in++;
				}
				xnew[j]=xsum/(double)rem;
				ynew[j]=ysum/(double)rem;
				enew[j]=sqrt(esum)/(double)rem;
			}

		}

	} // namespace Algorithm
} // namespace Mantid




