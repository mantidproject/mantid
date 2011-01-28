/***************************************************************************
    File                 : Fit.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Fit base class

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef FIT_H
#define FIT_H

#include <QObject>

#include "ApplicationWindow.h"
#include "Filter.h"

#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>

class Table;
class Matrix;

//! Fit base class
class Fit : public Filter
{
	Q_OBJECT

	public:

		typedef double (*fit_function_simplex)(const gsl_vector *, void *);
		typedef int (*fit_function)(const gsl_vector *, void *, gsl_vector *);
		typedef int (*fit_function_df)(const gsl_vector *, void *, gsl_matrix *);
		typedef int (*fit_function_fdf)(const gsl_vector *, void *, gsl_vector *, gsl_matrix *);

		enum Algorithm{ScaledLevenbergMarquardt, UnscaledLevenbergMarquardt, NelderMeadSimplex};
		enum WeightingMethod{NoWeighting, Instrumental, Statistical, Dataset};
        enum FitType{BuiltIn = 0, Plugin = 1, User = 2};

		Fit(ApplicationWindow *parent, Graph *g = 0, const QString& name = QString());
		Fit(ApplicationWindow *parent, Table *t, const QString& name = QString());
		~Fit();

		//! Actually does the fit. Should be reimplemented in derived classes.
		virtual void fit();
        virtual bool run(){fit(); return true;};

		//! Sets the data set to be used for weighting
		bool setWeightingData(WeightingMethod w, const QString& colName = QString::null);

		void setDataCurve(int curve, double start, double end);
		bool setDataFromTable(Table *t, const QString& xColName, const QString& yColName, int from = 1, int to = -1);

		QString resultFormula(){return d_result_formula;};
		QString formula(){return d_formula;};
		virtual void setFormula(const QString&){};

		int numParameters(){return d_p;};
		QStringList parameterNames(){return d_param_names;};
		virtual void setParametersList(const QStringList&){};
        void setParameterExplanations(const QStringList& lst){d_param_explain = lst;};

        double initialGuess(int parIndex){return gsl_vector_get(d_param_init, parIndex);};
		void setInitialGuess(int parIndex, double val){gsl_vector_set(d_param_init, parIndex, val);};
		void setInitialGuesses(double *x_init);

		virtual void guessInitialValues(){};

		void setParameterRange(int parIndex, double left, double right);
		void setAlgorithm(Algorithm s){d_solver = s;};

		//! Specifies weather the result of the fit is a function curve
		void generateFunction(bool yes, int points = 100);

		//! Output string added to the plot as a new legend
		virtual QString legendInfo();

		//! Returns a vector with the fit results
		double* results(){return d_results;};

		//! Returns a vector with the standard deviations of the results
		double* errors();

		//! Returns the sum of squares of the residuals from the best-fit line
		double chiSquare() {return chi_2;};

		//! Returns R^2
		double rSquare();

		//! Specifies wheather the errors must be scaled with sqrt(chi_2/dof)
		void scaleErrors(bool yes = true){d_scale_errors = yes;};

		Table* parametersTable(const QString& tableName);
		void writeParametersToTable(Table *t, bool append = false);

		Matrix* covarianceMatrix(const QString& matrixName);

        bool save(const QString& fileName);
        bool load(const QString& fileName);

        FitType type(){return d_fit_type;};
        void setType(FitType t){d_fit_type = t;};

        QString fileName(){return d_file_name;};
		void setFileName(const QString& fn){d_file_name = fn;};

        //! Frees the memory allocated for the X and Y data sets
        void freeMemory();

        //! Calculates the data for the output fit curve
        virtual double eval(double *, double){return 0.0;};

	private:
		void init();

		//! Pointer to the GSL multifit minimizer (for simplex algorithm)
		gsl_multimin_fminimizer * fitSimplex(gsl_multimin_function f, int &iterations, int &status);

		//! Pointer to the GSL multifit solver
		gsl_multifit_fdfsolver * fitGSL(gsl_multifit_function_fdf f, int &iterations, int &status);

		//! Customs and stores the fit results according to the derived class specifications. Used by exponential fits.
		virtual void customizeFitResults(){};

	protected:
		//! Allocates the memory for the fit workspace
		void initWorkspace(int par);
		//! Frees the memory allocated for the fit workspace
		void freeWorkspace();
		//! Adds the result curve as a FunctionCurve to the plot, if d_gen_function = true
		void insertFitFunctionCurve(const QString& name, double *x, double *y, int penWidth = 1);

		//! Adds the result curve to the plot
		virtual void generateFitCurve();

        //! Calculates the data for the output fit curve and store itin the X an Y vectors
		virtual void calculateFitCurveData(double *X, double *Y) {Q_UNUSED(X) Q_UNUSED(Y)};

		//! Output string added to the result log
		virtual QString logFitInfo(int iterations, int status);

		fit_function d_f;
		fit_function_df d_df;
		fit_function_fdf d_fdf;
		fit_function_simplex d_fsimplex;

		//! Number of fit parameters
		int d_p;

		//! Initial guesses for the fit parameters
		gsl_vector *d_param_init;

		/** \brief Tells whether the fitter uses non-linear/simplex fitting
		 * with an initial parameters set, that must be freed in the destructor.
		 */
		bool is_non_linear;

		//! weighting data set used for the fit
		double *d_w;

		//! Names of the fit parameters
		QStringList d_param_names;

		//! Stores a list of short explanations for the significance of the fit parameters
		QStringList d_param_explain;

		//! Specifies weather the result curve is a FunctionCurve or a normal curve with the same x values as the fit data
		bool d_gen_function;

		//! Algorithm type
		Algorithm d_solver;

		//! The fit formula given on input
		QString d_formula;

		//! The result fit formula, where the fit parameters are replaced with the calculated values.
		QString d_result_formula;

		//! Covariance matrix
		gsl_matrix *covar;

		//! The kind of weighting to be performed on the data
		WeightingMethod d_weighting;

		//! The name of the weighting dataset
		QString weighting_dataset;

		//! Stores the result parameters
		double *d_results;

		//! Stores standard deviations of the result parameters
		double *d_errors;

		//! The sum of squares of the residuals from the best-fit line
		double chi_2;

		//! Specifies wheather the errors must be scaled with sqrt(chi_2/dof)
		bool d_scale_errors;

		//! Table window used for the output of fit parameters
		Table *d_param_table;

		//! Matrix window used for the output of covariance matrix
		Matrix *d_cov_matrix;

		FitType d_fit_type;

		//! Path of the XML file where the user stores the fit model
        QString d_file_name;

		//! Stores the left limits of the research interval for the result parameters
		double *d_param_range_left;

		//! Stores the right limits of the research interval for the result parameters
		double *d_param_range_right;
};

#endif
