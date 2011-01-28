#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <complex>
#include <cmath>
#include <vector>
#include <list>
#include <map>
#include <stack>
#include <algorithm>
#include <iterator>

#include "MantidKernel/Logger.h"

#include "MantidGeometry/Math/Triple.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Objects/Rules.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Rule::PLog(Kernel::Logger::get("Rule"));

int
Rule::addToKey(std::vector<int>& AV,const int passN)
  /** 
    Static function ::
    Given a vector AV increase the number from lowest
    to highest in an iterative counting chain.
    (effectively an N-bit binary number++)
    @param AV :: vector of the N-bit array
    @param passN :: number that is to be skipped 
    @retval bit number that was set from 0 to 1
    @retval -1 if the a carry flag is set 
  */
{
  for(int i=0;i<static_cast<int>(AV.size());i++)
    {
      if (passN!=i)
        {
	  if (AV[i]==1)
	    AV[i]=0;
	  else
	    return i+1;
	}
    }
  return -1;
}

int
Rule::removeComplementary(Rule*& TopRule)
  /**
    Given a rule tree remove any parts that 
    are (-A B C D A) -> (B C D) and 
    (A B C D A ) -> (A B C D) 
    Code is borrowed from makeCNF. See comments
    within that for a better howto.
    Currently it passes the line down
    @retval 1 :: some simplification
    @retval 0 :: no simplification
    
  */
{
  // Search down the rule until we get to common
  // group. Once we have found a common type
  // apply simple
 
  if (TopRule->type()==0)     // One element tree (just return)
    return 0; 

  int active(1);             // active units

  Rule *tmpA,*tmpB,*tmpC;       // need three to play
  DTriple<Rule*,int,Rule*> TreeComp;     // Storage for top of stack

  while(active)
    {
      active=0;
      std::stack<DTriple<Rule*,int,Rule*> > TreeLine;   // Tree stack of rules
      TreeLine.push(DTriple<Rule*,int,Rule*>(0,0,TopRule));

      while (!active && !TreeLine.empty())        // need to exit on active
        {
	  TreeComp=TreeLine.top();
	  TreeLine.pop();	  
	  // get first rule off tree
	  tmpA=TreeComp.third;

	  if (!tmpA->commonType())          // Not a common type, then we must get branches 
	    {
	      tmpB=tmpA->leaf(0);        // get leaves (two of) 
	      tmpC=tmpA->leaf(1);
	      if (tmpB)
		TreeLine.push(DTriple<Rule*,int,Rule*>(tmpA,0,tmpB));
	      if (tmpC)
		TreeLine.push(DTriple<Rule*,int,Rule*>(tmpA,1,tmpC));
	    }
	  else    // Got something to simplify :-)
            {
	      // In the simplification we don't need 
	      // to add anything to the tree. 
	      // This call the appropiate intersection/Union
	      // TrueCount 1 is a simple simplification
	      //           -1 alway true
              //           -2 alway false 
	      const int tcount(tmpA->simplify());  
              
	      if (tcount==1)       // Deep simplification
	        {
		  active=1;
		}
	      else if (tcount== -1)  //replacement simplificationnn
	        {
		  if (TreeComp.first)
		    {
		      TreeComp.first=tmpA->leaf(0);
		      tmpA->setLeaf(0,0);
		      delete tmpA;
		    }
		}
	    }
	}
    }
  return 1;
}

int
Rule::makeCNFcopy(Rule* &TopRule)
  /**
    Static Function::

    Function to make the tree into CNF
    (conditional normal form) 
    @param TopRule :: Rule to simplify.

    We do not use *this :: The reason is that 
    we need to change the toprule type eg from 
    an intersection to a union.
    Since the type of a class can't be changed
    it is necessary to have a static function,
    so that the place that TopRule points is changable
    @return number of changes 
  */
{
  // Start at top to tree to find an instance
  // an intersection with stuff below.
  
  int count(0);                 // Number of changes made
  int active(1);                // Do we still have to use 
  Rule *tmpA,*tmpB,*tmpC;       // need three to play
  DTriple<Rule*,int,Rule*> TreeComp;     // Storage for top of stack
  
  /*
    The process works by below value deletion:
    This is when the tree is modified at a particular
    point, each memeber of the tree below the modification
    point is deleted and the modification point is
    replaced with a new version (normally a cloned copy).
    This is fine for small items (eg here the memory footprint
    is only 24 bytes, but not acceptable for bigger items.
    The two layer insertion (which could be done) is more
    complex but better proformance is possible.
  */

  while(active)
    {
      active=0;
      count++;
      // This stack takes a Different Triple < Parent, leaf, Child >
      // We need to store the parent so that when we change the tree
      // of a particular object. The integer records which side
      // of the branch the tree object came from 0 == LHS 1==RHS.

      std::stack<DTriple<Rule*,int,Rule*> > TreeLine;   // Tree stack of rules

      // Start by putting the top item on the Tree Stack.
      // Note that it doesn't have a parent.
      TreeLine.push(DTriple<Rule*,int,Rule*>(0,0,TopRule));   

      // Exit condition is that nothing changed last time 
      // or the tree is Empty. 

      while(!active && !TreeLine.empty())
        {
	  // Ok get and remvoe the top item from the stack.
	  TreeComp=TreeLine.top();  
	  TreeLine.pop();           

	  
	  // Get the item. (not its parent)
	  tmpA=TreeComp.third;       // get base item
	  // Now it might be a Union or Intersection 
	  // so we need to get is branches.
	  // If it is a surface item then it will not have
	  // branches and thus tmpB and tmpC will == 0
	  tmpB=tmpA->leaf(0);        // get leaves (two of) 
	  tmpC=tmpA->leaf(1);

	  // If either then we need to put copies on the stack
	  // for later consideration. 
	  // tmpA is the parent, since it is the leaves of tmpA the
	  // are written into tmpB and tmpC
	  if (tmpB)
	    TreeLine.push(DTriple<Rule*,int,Rule*>(tmpA,0,tmpB));
	  if (tmpC)
	    TreeLine.push(DTriple<Rule*,int,Rule*>(tmpA,1,tmpC));

	  // 
	  //  Time to see if we can apply rule 4 (propositional calculus)
	  //  to expand (a ^ b) v c to (a v c) ^ (b v c)
	  //
	  if (tmpA->type()==1 && tmpB && tmpC)            // it is an intersection otherwise no work to do
	    {
	      // require either the left or right to be unions.
	      if (tmpB->type()== -1 ||
		  tmpC->type()== -1)     // this is a union expand....
	        {
		  Rule* partReplace;               
		  Rule* alpha,*beta,*gamma;
		  if (tmpB->type()== -1)    // ok the LHS is a union. (a ^ b) v g ==> (a v g) ^ (b v g )
		    {
		      // Make copies of the Unions leaves (allowing for null union)
		      alpha= (tmpB->leaf(0)) ? tmpB->leaf(0)->clone() : 0;  
		      beta= (tmpB->leaf(1)) ? tmpB->leaf(1)->clone() : 0;    
		      gamma=  tmpC->clone(); 
		    } 
		  else     // RHS a v (b ^ g) ==> (a v b) ^ (a v g )
		    {
		      // Make copies of the Unions leaves (allowing for null union)
		      // Note :: alpha designated as beta , gamma plays the role of alpha
		      // in the RHS part of the above equation (allows common replace
                      // block below.
		      alpha= (tmpC->leaf(0)) ? tmpC->leaf(0)->clone() : 0;  
		      beta=  (tmpC->leaf(1)) ? tmpC->leaf(1)->clone() : 0;     
		      gamma=  tmpB->clone(); 
		    }
		  // Have bit to replace 

		  // Note:: no part of this can be memory copy
		  // hence we have to play games with a second
		  // gamma->clone()
		  partReplace=new Union(
		    new Intersection(alpha,gamma),
		    new Intersection(beta,gamma->clone()));
		  //
		  //General replacement
		  //
		  if (TreeComp.first)            // Not the top rule (so replace parents leaf)
		    TreeComp.first->setLeaf(partReplace,TreeComp.second);
		  else
		    // It is the top rule therefore, replace the toprule
		    TopRule=partReplace;  

		  // Clear up the mess and delete the rule that we have changes
		  delete tmpA;          
		  // Now we have to go back to the begining again and start again.
		  active=1;            // Exit loop
		}
	    }
	}
    }
  return count-1;  //return number of changes 
}

int
Rule::makeCNF(Rule* &TopRule)
  /**
    Static Function::

    Function to make the tree into CNF
    (conditional normal form) 
    @param TopRule :: Rule to simplify.

    We do not use *this :: The reason is that 
    we need to change the toprule type e.g. from 
    an intersection to a union.
    Since the type of a class can't be changed
    it is necessary to have a static function,
    so that the place that TopRule points is changable

    @retval 0 on failure
    @retval 1 on success
  */
{
  // Start at top to tree to find an instance
  // an intersection with stuff below.
  
  if (!TopRule)
    return 0;

  TopRule->makeParents();
  int count(0);                 // Number of changes made
  int active(1);                // Do we still have to use 
  Rule *tmpA,*tmpB,*tmpC;       // need three to play
  
  while(active)
    {
      active=0;
      count++;
      std::stack<Rule*> TreeLine;   // Tree stack of rules

      // Start by putting the top item on the Tree Stack.
      // Note that it doesn't have a parent.
      TreeLine.push(TopRule);   

      // Exit condition is that nothing changed last time 
      // or the tree is Empty. 
      if (!TopRule->checkParents())
	std::cerr<<"Parents False"<<std::endl;
      while(!active && !TreeLine.empty())
        {
	  // Ok get and remvoe the top item from the stack.
	  tmpA=TreeLine.top();  
	  TreeLine.pop();           

	  // Now it might be a Union or Intersection 
	  // so we need to get is branches.

	  tmpB=tmpA->leaf(0);        // get leaves (two of) 
	  tmpC=tmpA->leaf(1);

	  if (tmpB)
	    TreeLine.push(tmpB);
	  if (tmpC)
	    TreeLine.push(tmpC);

	  // 
	  //  Time to see if we can apply rule 4 (propositional calculus)
	  //  to expand (a ^ b) v c to (a v c) ^ (b v c)
	  //
	  if (tmpA->type()==1 && tmpB && tmpC)            // it is an intersection otherwise no work to do
	    {
	      // require either the left or right to be unions.
	      if (tmpB->type()== -1 ||
		  tmpC->type()== -1)     // this is a union expand....
	        {
		  Rule* partReplace;               
		  Rule* alpha,*beta,*gamma,*Uobj;        // Uobj to be deleted
		  if (tmpB->type()== -1)    // ok the LHS is a union. (a ^ b) v g ==> (a v g) ^ (b v g )
		    {
		      // Make copies of the Unions leaves (allowing for null union)
		      alpha= tmpB->leaf(0);
		      beta= tmpB->leaf(1);
		      gamma=  tmpC;
		      Uobj=tmpB;
		    } 
		  else     // RHS a v (b ^ g) ==> (a v b) ^ (a v g )
		    {
		      // Make copies of the Unions leaves (allowing for null union)
		      // Note :: alpha designated as beta , gamma plays the role of alpha
		      // in the RHS part of the above equation (allows common replace
                      // block below.
		      alpha= tmpC->leaf(0);
		      beta= tmpC->leaf(1);
		      gamma=  tmpB;
		      Uobj=tmpC;
		    }
		  // Have bit to replace 

		  // Note:: no part of this can be memory copy
		  // hence we have to play games with a second
		  // gamma->clone()
		  partReplace=new Union(
		    new Intersection(alpha,gamma),
		    new Intersection(beta,gamma->clone()));
		  //
		  //General replacement
		  //
		  Rule* Pnt=tmpA->getParent();                  // parent
		  if (Pnt)       // Not the top rule (so replace parents leaf)
		    {
		      const int leafN=Pnt->findLeaf(tmpA);     
		      Pnt->setLeaf(partReplace,leafN);
		    }
		  else
		    TopRule=partReplace;  

		  // Clear up the mess and delete the rule that we have changes
		  // Set the cutoffs in the correct place to avoid a complete
		  // tree deletion
		  tmpA->setLeaves(0,0);
		  delete tmpA;          
		  Uobj->setLeaves(0,0);
		  delete Uobj;          
		  // Now we have to go back to the begining again and start again.
		  active=1;            // Exit loop
		}
	    }
	}
    }
  return count-1;  //return number of changes 
}

int
Rule::removeItem(Rule* &TRule,const int SurfN) 
  /**
    Given an item as a surface name,
    remove the surface from the Rule tree.
    - If the found leaf is on a
    @param TRule :: Top rule to down search
    @param SurfN :: Surface key number to remove
    @return Number of instances removed
  */
{
  int cnt(0);
  Rule* Ptr = TRule->findKey(SurfN);
  while (Ptr)
  {
    Rule* LevelOne = Ptr->getParent(); // Must work
    Rule* LevelTwo = (LevelOne) ? LevelOne->getParent() : 0;

    if (LevelTwo) /// Not the top level
    {
      // Decide which of the pairs is to be copied
      Rule* PObj = (LevelOne->leaf(0) != Ptr) ? LevelOne->leaf(0) : LevelOne->leaf(1);
      // 
      LevelOne->setLeaves(0, 0); // Delete from Ptr, and copy
      const int side = (LevelTwo->leaf(0) != LevelOne) ? 1 : 0;
      LevelTwo->setLeaf(PObj, side);
    }
    else if (LevelOne) // LevelOne is the top rule
    {
      // Decide which of the pairs is to be copied
      Rule* PObj = (LevelOne->leaf(0) != Ptr) ? LevelOne->leaf(0) : LevelOne->leaf(1);

      PObj->setParent(0); /// New Top rule
      TRule = PObj;
      LevelOne->setLeaves(0, 0); // Delete from Ptr, and copy
      // Note we now need to delete this 
      delete LevelOne;
    }
    else // Basic surf object
    {
      SurfPoint* SX = dynamic_cast<SurfPoint*> (Ptr);
      SX->setKeyN(0);
      SX->setKey(0);
      return cnt + 1;
    }
    delete Ptr;
    //delete LevelOne; // Shouldn't delete now we're deleting in setLeaf.
    Ptr = TRule->findKey(SurfN);
    cnt++;
  }
  return cnt;
}

Rule::Rule()  : Parent(0)
  /**
    Standard Constructor
  */
{}

Rule::Rule(const Rule&) : 
  Parent(0)
  /**
    Constructor copies. 
    Parent set to 0
  */
{}

Rule::Rule(Rule* A) : 
  Parent(A)
  /**
    Constructor copies. 
    Parent set to A
    @param A :: Parent value
  */
{}

Rule&
Rule::operator=(const Rule&) 
  /**
    Assignment operator=
    does not set parent as Rules
    are cloned 
    @return *this
  */
{
  return *this;
}

Rule::~Rule()
  /**
    Destructor
  */
{}

void
Rule::setParent(Rule* A)
  /**
    Sets the parent object (not check for A==this)
    @param A :: Partent Object Ptr 
  */
{
  Parent=A;
}

Rule*
Rule::getParent() const
  /**
    Returns the parent object 
    @return Parent
  */
{
  return Parent;
}

void
Rule::makeParents()
  /**
    This is initialisation code that populates 
    all the parents in the rule tree.
  */
{
  std::stack<Rule*> Tree;
  Tree.push(this);
  while(!Tree.empty())
    {
      Rule* Ptmp=Tree.top();
      Tree.pop();

      if (Ptmp)
        {
	  for(int i=0;i<2;i++)
	    {
	      Rule* tmpB=Ptmp->leaf(i);
	      if (tmpB)
	        {
		  tmpB->setParent(Ptmp);
		  Tree.push(tmpB);
		}
	    }
	}
    }
  return;
}

int
Rule::checkParents() const
  /**
    This code checks if a parent tree
    is valid
    @retval 0 on failure 
    @retval 1 on success
  */
{
  std::stack<const Rule*> Tree;
  Tree.push(this);
  while(!Tree.empty())
    {
      const Rule* Ptmp=Tree.top();
      Tree.pop();

      if (Ptmp)
        {
	  for(int i=0;i<2;i++)
	    {
	      const Rule* tmpB=Ptmp->leaf(i);
	      if (tmpB)
	        {
		  if (tmpB->getParent()!=Ptmp)
		    return 0;
		}
	    }
	}
    }
  return 1;
}


int
Rule::commonType() const
  /**
    Function to return the common type
    of an rule.
    @retval 1 :: every rule is an intersection or component
    @retval -1 :: every rule is an union or component
    @retval 0 :: mixed rule group or only component
  */
{
  // initial type
  const int rtype = this->type();       // note the dereference to get non-common comonents
  if (!rtype)
    return 0;
  // now this must be an intersection or a Union   
  std::stack<const Rule*> Tree;
  Tree.push(this->leaf(0));
  Tree.push(this->leaf(1));
  while(!Tree.empty())
    {
      const Rule* tmpA=Tree.top();
      Tree.pop();
      if (tmpA)
        {
	  if (tmpA->type() == -rtype)      // other type return void
	    return 0;           
	  const Rule* tmpB=tmpA->leaf(0);
	  const Rule* tmpC=tmpA->leaf(1);
	  if (tmpB)
	    Tree.push(tmpB);
	  if (tmpC)
	    Tree.push(tmpC);
	}
    }
  return rtype;
}

int
Rule::substituteSurf(const int SurfN,const int newSurfN,Surface* SPtr)
  /**
    Substitues a surface item if within a rule
    @param SurfN :: Number number to change
    @param newSurfN :: New surface number (if -ve then the key is reversed)
    @param SPtr :: New surface Pointer
    @return number of substitutions
  */
{
  int cnt(0);
  SurfPoint* Ptr=dynamic_cast<SurfPoint*>(findKey(SurfN));
  while(Ptr)
  {
    Ptr->setKeyN(Ptr->getSign()*newSurfN);
    Ptr->setKey(SPtr->clone());  // Clone to ensure uniqueness
    cnt++;
    // get next key
    Ptr=dynamic_cast<SurfPoint*>(findKey(SurfN));
  }
  delete SPtr;  // Delete incoming pointer
  return cnt;
}

int
Rule::getKeyList(std::vector<int>& IList)  const
  /**
    Generate the key list given an insertion
    type object. The list is a unique list.
    @param IList :: place to put keyList
    @return number of object inserted 
  */
{
  IList.clear();
  std::stack<const Rule*> TreeLine;
  TreeLine.push(this);
  while(!TreeLine.empty())
    {
      const Rule* tmpA=TreeLine.top();
      TreeLine.pop();
      const Rule* tmpB=tmpA->leaf(0);
      const Rule* tmpC=tmpA->leaf(1);
      if (tmpB || tmpC)
        {
	  if (tmpB)
	    TreeLine.push(tmpB);
	  if (tmpC)
	    TreeLine.push(tmpC);
	}
      else
        {
	  const SurfPoint* SurX=dynamic_cast<const SurfPoint*>(tmpA);
	  if (SurX)
	    IList.push_back(SurX->getKeyN());
	  else
	    {
	      std::cerr<<"Error with surface List"<<std::endl;
	      return IList.size();
	    }
	}
    }
  std::sort(IList.begin(),IList.end());
  std::vector<int>::iterator px=std::unique(IList.begin(),IList.end());
  IList.erase(px,IList.end());
  return IList.size();
}

int
Rule::Eliminate()
  /**
    This rule eliminates any unnecessary rules.
    \todo Currently does not work since removeItem changes the top rule
    (so should this)
    @return number of items eliminated
  */
{
  std::map<int,int> Base;     // map of key names + test value (initially 1) 
  std::vector<int> baseVal;
  std::vector<int> baseKeys;
  std::vector<int> deadKeys;
  // collect base keys and populate the cells
  getKeyList(baseKeys);  
  std::vector<int>::const_iterator xv;
  for(xv=baseKeys.begin();xv!=baseKeys.end();xv++)
    {
      baseVal.push_back(0);
      Base[(*xv)]=1;
    }

  // For each key :: check if the Rule is equal for both cases 0 + 1
  // then loop through all combinations of the map to determine validity
  // This function is not optimised since the tree can be trimmed
  // if the test item is not in a branch.
  for(unsigned int TKey=0;TKey<baseKeys.size();TKey++)
    {
      //INITIALISE STUFF
      int valueTrue(1),valueFalse(1);
      int keyChange=0;
      int targetKey=baseKeys[TKey];
      for(unsigned int i=0;i<baseVal.size();i++)
        {
	  baseVal[i]=0;
	  Base[baseKeys[i]]=0;
	}

      // CHECK EACH KEY IN TURN
      while(valueTrue==valueFalse || keyChange>=0)
        {
	  // Zero value
	  Base[baseKeys[targetKey]]=0;
	  valueFalse=isValid(Base);

	  // True value
	  Base[baseKeys[targetKey]]=1;
	  valueTrue=isValid(Base);

	  // Put everything back 
	  if (valueTrue==valueFalse)
	    {
	      keyChange=addToKey(baseVal,TKey);    // note pass index not key
	      for(int ic=0;ic<keyChange;ic++)
		Base[baseKeys[ic]]=baseVal[ic];
	    }
	}	  
      if (keyChange<0)     // Success !!!!!
	deadKeys.push_back(targetKey);
    }

  // NOW REMOVE DEAD KEYS
//  std::vector<int>::const_iterator vc;
//  for(vc=deadKeys.begin();vc!=deadKeys.end();vc++)
//    removeItem(*vc);

  return deadKeys.size();
}

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
