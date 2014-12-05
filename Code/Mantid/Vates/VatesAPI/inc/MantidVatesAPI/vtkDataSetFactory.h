

#ifndef MANTID_VATES_VTKDATASETFACTORY_H_
#define MANTID_VATES_VTKDATASETFACTORY_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/System.h"
#include "vtkDataSet.h"
#include <boost/shared_ptr.hpp>
#include <string>

class vtkFloatArray;
namespace Mantid
{
  namespace API
  {
    class IMDWorkspace;
  }

namespace VATES
{

  //Forward declaration
  class ProgressAction;

  /* Helper struct allows recognition of points that we should not bother to draw.
  */
  struct UnstructuredPoint
  {
    bool isSparse;
    vtkIdType pointId;
  };

/** Abstract type to generate a vtk dataset on demand from a MDWorkspace.
 Uses Chain Of Responsibility pattern to self-manage and ensure that the workspace rendering is delegated to another factory
 if the present concrete type can't handle it.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class DLLExport vtkDataSetFactory
{

public:

  /// Constructor
  vtkDataSetFactory();

  /// Destructor
  virtual ~vtkDataSetFactory()=0;

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkDataSet* create(ProgressAction&) const=0;

  /// Initalize with a target workspace.
  virtual void initialize(Mantid::API::Workspace_sptr)=0;

  /// Create the product in one step.
  virtual vtkDataSet* oneStepCreate(Mantid::API::Workspace_sptr,ProgressAction&);

  /// Add a chain-of-responsibility successor to this factory. Handle case where the factory cannot render the MDWorkspace owing to its dimensionality.
  virtual void SetSuccessor(vtkDataSetFactory* pSuccessor);

  /// Determine whether a successor factory has been provided.
  virtual bool hasSuccessor() const;

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const =0;

  virtual void setRecursionDepth(size_t) //TODO subtype vtkDataSet factory specifically for MDWEW type workspaces and put this method on that subtype.
  {
    throw std::runtime_error("vtkDataSetFactory does not implement ::setRecursionDepth"); 
  }

  /// Setter for whether a workspace defined transformation should be used or not.
  virtual void setUseTransform(bool bUseTransform)
  {
    m_useTransform = bUseTransform;
  }

  /// Getter for the use transform status.
  virtual bool getUseTransform() const
  {
    return m_useTransform;
  }

  /// Setter to indicate that dimensionality should/should not be checked.
  void setCheckDimensionality(bool flag);

  /// Getter for the state of the dimensionality checking.
  bool doesCheckDimensionality() const;

  /// Dimensionalities of interest.
  enum Dimensionality{OneDimensional=1, TwoDimensional=2, ThreeDimensional=3, FourDimensional=4};

protected:
  
  /**
  Try to cast it to the specified IMDType and then run checks based on the non-integrated dimensionality. 
  The latter checks are only run if the factory is set to apply these checks.
  @param  workspace : workspace to cast.
  @param  bExactMatch : run an exact match on non-integarated dimensionality if TRUE, otherwise is less than or equal to ExpectedDimensions.
  @return  correctly cast shared pointer or an empty shared pointer if cast or checks fail.
  */
  template<typename IMDWorkspaceType, size_t ExpectedNDimensions>
  boost::shared_ptr<IMDWorkspaceType> castAndCheck(Mantid::API::Workspace_sptr workspace, bool bExactMatch=true) const
  {
    boost::shared_ptr<IMDWorkspaceType> temp;
    boost::shared_ptr<IMDWorkspaceType> imdws = boost::dynamic_pointer_cast<IMDWorkspaceType>(workspace);
    if(!imdws)
    {
      //Abort as imdws cannot be dynamically cast to the target type.
      return temp;
    }
    bool bPassesDimensionalityCheck = false;
    size_t actualNonIntegratedDimensionality = imdws->getNonIntegratedDimensions().size();
    if(bExactMatch)
    {
      bPassesDimensionalityCheck = (ExpectedNDimensions == actualNonIntegratedDimensionality);
    }
    else
    {
      bPassesDimensionalityCheck = (actualNonIntegratedDimensionality >= ExpectedNDimensions);
    }
    if(this->doesCheckDimensionality() &&  !bPassesDimensionalityCheck)
    {
      //Abort as there are dimensionality checks to be applied and these checks fail.
      return temp;
    }
    return imdws;
  }

  /**
  Common initialization implementation. Most vtkDataSets will need this in order to correctly delegate initialization onto successors.
  @param workspace : workspace to cast.
  @param bExactMatch : run an exact match on non-integarated dimensionality if TRUE, otherwise is less than or equal to ExpectedDimensions.
  @return correctly cast shared pointer or an empty shared pointer if cast or checks fail.
  */
  template<typename IMDWorkspaceType, size_t ExpectedNDimensions>
  boost::shared_ptr<IMDWorkspaceType> doInitialize(Mantid::API::Workspace_sptr workspace, bool bExactMatch=true) const
  {
    if(workspace == NULL)
    {
      std::string message = this->getFactoryTypeName() + " initialize cannot operate on a null workspace";
      throw std::invalid_argument(message);
    }
    boost::shared_ptr<IMDWorkspaceType> imdws = castAndCheck<IMDWorkspaceType, ExpectedNDimensions>(workspace, bExactMatch);
    if(!imdws)
    {
      if(this->hasSuccessor())
      {
        m_successor->setUseTransform(m_useTransform);
        m_successor->initialize(workspace);
      }
      else
      {
        std::string message = this->getFactoryTypeName() + " has no successor";
        throw std::runtime_error(message);
      }
    }
    return imdws;
  }

  /**
  Common creation implementation whereby delegation to successor is attempted if appropriate. 
  @param workspace : workspace to cast and create from.
  @param progressUpdate : object used to pass progress information back up the stack.
  @param bExactMatch : Check for an exact match if true.
  @return TRUE if delegation to successors has occured. Otherwise returns false.
  */
  template<typename IMDWorkspaceType, size_t ExpectedNDimensions>
  vtkDataSet* tryDelegatingCreation(Mantid::API::Workspace_sptr workspace, ProgressAction& progressUpdate, bool bExactMatch=true) const
  {
    boost::shared_ptr<IMDWorkspaceType> imdws = castAndCheck<IMDWorkspaceType, ExpectedNDimensions>(workspace, bExactMatch);
    if(!imdws)
    {
      if(this->hasSuccessor())
      {
        return m_successor->create(progressUpdate);
      }
      else
      {
        std::string message = this->getFactoryTypeName() + " has no successor";
        throw std::runtime_error(message);
      }
    }
    return NULL;
  }

  /// Typedef for internal unique shared pointer for successor types.
  typedef boost::shared_ptr<vtkDataSetFactory> SuccessorType;

  vtkDataSetFactory::SuccessorType m_successor;

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const = 0;

  /// Flag indicating whether a transformation should be used.
  bool m_useTransform;

private:
  
  /// Dimensionality checking flag
  bool m_bCheckDimensionality;
};

typedef boost::shared_ptr<vtkDataSetFactory> vtkDataSetFactory_sptr;

}
}


#endif
