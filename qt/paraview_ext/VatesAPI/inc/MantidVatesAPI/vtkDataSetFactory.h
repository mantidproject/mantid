// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_VATES_VTKDATASETFACTORY_H_
#define MANTID_VATES_VTKDATASETFACTORY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/Chainable.h"
#include "MantidKernel/System.h"

#include "vtkDataSet.h"
#include "vtkSmartPointer.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>

class vtkFloatArray;
namespace Mantid {

namespace VATES {

// Forward declaration
class ProgressAction;

/* Helper struct allows recognition of points that we should not bother to draw.
 */
struct UnstructuredPoint {
  bool isSparse;
  vtkIdType pointId;
};

/** Abstract type to generate a vtk dataset on demand from a MDWorkspace.
 Uses Chain Of Responsibility pattern to self-manage and ensure that the
 workspace rendering is delegated to another factory
 if the present concrete type can't handle it.

 @author Owen Arnold, Tessella plc
 @date 24/01/2010
 */

class DLLExport vtkDataSetFactory
    : public Mantid::Kernel::Chainable<vtkDataSetFactory> {

public:
  /// Constructor
  vtkDataSetFactory();

  /// Factory Method. Should also handle delegation to successors.
  virtual vtkSmartPointer<vtkDataSet> create(ProgressAction &) const = 0;

  /// Initalize with a target workspace.
  virtual void initialize(const Mantid::API::Workspace_sptr &workspace) = 0;

  /// Create the product in one step.
  virtual vtkSmartPointer<vtkDataSet> oneStepCreate(Mantid::API::Workspace_sptr,
                                                    ProgressAction &);

  /// Get the name of the type.
  virtual std::string getFactoryTypeName() const = 0;

  virtual void setRecursionDepth(size_t) // TODO subtype vtkDataSet factory
                                         // specifically for MDWEW type
                                         // workspaces and put this method on
                                         // that subtype.
  {
    throw std::runtime_error(
        "vtkDataSetFactory does not implement ::setRecursionDepth");
  }

  /// Setter for whether a workspace defined transformation should be used or
  /// not.
  virtual void setUseTransform(bool bUseTransform) {
    m_useTransform = bUseTransform;
  }

  /// Getter for the use transform status.
  virtual bool getUseTransform() const { return m_useTransform; }

  /// Setter to indicate that dimensionality should/should not be checked.
  void setCheckDimensionality(bool flag);

  /// Getter for the state of the dimensionality checking.
  bool doesCheckDimensionality() const;

  /// Dimensionalities of interest.
  enum Dimensionality {
    OneDimensional = 1,
    TwoDimensional = 2,
    ThreeDimensional = 3,
    FourDimensional = 4
  };

  static const std::string ScalarName;

protected:
  /**
   Run checks based on the non-integrated dimensionality, which are only run
   if the factory is set to apply these checks.
   @param  imdws : workspace to check.
   @param  bExactMatch : run an exact match on non-integarated dimensionality if
   TRUE, otherwise is less than or equal to ExpectedDimensions.
   @return whether the checks pass or fail.
   */
  template <typename IMDWorkspaceType, size_t ExpectedNDimensions>
  bool checkWorkspace(const IMDWorkspaceType &imdws,
                      bool bExactMatch = true) const {
    bool bPassesDimensionalityCheck = false;
    size_t actualNonIntegratedDimensionality =
        imdws.getNonIntegratedDimensions().size();
    if (bExactMatch) {
      bPassesDimensionalityCheck =
          (ExpectedNDimensions == actualNonIntegratedDimensionality);
    } else {
      bPassesDimensionalityCheck =
          (actualNonIntegratedDimensionality >= ExpectedNDimensions);
    }
    if (this->doesCheckDimensionality() && !bPassesDimensionalityCheck) {
      // Abort as there are dimensionality checks to be applied and these checks
      // fail.
      return false;
    }
    return true;
  }

  /**
   Try to cast it to the specified IMDType and then run checks based on the
   non-integrated dimensionality.
   The latter checks are only run if the factory is set to apply these checks.
   @param  workspace : workspace to cast.
   @param  bExactMatch : run an exact match on non-integarated dimensionality if
   TRUE, otherwise is less than or equal to ExpectedDimensions.
   @return  correctly cast shared pointer or an empty shared pointer if cast or
   checks fail.
   */
  template <typename IMDWorkspaceType, size_t ExpectedNDimensions>
  boost::shared_ptr<IMDWorkspaceType>
  castAndCheck(Mantid::API::Workspace_sptr workspace,
               bool bExactMatch = true) const {
    boost::shared_ptr<IMDWorkspaceType> imdws =
        boost::dynamic_pointer_cast<IMDWorkspaceType>(workspace);
    if (imdws && this->checkWorkspace<IMDWorkspaceType, ExpectedNDimensions>(
                     *imdws, bExactMatch)) {
      return imdws;
    } else {
      // Abort as imdws cannot be dynamically cast to the target type.
      return nullptr;
    }
  }

  /**
  Common initialization implementation. Most vtkDataSets will need this in
  order
  to correctly delegate initialization onto successors.
  @param workspace : workspace to cast.
  @param bExactMatch : run an exact match on non-integarated dimensionality if
  TRUE, otherwise is less than or equal to ExpectedDimensions.
  @return correctly cast shared pointer or an empty shared pointer if cast or
  checks fail.
  */
  template <typename IMDWorkspaceType, size_t ExpectedNDimensions>
  boost::shared_ptr<IMDWorkspaceType>
  doInitialize(Mantid::API::Workspace_sptr workspace,
               bool bExactMatch = true) const {
    if (!workspace) {
      std::string message = this->getFactoryTypeName() +
                            " initialize cannot operate on a null workspace";
      throw std::invalid_argument(message);
    }
    boost::shared_ptr<IMDWorkspaceType> imdws =
        castAndCheck<IMDWorkspaceType, ExpectedNDimensions>(workspace,
                                                            bExactMatch);
    if (!imdws) {
      if (this->hasSuccessor()) {
        m_successor->setUseTransform(m_useTransform);
        m_successor->initialize(workspace);
      } else {
        std::string message = this->getFactoryTypeName() + " has no successor";
        throw std::runtime_error(message);
      }
    }
    return imdws;
  }

  /**
  Common creation implementation whereby delegation to successor is attempted
  if
  appropriate.
  @param workspace : workspace to cast and create from.
  @param progressUpdate : object used to pass progress information back up the
  stack.
  @param bExactMatch : Check for an exact match if true.
  @return TRUE if delegation to successors has occured. Otherwise returns
  false.
  */
  template <typename IMDWorkspaceType, size_t ExpectedNDimensions>
  vtkSmartPointer<vtkDataSet>
  tryDelegatingCreation(Mantid::API::Workspace_sptr workspace,
                        ProgressAction &progressUpdate,
                        bool bExactMatch = true) const {
    boost::shared_ptr<IMDWorkspaceType> imdws =
        castAndCheck<IMDWorkspaceType, ExpectedNDimensions>(workspace,
                                                            bExactMatch);
    if (!imdws) {
      if (this->hasSuccessor()) {
        return m_successor->create(progressUpdate);
      } else {
        std::string message = this->getFactoryTypeName() + " has no successor";
        throw std::runtime_error(message);
      }
    }
    return nullptr;
  }

  /// Template Method pattern to validate the factory before use.
  virtual void validate() const = 0;

  /// Checks successor when set and throws if bad
  void checkSuccessor() const override;

  /// Flag indicating whether a transformation should be used.
  bool m_useTransform;

private:
  /// Dimensionality checking flag
  bool m_bCheckDimensionality;
};

using vtkDataSetFactory_sptr = boost::shared_ptr<vtkDataSetFactory>;
using vtkDataSetFactory_uptr = std::unique_ptr<vtkDataSetFactory>;
} // namespace VATES
} // namespace Mantid

#endif
