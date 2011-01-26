#ifndef MANTID_ALGORITHMS_BINARYOPERATION_H_
#define MANTID_ALGORITHMS_BINARYOPERATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/PairedGroupAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"

namespace Mantid
{
  namespace Algorithms
  {

    enum OperandType
    {
      eEventList = 0,
      eHistogram = 1,
      eNumber = 2
    };

//    struct OpRequirements
//    {
//      bool matchXSize;
//      bool keepEventWorkspace;
//    };


    /** 
    BinaryOperation supports the implementation of a binary operation on two input workspaces.
    It inherits from the Algorithm class, and overrides the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> InputWorkspace1 - The name of the workspace forming the left hand operand</LI>
    <LI> InputWorkspace2 - The name of the workspace forming the right hand operand </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    @author Nick Draper
    @date 14/12/2007

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    */
    class DLLExport BinaryOperation : public API::PairedGroupAlgorithm
    {
    public:
      /// Default constructor
      BinaryOperation();
      /// Destructor
      virtual ~BinaryOperation();

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Arithmetic";}


      /** BinaryOperationTable: a list of ints.
       * Index into vector: workspace index in the lhs;
       * Value at that index: workspace index of the rhs to apply to the WI in the lhs. -1 if not found.
       */
      typedef std::vector< int >  BinaryOperationTable;

      static BinaryOperationTable * buildBinaryOperationTable(API::MatrixWorkspace_const_sptr lhs, API::MatrixWorkspace_const_sptr rhs);


    //protected:
    private:
      // Overridden Algorithm methods
      void init();
      void exec();

    protected:
      /// Execution method for event workspaces, to be overridden as needed.
      virtual void execEvent( DataObjects::EventWorkspace_const_sptr lhs, DataObjects::EventWorkspace_const_sptr rhs );

      /// The name of the first input workspace property
      virtual std::string inputPropName1() const { return "LHSWorkspace";}
      /// The name of the second input workspace property
      virtual std::string inputPropName2() const { return "RHSWorkspace";}
      /// The name of the output workspace property
      virtual std::string outputPropName() const { return "OutputWorkspace";}

      /// Checks the compatibility of the two workspaces
      virtual bool checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;

      /// Checks the compatibility of event-based processing of the two workspaces
      virtual bool checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs);

      /// Checks the overall size compatibility of two workspaces
      virtual bool checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const;
      
      /// Checks if the spectra at the given index of either input workspace is masked. If so then the output spectra has zeroed data
      /// and is also masked. The function returns true if further processing is not required on the spectra.
      virtual bool propagateSpectraMask(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs, 
					const int index, API::MatrixWorkspace_sptr out);

      /** Carries out the binary operation on a single spectrum, with another spectrum as the right-hand operand.
       *
       *  @param lhsX The X values, made available if required.
       *  @param lhsY The vector of lhs data values
       *  @param lhsE The vector of lhs error values
       *  @param rhsY The vector of rhs data values
       *  @param rhsE The vector of rhs error values
       *  @param YOut The vector to hold the data values resulting from the operation
       *  @param EOut The vector to hold the error values resulting from the operation
       */
      virtual void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut) = 0;

      /** Carries out the binary operation when the right hand operand is a single number.
       *
       *  @param lhsX The X values, made available if required.
       *  @param lhsY The vector of lhs data values
       *  @param lhsE The vector of lhs error values
       *  @param rhsY The rhs data value
       *  @param rhsE The rhs error value
       *  @param YOut The vector to hold the data values resulting from the operation
       *  @param EOut The vector to hold the error values resulting from the operation
       */
      virtual void performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                          const double& rhsY, const double& rhsE, MantidVec& YOut, MantidVec& EOut) = 0;


      // ===================================== EVENT LIST BINARY OPERATIONS ==========================================

      /** Carries out the binary operation IN-PLACE on a single EventList,
       * with another EventList as the right-hand operand.
       * The event lists simply get appended.
       *
       *  @param lhs Reference to the EventList that will be modified in place.
       *  @param rhs Const reference to the EventList on the right hand side.
       */
      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const DataObjects::EventList & rhs);


      /** Carries out the binary operation IN-PLACE on a single EventList,
       * with another (histogrammed) spectrum as the right-hand operand.
       *
       *  @param lhs Reference to the EventList that will be modified in place.
       *  @param rhsX The vector of rhs X bin boundaries
       *  @param rhsY The vector of rhs data values
       *  @param rhsE The vector of rhs error values
       */
      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const MantidVec& rhsX, const MantidVec& rhsY, const MantidVec& rhsE);

      /** Carries out the binary operation IN-PLACE on a single EventList,
       * with a single (double) value as the right-hand operand
       *
       *  @param lhs Reference to the EventList that will be modified in place.
       *  @param rhsY The rhs data value
       *  @param rhsE The rhs error value
       */
      virtual void performEventBinaryOperation(DataObjects::EventList & lhs,
          const double& rhsY, const double& rhsE);




      /** Should be overridden by operations that need to manipulate the units of the output workspace.
       *  Does nothing by default.
       *  @param lhs The first input workspace
       *  @param rhs The second input workspace
       *  @param out The output workspace
       */
      virtual void setOutputUnits(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
      {
        (void) lhs; //Avoid compiler warning
        (void) rhs;
        (void) out;
      }

      /** Only overridden by operations that affect the properties of the run (e.g. Plus
       *  where the proton currents (charges) are added). Otherwise it does nothing.
       *  @param lhs one of the workspaces to operate on
       *  @param rhs the other workspace
       *  @param ans the output workspace
       */

      virtual void operateOnRun(const API::Run& lhs, const API::Run& rhs, API::Run& ans) const
      {
        (void) lhs; //Avoid compiler warning
        (void) rhs;
        (void) ans;
      };


      OperandType getOperandType(const API::MatrixWorkspace_const_sptr ws);

      virtual void checkRequirements();

      // ------- Workspaces being worked on --------
      /// Left-hand side workspace
      API::MatrixWorkspace_const_sptr m_lhs;
      /// Left-hand side EventWorkspace
      DataObjects::EventWorkspace_const_sptr m_elhs;

      /// Right-hand side workspace
      API::MatrixWorkspace_const_sptr m_rhs;
      /// Right-hand side EventWorkspace
      DataObjects::EventWorkspace_const_sptr m_erhs;

      /// Output workspace
      API::MatrixWorkspace_sptr m_out;
      /// Output EventWorkspace
      DataObjects::EventWorkspace_sptr m_eout;

      /// The property value
      bool m_AllowDifferentNumberSpectra;

      //------ Requirements -----------

      /// matchXSize set to true if the X sizes of histograms must match.
      bool m_matchXSize;

      /// flipSides set to true if the rhs and lhs operands should be flipped - for commutative binary operations, normally.
      bool m_flipSides;

      /// Variable set to true if the operation allows the output to stay as an EventWorkspace. If this returns false, any EventWorkspace will be converted to Workspace2D. This is ignored if the lhs operand is not an EventWorkspace.
      bool m_keepEventWorkspace;

      /** Are we going to use the histogram representation of the RHS event list when performing the operation?
       * e.g. divide and multiply? Plus and Minus will set this to false (default).
       */
      bool m_useHistogramForRhsEventWorkspace;

    private:

      void doSingleValue();
      void doSingleSpectrum();
      void doSingleColumn();
      void do2D(bool mismatchedSpectra);

      void propagateBinMasks(const API::MatrixWorkspace_const_sptr rhs, API::MatrixWorkspace_sptr out);
      /// Apply masking requested by propagateSpectraMasks.
      void applyMaskingToOutput(API::MatrixWorkspace_sptr out);


      /// A store for accumulated spectra that should be masked in the output workspace
      std::vector<int> m_indicesToMask;
      /// Progress reporting
      API::Progress* m_progress;  
    };

  } // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_BINARYOPERATION_H_*/
