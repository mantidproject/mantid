//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Rebunch.h"
#include "MantidAPI/Workspace.h"

#include <sstream>
#include <numeric>
#include <cmath>

namespace Mantid
{
	namespace Algorithms
	{

		// Register the class into the algorithm factory
		DECLARE_ALGORITHM(Rebunch)

		using namespace Kernel;
		using API::WorkspaceProperty;
		using API::MatrixWorkspace_const_sptr;
		using API::MatrixWorkspace;

		/** Initialisation method. Declares properties to be used in algorithm.
		*
		*/
		void Rebunch::init()
		{
		  //this->setWikiSummary("Rebins data by adding together ''n_bunch'' successive bins.");
		  //this->setOptionalMessage("Rebins data by adding together 'n_bunch' successive bins.");

			declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
        "Name of the input workspace" );
			declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "The name of the workspace to be created as the output of the algorithm");

			BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
			mustBePositive->setLower(1);
			declareProperty("NBunch",1, mustBePositive,
        "The number of bins to that will be summed in each bunch");
		}

		/** Executes the rebin algorithm
		*
		*  @throw runtime_error Thrown if
		*/
		void Rebunch::exec()
		{
			// retrieve the properties
			int n_bunch=getProperty("NBunch");

			// Get the input workspace
			MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");

			bool dist = inputW->isDistribution();

			// workspace independent determination of length
			int histnumber = inputW->size()/inputW->blocksize();

			/*
			const std::vector<double>& Xold = inputW->readX(0);
			const std::vector<double>& Yold = inputW->readY(0);
			int size_x=Xold.size();
			int size_y=Yold.size();
			*/
			int size_x = inputW->readX(0).size();
			int size_y = inputW->readY(0).size();

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
			API::MatrixWorkspace_sptr outputW = API::WorkspaceFactory::Instance().create(inputW,histnumber,nx,ny);

            int progress_step = histnumber / 100;
            if (progress_step == 0) progress_step = 1;
			PARALLEL_FOR2(inputW,outputW)
			for (int hist=0; hist <  histnumber;hist++)
			{
				PARALLEL_START_INTERUPT_REGION
				// Ensure that axis information are copied to the output workspace if the axis exists
			        try
				{
				  outputW->getAxis(1)->spectraNo(hist)=inputW->getAxis(1)->spectraNo(hist);
				}
				catch( Exception::IndexError& )
				{ 
				  // Not a Workspace2D
				}

				// get const references to input Workspace arrays (no copying)
				const MantidVec& XValues = inputW->readX(hist);
				const MantidVec& YValues = inputW->readY(hist);
				const MantidVec& YErrors = inputW->readE(hist);

				//get references to output workspace data (no copying)
				MantidVec& XValues_new=outputW->dataX(hist);
				MantidVec& YValues_new=outputW->dataY(hist);
				MantidVec& YErrors_new=outputW->dataE(hist);

				// output data arrays are implicitly filled by function
				if(point)
				{
					rebunch_point(XValues,YValues,YErrors,XValues_new,YValues_new,YErrors_new,n_bunch);
				}
				else
				{
					rebunch_hist(XValues,YValues,YErrors,XValues_new,YValues_new,YErrors_new,n_bunch, dist);
				}

				if (hist % progress_step == 0)
				{
				  progress(double(hist)/histnumber);
				  interruption_point();
				}
				PARALLEL_END_INTERUPT_REGION
			}
			PARALLEL_CHECK_INTERUPT_REGION
			outputW->isDistribution(dist);

			// Copy units
			if (outputW->getAxis(0)->unit().get())
			  outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
			try
			{
			  if (inputW->getAxis(1)->unit().get())
			    outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
			}
			catch(Exception::IndexError&) {
			  // OK, so this isn't a Workspace2D
			}

			// Assign it to the output workspace property
			setProperty("OutputWorkspace",outputW);

			return;
		}

		/** Rebunches histogram data data according to n_bunch input
		*
		* @param xold :: old x array of data
		* @param xnew :: new x array of data
		* @param yold :: old y array of data
		* @param ynew :: new y array of data
		* @param eold :: old error array of data
		* @param enew :: new error array of data
		* @param n_bunch :: number of data points to bunch together for each new point
		* @param distribution :: flag defining if distribution data (1) or not (0)
		* @throw runtime_error Thrown if algorithm cannot execute
		* @throw invalid_argument Thrown if input to function is incorrect
		**/
		void Rebunch::rebunch_hist(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
			std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, const size_t n_bunch, const bool distribution)
		{
			size_t i,j;
			double width;
			size_t size_x=xold.size();
			size_t size_y=yold.size();
			double ysum,esum;
			size_t hi_index=size_x-1;
			size_t wbins=size_y/n_bunch;
			size_t rem=size_y%n_bunch;


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
		* @param xold :: old x array of data
		* @param xnew :: new x array of data
		* @param yold :: old y array of data
		* @param ynew :: new y array of data
		* @param eold :: old error array of data
		* @param enew :: new error array of data
		* @param n_bunch :: number of data points to bunch together for each new point
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




