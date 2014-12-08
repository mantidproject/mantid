#ifndef MANTID_CURVEFITTING_FITMW_H_
#define MANTID_CURVEFITTING_FITMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IDomainCreator.h"
#include "MantidKernel/cow_ptr.h"

#include <list>

namespace Mantid
{
  namespace API
  {
    class FunctionDomain;
    class FunctionDomain1D;
    class FunctionValues;
    class MatrixWorkspace;
  }

  namespace CurveFitting
  {
    /**
    Creates FunctionDomain1D form a spectrum in a MatrixWorkspace.
    Declares WorkspaceIndex, StartX, and EndX input properties.
    Declares OutputWorkspace output property.

    @author Roman Tolchenov, Tessella plc
    @date 06/12/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport FitMW : public API::IDomainCreator
    {
    public:
      /// Constructor
      FitMW(Kernel::IPropertyManager* fit,
        const std::string& workspacePropertyName, 
        DomainType domainType = Simple);
      /// Constructor
      FitMW(DomainType domainType = Simple);
      /// declare properties that specify the dataset within the workspace to fit to.
      virtual void declareDatasetProperties(const std::string& suffix = "",bool addProp = true);
      /// Create a domain from the input workspace
      virtual void createDomain(
        boost::shared_ptr<API::FunctionDomain>& domain, 
        boost::shared_ptr<API::FunctionValues>& values, size_t i0 = 0);
      boost::shared_ptr<API::Workspace> createOutputWorkspace(
        const std::string& baseName,
        API::IFunction_sptr function,
        boost::shared_ptr<API::FunctionDomain> domain,
        boost::shared_ptr<API::FunctionValues> values,
        const std::string& outputWorkspacePropertyName
        );
      /// Return the size of the domain to be created.
      virtual size_t getDomainSize() const;
      /// Initialize the function
      virtual void initFunction(API::IFunction_sptr function);
      /// Set the workspace
      /// @param ws :: workspace to set.
      void setWorkspace(boost::shared_ptr<API::MatrixWorkspace> ws) {m_matrixWorkspace = ws;} 
      /// Set the workspace index
      /// @param wi :: workspace index to set.
      void setWorkspaceIndex(size_t wi) {m_workspaceIndex = wi;} 
      /// Set the startX and endX
      /// @param startX :: Start of the domain
      /// @param endX :: End of the domain
      void setRange(double startX, double endX){m_startX = startX; m_endX = endX;}
      /// Set max size for Sequantial and Parallel domains
      /// @param maxSize :: Maximum size of each simple domain
      void setMaxSize(size_t maxSize){m_maxSize = maxSize;}
      /// Set the normalisation flag
      /// @param on :: If true and the spectrum is a histogram the fitting data will be normalised 
      /// by the bin width.
      void setNormalise(bool on){m_normalise = on;}
    protected:
      /// Calculate size and starting iterator in the X array
      void getStartIterator(const Mantid::MantidVec& X, Mantid::MantidVec::const_iterator& from, size_t& n, bool isHisto) const;
      /// Set all parameters
      void setParameters()const;

    private:
      // Unrolls function into its constituent parts if it is a composite and adds it to the list. Note this is recursive
      void appendCompositeFunctionMembers(std::list<API::IFunction_sptr> & functionList, const API::IFunction_sptr & function) const;
      // Create separate Convolutions for each component of the model of a convolution
      void appendConvolvedCompositeFunctionMembers(std::list<API::IFunction_sptr> & functionList, const API::IFunction_sptr & function) const;
      /// Creates the blank output workspace of the correct size
      boost::shared_ptr<API::MatrixWorkspace> createEmptyResultWS(const size_t nhistograms, const size_t nyvalues);
      /// Add the calculated function values to the workspace
      void addFunctionValuesToWS(const API::IFunction_sptr & function, boost::shared_ptr<API::MatrixWorkspace> & ws,
        const size_t wsIndex, const boost::shared_ptr<API::FunctionDomain> & domain, boost::shared_ptr<API::FunctionValues> resultValues) const;

      /// Store workspace property name
      std::string m_workspacePropertyName;
      /// Store workspace index property name
      std::string m_workspaceIndexPropertyName;
      /// Store startX property name
      std::string m_startXPropertyName;
      /// Store endX property name
      std::string m_endXPropertyName;
      /// Store maxSize property name
      std::string m_maxSizePropertyName;
      /// Store Normalise property name
      std::string m_normalisePropertyName;

      /// The input MareixWorkspace
      mutable boost::shared_ptr<API::MatrixWorkspace> m_matrixWorkspace;
      /// The workspace index
      mutable size_t m_workspaceIndex;
      /// startX
      mutable double m_startX;
      /// endX
      mutable double m_endX;
      /// Max size for seq domain
      mutable size_t m_maxSize;
      /// Option to normalise the data
      mutable bool m_normalise;
      size_t m_startIndex;
    };

    
  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FITMW_H_*/
