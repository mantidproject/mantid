#include "MantidGeometry/Math/Acomp.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Support.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Math/RotCounter.h"
#include <algorithm>
#include <iterator>

namespace Mantid
{

  namespace Geometry 
  {

    Kernel::Logger& Acomp::PLog( Kernel::Logger::get("Acomp"));
    // Friend function
    std::ostream&
      operator<<(std::ostream& OX,const Acomp& A) 
      /**
      Stream operator (uses Acomp::write)
      @param OX :: output stream
      @param A :: Acomp unit to output
      @return Output stream
      */

    {
      OX<<A.display();
      return OX;
    }

    void 
      split(const int A,int& S,int& V)
      /**
      Split a number into the sign and
      positive value
      @param A :: number to split
      @param S :: sign value +/- 1
      @param V :: abs(A) 
      */
    {
      if (A>=0)
      {
        S=1;
        V=A;
      }
      else
      {
        S=-1;
        V= -A;
      }
      return;
    }

    //
    // ------------- ACOMP ---------------- 
    //

    Acomp::Acomp(const int Tx) : 
    Intersect(Tx)
      /**
      Standard Constructor 
      @param Tx :: 1 it means intersect 0 it meanse union
      */
    {}

    Acomp::Acomp(const Acomp& A) : 
    Intersect(A.Intersect),Units(A.Units)
      /**
      Copy constructor
      @param A :: Acomp object to copy
      */
    { 
      std::vector<Acomp>::const_iterator vc;
      for(vc=A.Comp.begin();vc!=A.Comp.end();vc++)
        Comp.push_back(*vc);
    }

    Acomp&
      Acomp::operator=(const Acomp& A)
      /**
      The assignment operator carries out a 
      down level deletion of the Comp vector
      @param A :: Object to copy
      @return *this
      */
    {
      if (this!=&A)
      {
        Intersect=A.Intersect;
        Units=A.Units;
        Comp=A.Comp;
      }
      return *this;
    }


    Acomp::~Acomp()
      /**
      Destructor deletes each Comp Unit and
      down the chain.
      */
    { }

    bool
      Acomp::operator!=(const Acomp& A) const
      /**
      Inequality operator
      @param A :: other Acomp item to compare
      @return Not this==A
      */
    {
      return !(this->operator==(A));
    }

    bool
      Acomp::operator==(const Acomp& A) const
      /**
      Equals operator requires that the
      Units are equal and the Comp units
      are equal. Sort of the class is required.
      @param A :: other Acomp item to compare
      @return A==*this
      */
    {
      // Size not equal then definately not equal
      if (A.Units.size()!=Units.size())
        return false;
      if (A.Comp.size()!=Comp.size())
        return false;
      // Intersect not the same (unless size==1)
      if (A.Intersect!=Intersect && 
        Units.size()+Comp.size()!=1)          // Intersect type is not relevent 
        return false;                               // if singular.

      // If Units not empty compare each and determine
      // if equal. 
      if (!Units.empty())
      {
        std::vector<int>::const_iterator vc,xc;
        xc=A.Units.begin();
        for(vc=Units.begin();vc!=Units.end();xc++,vc++)
          if (*vc != *xc)
            return false;
      }

      // Both Empty :: thus equal
      if (Comp.empty())    
        return true;

      // Assume that comp Units are sorted.
      std::vector<Acomp>::const_iterator acv,xcv;
      acv=A.Comp.begin();
      for(xcv=Comp.begin();xcv!=Comp.end() &&
        *xcv==*acv ;xcv++,acv++);
        return (xcv==Comp.end()) ? true : false;
    }

    bool
      Acomp::operator<(const Acomp& A) const
      /**
      Comparitor operator:: Comparies the 
      unit list (which is already sorted)
      part by part.  The sort is ASSUMED.
      Then the Comp units (which are not)
      Order (low first)
      - Singulars
      - Intersections
      - Units (largest negative)
      - Acomps.
      @param A :: Object to compare
      @return this<A
      */
    {
      // PROCESS Singular
      const int TS=isSingle();         // is this one component
      const int AS=A.isSingle(); 
      if (TS!=AS)                     
        return (TS>AS) ? true : false;
      // PROCESS Intersection/Union
      if (!TS && Intersect!=A.Intersect)
      {
        // Union==0 therefore this > A
        if (Intersect > 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }

      // PROCESS Units. (order : then size)
      std::vector<int>::const_iterator uc,ac;
      ac=A.Units.begin();
      uc=Units.begin();
      for(;ac!=A.Units.end() && uc!=Units.end();uc++,ac++)
        if (*uc!=*ac)
          return (*uc < *ac);

      if (ac!=A.Units.end())
        return false;
      if (uc!=Units.end())
        return true;

      // PROCESS CompUnits.
      std::vector<Acomp>::const_iterator ux,ax;
      ux=Comp.begin();
      ax=A.Comp.begin();
      for(;ux!=Comp.end() && ax!=A.Comp.end();ux++,ax++)
      {
        if (*ax!=*ux)
          return (*ux < *ax);
      }
      if (uc!=Units.end())
        return true;
      // everything idential or A.comp bigger:
      return false;
    }

    Acomp&
      Acomp::operator+=(const Acomp& A)
      /**
      Operator + (union addition)
      @param A :: Object to union with this
      @return *this
      */
    {
      if (Intersect)                // If this is an intersection
      {                            // we need to have a union 
        Acomp Ax=Acomp(*this);    //make copy
        Units.clear();                    // remove everthing else
        Comp.clear();
        Intersect=0;                      // make this into a Union
        addComp(Ax);                     // add oldThis to the list
      }
      // Now add A to the union.
      if (!A.Intersect)                 // Can add components
        copySimilar(A);
      else 
        addComp(A);       // add components

      removeEqComp();     // remove equal units
      joinDepth();        // Up shift suitable objects.
      return *this;

    }

    Acomp&
      Acomp::operator-=(const Acomp& A)
      /**
      Operator - (removal)
      This operation can be carried out in many ways.
      It is by direct pattern subtraction followed
      by complementary subtraction or the remainder
      Complementary subtraction is by making A-B == A*B'
      @param A :: Object to subtract
      @return *this 
      */
    {

      std::vector<Acomp> Fparts;     // Parts in This 
      std::vector<Acomp> Gparts;     // Parts in A 
      if (!getDNFpart(Fparts) ||
        !A.getDNFpart(Gparts))
        return *this;

      std::vector<Acomp> Outparts;   // Parts in F not in G
      std::vector<Acomp> NegParts;   // Parts in G not in F

      // Have DNF parts and will return the same...
      // Sort the components of the list
      std::for_each(Fparts.begin(),Fparts.end(),std::mem_fun_ref(&Acomp::Sort));
      std::for_each(Gparts.begin(),Gparts.end(),std::mem_fun_ref(&Acomp::Sort));

      //Sort the list itself...
      std::sort(Gparts.begin(),Gparts.end());
      std::sort(Fparts.begin(),Fparts.end());

      //Process Components:
      std::vector<Acomp>::const_iterator fc,gc;
      gc=Gparts.begin();
      for(fc=Fparts.begin();gc!=Gparts.end() && fc!=Fparts.end();fc++)
      {
        while(gc!=Gparts.end() && (*gc < *fc))
        {
          NegParts.push_back(*gc);
          gc++;
        }

        if (gc!=Gparts.end() && *gc==*fc)          // match
          gc++;
        else   // ok that didn't work so add to us
          Outparts.push_back(*fc);
      }  
      // add extra bits from Gparts onto NegParts
      for(;gc!=Gparts.end();gc++)
        NegParts.push_back(*gc);

      // Clear This to put in Outparts.
      Units.clear();
      Comp.clear();
      Intersect=0;
      for(fc=Outparts.begin();fc!=Outparts.end();fc++)
        addComp(*fc);
      joinDepth();
      removeEqComp();                       // move equal items.
      // Now add any components from g but need to be as intersections
      std::vector<Acomp>::iterator negX;
      for(negX=NegParts.begin();negX!=NegParts.end();negX++)
      {
        negX->complement();
        this->operator*=(*negX);
      }
      removeEqComp();
      return *this;
    }

    bool
      Acomp::operator>(const Acomp& A) const
      /** 
      Operator> takes first to last precidence.
      Uses operator<  to obtain value.
      Note it does not uses 1-(A<this)
      @param  A :: object to compare
      @return this>A
      */
    {
      return A.operator<(*this);
    }


    Acomp&
      Acomp::operator*=(const Acomp& A)
      /**
      This carries out the intersection operation 
      with A. e.g. (a+b) * (ced) == (a+b)ced
      @param A :: Acomp unit to intersect
      @return *this
      */
    {
      if (!Intersect)                // If this is an intersection
      {                            // we need to have a union 
        Acomp Ax=Acomp(*this);    //make copy
        Units.clear();                    // remove everthing else
        Comp.clear();
        Intersect=1;                      // make this into a Union
        addComp(Ax);                     // add oldThis to the list
      }
      if (A.Intersect)                  // can add components
        copySimilar(A);
      else 
        addComp(A);       // combine components
      removeEqComp();
      joinDepth();  
      return *this;
    }

    // ---------------------------------------------------
    //  PRIVATE FUNCTIONS 
    // ---------------------------------------------------

    void
      Acomp::deleteComp()
      /**
      Deletes everything in Composite
      */
    {
      Comp.clear();
      return;
    }

    void
      Acomp::addComp(const Acomp& AX)
      /**
      Adds a pointer to the Comp list.
      If the pointer is singular, extracts the
      object and adds that componenet.
      Assumes that the component is sorted and inserts appropiately.
      @param AX :: Acomp component to add
      */
    {
      std::pair<int,int> Stype=AX.size();
      if (Stype.first+Stype.second==0)
        throw std::runtime_error("Pair Count wrong in AddComp");

      if (AX.isSingle() || AX.Intersect==Intersect)       //single unit component/Conjoint
      {
        std::vector<int>::const_iterator aup;
        for(aup=AX.Units.begin();aup!=AX.Units.end();aup++)
        {
          std::vector<int>::iterator ipt;
          ipt=std::lower_bound(Units.begin(),Units.end(),*aup);
          if (ipt==Units.end() || *ipt!=*aup)                       // Only insert if new
            Units.insert(ipt,*aup);
        }
        std::vector<Acomp>::const_iterator acp;
        for(acp=AX.Comp.begin();acp!=AX.Comp.end();acp++)
        {
          std::vector<Acomp>::iterator cpt;
          cpt=std::lower_bound(Comp.begin(),Comp.end(),*acp);
          if (cpt==Comp.end() || *cpt!=*aup)                       // Only insert if new
            Comp.insert(cpt,*acp);
        }
        return;
      }
      // Type different insertion
      std::vector<Acomp>::iterator cpt;
      cpt=std::lower_bound(Comp.begin(),Comp.end(),AX);
      if (cpt==Comp.end() || *cpt!=AX)                       // Only insert if new
        Comp.insert(cpt,AX);
      return;
    }

    void
      Acomp::addUnitItem(const int Item)
      /**
      Adds a unit to the Unit list (if it doesn't exist). 
      @param Item :: Unit to add
      */
    {
      // Quick and cheesy insertion if big.
      std::vector<int>::iterator ipt;
      ipt=std::lower_bound(Units.begin(),Units.end(),Item);
      if (ipt==Units.end() || *ipt!=Item)                       // Only insert if new
        Units.insert(ipt,Item);
      return;
    }

    void
      Acomp::processIntersection(const std::string& Ln)
      /**
      Helper function :: assumes that Ln has been
      checked for bracket consistency.
      Units are sorted after this function is returned.
      @param Ln :: String to processed as an intersection
      must not contain a toplevel +
      @throw ExBase on failure to pass string
      */
    {
      std::string Bexpress;           // bracket expression
      int blevel=0;                       // this should already be zero!!
      // find first Component to add 
      //  std::cerr<<"Process Inter:"<<Ln<<std::endl;
      int numItem(0);
      for(unsigned int iu=0;iu<Ln.length();iu++)
      {
        if (blevel)       // we are in a bracket then...
        {
          if (Ln[iu]==')')        // maybe closing outward..
            blevel--;
          else if (Ln[iu]=='(')
            blevel++;
          if (blevel)
            Bexpress+=Ln[iu];
          else           // Process end: of Brackets 
          {
            Acomp AX;   
            try
            {
              AX.setString(Bexpress);
            }
            catch(...)
            {
              std::cerr<<"Error in string creation"<<std::endl;
            }
            Bexpress.erase();     // reset string
            addComp(AX);          // add components
          }
        }
        else               // Not in a bracket (currently)
        {
          if (Ln[iu]=='(')
            blevel++;
          else if (isalpha(Ln[iu]) || Ln[iu]=='%')
          {
            if (Ln[iu]=='%')
            {
              iu++;
              const int Nmove=StrFunc::convPartNum(Ln.substr(iu),numItem);
              if (!Nmove)
                throw std::invalid_argument("Acomp::procIntersection error in line Ln\"" + Ln + "\"");
              numItem+=52;
              iu+=Nmove;
            }
            else
            {
              numItem=(islower(Ln[iu])) ?
                static_cast<int>(1+Ln[iu]-'a') :
              static_cast<int>(27+Ln[iu]-'A');
              iu++;
            }
            if (iu<Ln.length() && Ln[iu]=='\'')
            {
              addUnitItem(-numItem);
            }
            else
            {
              addUnitItem(numItem);
              iu--;
            }
          }
        }
      }
      return;
    }

    void
      Acomp::processUnion(const std::string& Ln)
      /**
      Helper function :: assumes that Ln has been
      checked for bracket consistency
      Units are sorted after this function is returned.
      @param Ln :: String to processed as a union 
      (must contain one external +)
      @throw ExBase on failure to pass string
      */
    {
      std::string Express;           // Intersection Expression
      int blevel=0;          
      int bextra=0;
      // find first Component to add 
      //  std::cerr<<"Process Union:"<<Ln<<std::endl;
      for(unsigned int iu=0;iu<Ln.length();iu++)
      {
        if (blevel)       // we are in a bracket then...
        {
          if (Ln[iu]==')')        // maybe closing outward..
            blevel--;
          else if (Ln[iu]=='(')
            blevel++;
          if (blevel || bextra)
            Express+=Ln[iu];
        }
        else
        {
          if (Ln[iu]=='+')
          {
            Acomp AX;
            try
            {
              AX.setString(Express);  
            }
            catch(...)
            {
              std::cerr<<"Error in string "<<std::endl;
              throw;
            }
            Express.erase();     // reset string
            addComp(AX);       // add components
          }
          else if (Ln[iu]=='(')
          {
            blevel++;
            if (Express.length())
            {
              Express+='(';
              bextra=1;
            }
            else
              bextra=0;
          }
          else
            Express+=Ln[iu];
        }
      }
      if (Express.size()>0)
      {
        Acomp AX;
        try
        {
          AX.setString(Express);  
        }
        catch(...)
        {
          std::cerr<<"Error in bracket ::"<<std::endl;
          throw;
        }
        addComp(AX);       // add component
      }
      return;
    }

    int
      Acomp::copySimilar(const Acomp& A)
      /**
      Class to merge two list of similar
      objects. Makes a full copy of the objects
      It requires that the Intersect is the same for both
      @param A :: Object to copy
      @return 0 on success -1 on failure
      */
    {
      // copy units and components
      if (Intersect!=A.Intersect)
        return -1;

      if (!A.Units.empty())
      {
        std::vector<int>::iterator Ept=Units.end();
        Units.resize(Units.size()+A.Units.size());
        copy(A.Units.begin(),A.Units.end(),Ept);
        std::sort(Units.begin(),Units.end());
      }

      // Add components 
      std::vector<Acomp>::const_iterator vc;
      for(vc=A.Comp.begin();vc!=A.Comp.end();vc++)
        addComp(*vc);
      return 0;
    }

    void
      Acomp::addUnit(const std::vector<int>& Index,const BnId& BX)
      /**
      Given a single BnId unit and an index
      adds it to the main Units object.
      @param Index :: number , surfNumber
      @param BX :: binary component
      */
    {
      int flag,S,V;
      for(int i=0;i<BX.Size();i++)
      {
        flag=BX[i];
        if (flag)
        {
          split(flag,S,V);
          if (V>=static_cast<int>(Index.size()))
            throw std::runtime_error("Error with addUnit::Index");
          Units.push_back(S*Index[i]);
        }
      }
      std::sort(Units.begin(),Units.end());
      return;
    }


    void
      Acomp::assignDNF(const std::vector<int>& Index,const std::vector<BnId>& A)
      /**
      Assign the object to the Essentual PI in the vector
      A. This will make the form DNF.
      @param Index :: SurfNumbers
      @param A :: Vector of BnId's that are valid
      */
    {
      Units.clear();
      deleteComp();
      if (A.size()==0)
        return;

      if (A.size()==1)  //special case for single intersection 
      {
        Intersect=1;
        addUnit(Index,A[0]);
        return;
      }

      Intersect=0;               // union of intersection
      std::vector<BnId>::const_iterator vc;
      for(vc=A.begin();vc!=A.end();vc++)
      {
        Acomp Px(1);      // Intersection
        Px.addUnit(Index,*vc);
        addComp(Px);
      }
      return;
    }

    void
      Acomp::assignCNF(const std::vector<int>& Index,const std::vector<BnId>& A)
      /**
      Assign the object to the Essentual PI in the vector
      A. This will make the form DNF.
      @param Index :: SurfNumbers
      @param A :: Vector of BnId's that are valid
      */
    {
      Units.clear();
      deleteComp();
      if (A.size()==0)
        return;

      if (A.size()==1)  //special case for single union
      {
        Intersect=0;
        addUnit(Index,A[0]);
        return;
      }

      Intersect=1;               // intersection of union 
      std::vector<BnId>::const_iterator vc;
      for(vc=A.begin();vc!=A.end();vc++)
      {
        Acomp Px(0);      // Union
        //std::cout<<"Item == "<<*vc<<std::endl;
        BnId X=*vc;
        X.reverse();
        Px.addUnit(Index,X);
        addComp(Px);
      }
      return;
    }


    // -------------------------------------
    //   PUBLIC FUNCTIONS 
    // -------------------------------------

    void
      Acomp::Sort()
      /**
      Function to sort the components of the lists.
      Decends down the Comp Tree.
      */
    {
      std::sort(Units.begin(),Units.end());
      // Sort each decending object first
      std::for_each(Comp.begin(),Comp.end(),std::mem_fun_ref(&Acomp::Sort));
      // use the sorted components to sort our component list
      std::sort(Comp.begin(),Comp.end());
      return;
    }

    int
      Acomp::makeReadOnce() 
      /**
      This function attempts to make the function
      a read one form. Assumes that it is either DNF or
      CNF form
      @retval 0 if fails
      @retval 1 if success (and sets it into the appropiate form)
      */
    {
      std::map<int,int> LitSet;
      getLiterals(LitSet);
      /*  // Process DNF
      if (isDNF())
      return makeReadOnceForDNF();
      if (isCNF())
      return makeReadOnceForCNF();
      return 0;
      */
      return 0;
    }



    int
      Acomp::logicalEqual(const Acomp& A) const
      /**
      Test that the system that is logically the same:
      @param A :: Logical state to test
      @retval 0 :: false  
      @retval 1 :: true
      */
    {
      std::map<int,int> litMap;       // keynumber :: number of occurances
      getAbsLiterals(litMap);

      A.getAbsLiterals(litMap);
      std::map<int,int> Base;       // keynumber :: number of occurances
      std::vector<int> keyNumbers;


      std::map<int,int>::const_iterator mc;
      for(mc=litMap.begin();mc!=litMap.end();mc++)
      {
        Base[mc->first]=1;          // Insert and Set to true
        keyNumbers.push_back(mc->first);
      }

      BnId State(Base.size(),0);                 //zero base
      do
      {
        State.mapState(keyNumbers,Base);
        if (isTrue(Base) != A.isTrue(Base))
          return 0;
      } while(++State);
      return 1;
    }

    int
      Acomp::isNull() const
      /**
      @return 1 if there are no memebers
      */
    {
      return ((!Units.size() && !Comp.size()) ? 1 : 0);
    }

    int
      Acomp::isDNF() const
      /**
      Determines if the component is in
      DNF form. This Acomp needs to be a union
      of intersection. 
      @retval 1 ::  DNF form
      @retval 0 ::  not DNF form
      */
    {
      // If an intersect (single component) 
      if (Intersect)
        return (Comp.empty()) ? 1 : 0;
      // Else needs to be union of Intersections.
      std::vector<Acomp>::const_iterator cc;
      for(cc=Comp.begin();cc!=Comp.end();cc++)
        if (cc->Intersect==0 || !cc->isSimple())
          return 0;

      return 1;
    }

    int
      Acomp::isCNF() const
      /**
      Determines if the component is in
      CNF form. This Acomp needs to be an intersection
      of unions
      @retval 1 ::  DNF form
      @retval 0 ::  not DNF form
      */
    {
      // If an intersect (single component) 
      if (!Intersect)
        return (Comp.empty()) ? 1 : 0;
      // Else needs to be intersection of unions
      std::vector<Acomp>::const_iterator cc;
      for(cc=Comp.begin();cc!=Comp.end();cc++)
        if (cc->Intersect==1 || !cc->isSimple())
          return 0;

      return 1;
    }

    void
      Acomp::getAbsLiterals(std::map<int,int>& literalMap) const
      /**
      get a map of the literals and the frequency
      that they occur.
      This does not keep the +/- part of the literals separate
      @param literalMap :: Map the get the frequency of the 
      literals 
      */
    {
      std::vector<int>::const_iterator uc;
      std::map<int,int>::iterator mc;
      int S,V;
      for(uc=Units.begin();uc!=Units.end();uc++)
      {
        split(*uc,S,V);
        mc=literalMap.find(V);
        if (mc!=literalMap.end())
          mc->second++;
        else
          literalMap.insert(std::pair<int,int>(V,1));
      }
      std::vector<Acomp>::const_iterator cc;
      for(cc=Comp.begin();cc!=Comp.end();cc++)
        cc->getAbsLiterals(literalMap);
      return;
    }

    void
      Acomp::getLiterals(std::map<int,int>& literalMap) const
      /**
      Get a map of the literals and the frequency
      that they occur.
      This keeps + and - literals separate
      @param literalMap :: Map the get the frequency of the 
      literals 
      */
    {
      std::vector<int>::const_iterator uc;
      std::map<int,int>::iterator mc;
      for(uc=Units.begin();uc!=Units.end();uc++)
      {
        mc=literalMap.find(*uc);
        if (mc!=literalMap.end())
          mc->second++;
        else
          literalMap.insert(std::pair<int,int>(*uc,1));
      }
      std::vector<Acomp>::const_iterator cc;
      for(cc=Comp.begin();cc!=Comp.end();cc++)
      {
        cc->getLiterals(literalMap);
      }
      // Doesn't work because literal map is a reference
      //  for_each(Comp.begin(),Comp.end(),
      // std::bind2nd(std::mem_fun(&Acomp::getLiterals),literalMap));
      return;
    }

    int
      Acomp::isSimple() const
      /**
      Determines if there are not complex components.
      @retval 1 :: Comp is empty
      @retval 0 :: Contains Components.
      */
    {
      return (Comp.empty() ? 1 : 0);
    }

    int
      Acomp::isSingle() const
      /**
      Deterimines if the item's singular
      @retval 1 if only one/zero item
      @retval 0 if more than one item.
      */
    {
      return (Comp.size()+Units.size()>1 ? 0 : 1);
    }


    int
      Acomp::removeEqComp()
      /**
      Remove identical items.
      @return number removed.
      */
    {
      // First check all the Comp units:
      int cnt(0);
      std::vector<Acomp>::iterator dx;
      sort(Comp.begin(),Comp.end());
      dx=std::unique(Comp.begin(),Comp.end());
      cnt+=std::distance(dx,Comp.end());
      Comp.erase(dx,Comp.end());

      // Units are sorted

      sort(Units.begin(),Units.end());
      std::vector<int>::iterator ux=unique(Units.begin(),Units.end());
      cnt+=std::distance(ux,Units.end());
      Units.erase(ux,Units.end());
      return cnt;
    }


    int
      Acomp::makePI(std::vector<BnId>& DNFobj) const
      /**
      This method finds the principle implicants.
      @param DNFobj :: A vector of Binary ID from a true 
      vectors of keyvalues.
      @return number of PIs found.
      \todo  Can we set this up to get non-pairs
      i.e. one pass.

      */
    {
      if (DNFobj.empty())   // no work to do return.
        return 0;
      // Note: need to store PI components separately
      // since we don't want to loop continuously through them
      std::vector<BnId> Work;       // Working copy
      std::vector<BnId> PIComp;     // Store for PI componends
      std::vector<BnId> Tmod;       // store modified components
      int changeCount(0);           // Number change
      std::vector<BnId>::iterator uend;     // itor to remove unique
      // Need to make an initial copy.
      Work=DNFobj;

      int cnt(0);
      do
      {
        cnt++;
        // Deal with tri-state objects ??
        sort(Work.begin(),Work.end());
        uend=unique(Work.begin(),Work.end());
        Work.erase(uend,Work.end());
        Tmod.clear();                  // erase all at the start
        //set PI status to 1
        for_each( Work.begin(),Work.end(),
          std::bind2nd(std::mem_fun_ref(&BnId::setPI),1) );

        //Collect into pairs which have a difference of +/- one 
        // object
        // Can sort this realive to 
        std::vector<BnId>::iterator vc;
        for(vc=Work.begin();vc!=Work.end();vc++)
        {
          const int GrpIndex(vc->TrueCount()+1);
          std::vector<BnId>::iterator oc=vc+1;
          for(oc=vc+1;oc!=Work.end();oc++)
          {
            const int OCnt=oc->TrueCount();
            if (OCnt>GrpIndex)
              break;
            if (OCnt==GrpIndex)
            {
              std::pair<int,BnId> cVal=vc->makeCombination(*oc);
              if (cVal.first==1)   // was complementary
              {
                Tmod.push_back(cVal.second);
                oc->setPI(0);         
                vc->setPI(0);
                changeCount++;       // 1 changed
              }
            }
          }
        }

        for(vc=Work.begin();vc!=Work.end();vc++)
          if (vc->PIstatus()==1)
            PIComp.push_back(*vc);
        Work=Tmod;

      } while (!Tmod.empty());
      // Copy over the unit.

      return makeEPI(DNFobj,PIComp);
    }


    int
      Acomp::makeEPI(std::vector<BnId>& DNFobj,
      std::vector<BnId>& PIform) const
      /**
      Creates an essentual PI list (note: this is 
      not unique).
      Given the list form the EPI based on the Quine-McClusky method.

      @param DNFobj :: Object in DNF form 
      @param PIform :: List of rules in Prime Implicant form
      It is set on exit (to the EPI)
      @return :: 1 if successful and 0 if failed
      */
    {
      const int debug(0);
      if (!PIform.size())
        return 0;

      std::vector<BnId> EPI;  // Created Here.

      std::vector<int> EPIvalue;
      // Make zeroed matrix.
      Geometry::Matrix<int> Grid(PIform.size(),DNFobj.size()); 
      std::vector<int> DNFactive(DNFobj.size());       // DNF that active
      std::vector<int> PIactive(PIform.size());        // PI that are active
      std::vector<int> DNFscore(DNFobj.size());        // Number in each channel
      std::vector<int> PIscore(DNFobj.size());        // Number in each channel

      //Populate
      for(unsigned int pc=0;pc!=PIform.size();pc++)
        PIactive[pc]=pc;

      for(unsigned int ic=0;ic!=DNFobj.size();ic++)
      {
        DNFactive[ic]=ic;                            //populate (avoid a loop)
        for(unsigned int pc=0;pc!=PIform.size();pc++)
        {
          if (PIform[pc].equivalent(DNFobj[ic]))
          {
            Grid[pc][ic]=1;
            DNFscore[ic]++;
          }
        }
        if (DNFscore[ic]==0)
        {
          std::cerr<<"PIForm:"<<std::endl;
          copy(PIform.begin(),PIform.end(),
            std::ostream_iterator<BnId>(std::cerr,"\n"));
          std::cerr<<"Error with DNF / EPI determination at "<<ic<<std::endl;
          std::cerr<<" Items "<<DNFobj[ic]<<std::endl;
          return 0;
        }
      }
      /// DEBUG PRINT 
      if (debug)
        printImplicates(PIform,Grid);
      /// END DEBUG

      std::vector<int>::iterator dx,ddx;     // DNF active iterator
      std::vector<int>::iterator px;     // PIactive iterator

      // 
      // First remove singlets:
      // 
      for(dx=DNFactive.begin();dx!=DNFactive.end();dx++)
      {
        if (*dx>=0 && DNFscore[*dx]==1)        // EPI (definately)
        {
          for(px=PIactive.begin();
            px!=PIactive.end() && !Grid[*px][*dx];px++);

            EPI.push_back(PIform[*px]);
          // remove all minterm that the EPI covered
          for(ddx=DNFactive.begin();ddx!=DNFactive.end();ddx++)
            if (*ddx>=0 && Grid[*px][*ddx])
              *ddx= -1;      //mark for deletion (later)
          // Can remove PIactive now.
          PIactive.erase(px,px+1);
        }
      }
      // Remove dead items from active list
      DNFactive.erase(
        remove_if(DNFactive.begin(),DNFactive.end(),
        std::bind2nd(std::less<int>(),0)),
        DNFactive.end());


      /// DEBUG PRINT 
      if (debug)
      {
        for(px=PIactive.begin();px!=PIactive.end();px++)
        {
          std::cerr<<PIform[*px]<<":";
          for(ddx=DNFactive.begin();ddx!=DNFactive.end();ddx++)
            std::cerr<<((Grid[*px][*ddx]) ? " 1" : " 0");
          std::cerr<<std::endl;
        }
        std::cerr<<"END OF TABLE "<<std::endl;
      }

      // Ok -- now the hard work...
      // need to find shortest "combination" that spans
      // the remaining table.

      // First Make a new matrix for speed.  Useful ???
      Geometry::Matrix<int> Cmat(PIactive.size(),DNFactive.size());  // corrolation matrix
      int cm(0);
      for(px=PIactive.begin();px!=PIactive.end();px++)
      {
        int dm(0);
        for(ddx=DNFactive.begin();ddx!=DNFactive.end();ddx++)
        {
          if (Grid[*px][*ddx])
            Cmat[cm][dm]=1;
          dm++;
        } 
        cm++;
      }

      const int Dsize(DNFactive.size());
      const int Psize(PIactive.size());
      //icount == depth of search ie 
      int vecI,di; // variable for later
      for(int Icount=1;Icount<Psize;Icount++)
      {
        // This counter is a ripple counter, ie 1,2,3 where no numbers 
        // are the same. BUT it is acutally 0->N 0->N 0->N
        // index by A, A+1 ,A+2  etc
        RotaryCounter Index(Icount,Psize);
        do {

          for(di=0;di<Dsize;di++)   //check each orignal position
          {
            for(vecI=0;vecI<Icount &&
              !Cmat[Index[vecI]][di];vecI++);
              if (vecI==Icount)
                break;
          }
          if (di==Dsize)          // SUCCESS!!!!!
          {
            for(int iout=0;iout<Icount;iout++)
              EPI.push_back(PIform[Index[iout]]);
            DNFobj=EPI;
            return 1;;
          }
        } while(!(++Index));
      }

      //OH well that means every PIactive is a EPI :-(
      for(px=PIactive.begin();px!=PIactive.end();px++)
        EPI.push_back(PIform[*px]);
      DNFobj=EPI;
      return 1;
    }

    std::vector<int>
      Acomp::getKeys() const
      /**
      Get the key numbers in the system
      @return Key of literals
      */
    {
      std::map<int,int> litMap;       // keynumber :: number of occurances
      std::vector<int> keyNumbers;
      getAbsLiterals(litMap);
      std::map<int,int>::const_iterator mc;
      for(mc=litMap.begin();mc!=litMap.end();mc++)
        keyNumbers.push_back(mc->first);

      return keyNumbers;
    }

    int
      Acomp::getDNFobject(std::vector<int>& keyNumbers,
      std::vector<BnId>& DNFobj) const
      /**
      Creates the DNF items (ie the binary list
      of true statements) It forms a sum of products.
      The original is not changed by the keynumbers and the 
      DNF objects are output into the function parameters.
      @param keyNumbers :: index list of the DNFobj. The
      [bitNum]->rule/key number.
      @param DNFobj :: write out the DNF object into BnId form
      @retval 0 :: on success.
      @retval -1 :: on error.
      */
    {
      std::map<int,int> litMap;       // keynumber :: number of occurances
      std::map<int,int> Base;         // keynumber :: value
      getAbsLiterals(litMap);

      if (litMap.empty())
        return -1;

      keyNumbers.clear();
      std::map<int,int>::iterator mc;

      for(mc=litMap.begin();mc!=litMap.end();mc++)
      {
        Base[mc->first]=1;          // Set to true
        keyNumbers.push_back(mc->first);
      }

      DNFobj.clear();
      BnId State(Base.size(),0);                 //zero base
      do
      {
        State.mapState(keyNumbers,Base);
        if (isTrue(Base))
        {
          DNFobj.push_back(State);
        }
      } while(++State);

      return 0;
    }


    int
      Acomp::makeDNFobject()
      /**
      Sets the object to the DNF form.
      @retval 0 on failure
      @retval Number of DNF components
      */
    {
      std::vector<BnId> DNFobj;
      std::vector<int> keyNumbers;
      if (!getDNFobject(keyNumbers,DNFobj))
      {
        if (makePI(DNFobj))
          assignDNF(keyNumbers,DNFobj);
        return DNFobj.size();
      }
      return 0;
    }

    int
      Acomp::makeCNFobject()
      /**
      Sets the object to the CNF form.
      @retval 0 on failure
      @retval Number of CNF components
      */
    {
      std::vector<BnId> CNFobj;
      std::vector<int> keyNumbers;
      if (!getCNFobject(keyNumbers,CNFobj))
      {
        if (makePI(CNFobj))
          assignCNF(keyNumbers,CNFobj);
        return CNFobj.size();
      }
      return 0;
    }

    int
      Acomp::getDNFpart(std::vector<Acomp>& Parts) const
      /**
      Sets the object into parts of the DNF form
      then puts the object in the Parts section
      @param Parts:: vector of the Parts found (Acomp units without
      component)
      @return number of parts
      */
    {
      /**
      If this is DNF then we don't want
      to calculate the DNF but just use it
      */
      if (isDNF())
      {
        std::vector<int>::const_iterator vc;
        std::vector<Acomp>::const_iterator xc;

        Parts.clear();
        for(vc=Units.begin();vc!=Units.end();vc++)
        {
          Acomp Aitem(1);            // Intersection (doesn't matter since 1 object)
          Aitem.addUnitItem(*vc);
          Parts.push_back(Aitem);
        }
        for(xc=Comp.begin();xc!=Comp.end();xc++)
          Parts.push_back(*xc);
        return Parts.size();
      }

      std::vector<int> keyNumbers;
      std::vector<BnId> DNFobj;
      if (!getDNFobject(keyNumbers,DNFobj))
      {
        if (makePI(DNFobj))
        {
          std::vector<BnId>::const_iterator vc;
          for(vc=DNFobj.begin();vc!=DNFobj.end();vc++)
          {
            Acomp Aitem(1);     // make an intersection and add components
            Aitem.addUnit(keyNumbers,*vc);
            Parts.push_back(Aitem);
          }
        }	  
        return Parts.size();
      }
      return 0;
    }

    int
      Acomp::getCNFobject(std::vector<int>& keyNumbers,
      std::vector<BnId>& CNFobj) const
      /**
      Creates the CNF items (ie the binary list
      of false statements) It forms a sum of products.
      The original is not changed by the keynumbers and the 
      DNF objects are output into the function parameters.
      @param keyNumbers :: index list of the CNFobj. The
      [bitNum]->rule/key number.
      @param CNFobj :: write out the CNF object into BnId form
      @retval 0 :: on success.
      @retval -1 :: on error.
      */
    {
      std::map<int,int> litMap;       // keynumber :: number of occurances
      std::map<int,int> Base;         // keynumber :: value
      getAbsLiterals(litMap);

      if (litMap.size()<1)
        return -1;

      keyNumbers.clear();
      std::map<int,int>::iterator mc;
      int cnt(0);


      for(mc=litMap.begin();mc!=litMap.end();mc++)
      {
        mc->second=cnt++;
        Base[mc->first]=1;          // Set to true
        keyNumbers.push_back(mc->first);
      }

      CNFobj.clear();
      BnId State(Base.size(),0);                 //zero base
      do
      {
        State.mapState(keyNumbers,Base);
        if (!isTrue(Base))
          CNFobj.push_back(State);
      } while(++State);

      return 0;
    }

    int
      Acomp::isTrue(const std::map<int,int>& Base) const
      /**
      Determines if the rule is true, given
      the Base state.
      @param Base :: map of <LiteralNumber, State> 
      @return 1 if true and 0 if false
      */
    {
      if (Units.empty() &&  Comp.empty())
        return 1;

      // Deal with case of a single object (then join
      // doesn't matter
      int retJoin=Units.size()+Comp.size();
      if (retJoin!=1)          // single unit is alway ok 
        retJoin=1-Intersect;         

      int aimTruth,S,V;
      std::map<int,int>::const_iterator bv;
      std::vector<int>::const_iterator uc;
      // e.g. a'b   1 1  (retJoin ==1)
      for(uc=Units.begin();uc!=Units.end();uc++)
      {
        split(*uc,S,V);
        bv=Base.find(V);
        if (bv==Base.end())
          throw std::runtime_error("Base unit not found");
        aimTruth= (S<0) ? 1-retJoin : retJoin;

        if (bv->second == aimTruth)          // any true then return true
          return retJoin;
      }

      std::vector<Acomp>::const_iterator cc;
      for(cc=Comp.begin();cc!=Comp.end();cc++)
        if (cc->isTrue(Base)==retJoin)
          return retJoin;

      // Finally not true then
      return 1-retJoin;    

    }

    std::pair<Acomp,Acomp>
      Acomp::algDiv(const Acomp& G)
      /**
      Carries out algebraic division
      @param G :: The divisor
      @return Pair of Divided + Remainder
      */
    {
      //First make completely DNF (if necessary)
      if (!isDNF() && !makeDNFobject())
        return std::pair<Acomp,Acomp>(Acomp(),Acomp());

      std::map<int,int> Gmap; // get map of literals and frequency
      G.getLiterals(Gmap);
      if (!Gmap.size())
        return std::pair<Acomp,Acomp>(Acomp(),Acomp());
      // Make two lists.
      // U == set of elements in F (item to be divided) (this)
      // V == set of elements in G 
      // U is ste to be 
      std::vector<Acomp> U;
      std::vector<Acomp> V;
      std::map<int,int>::const_iterator mc;
      // Only have First level components to consider
      std::vector<Acomp>::const_iterator cc;
      int cell;

      std::vector<Acomp> Flist,Glist;
      if (!getDNFpart(Flist) || !G.getDNFpart(Glist))
        return std::pair<Acomp,Acomp>(Acomp(),Acomp());

      for(cc=Flist.begin();cc!=Flist.end();cc++)
      {
        int itemCnt(0);
        U.push_back(Acomp(0));      //intersection Unit
        V.push_back(Acomp(0));      //intersection Unit
        Acomp& Uitem= U.back();
        Acomp& Vitem= V.back();
        while( (cell = cc->itemN(itemCnt)) )
        {
          if (Gmap.find(cell)!=Gmap.end())
            Uitem.addUnitItem(cell);
          else
            Vitem.addUnitItem(cell);
          itemCnt++;
        }
      }
      Acomp H(1);       // H is an intersection group
      Acomp Hpart(0);    //Hpart is a union group
      /*
      std::cerr<<"U == ";
      copy(U.begin(),U.end(),std::ostream_iterator<Acomp>(std::cerr,":"));
      std::cerr<<std::endl;
      std::cerr<<"V == ";
      copy(V.begin(),V.end(),std::ostream_iterator<Acomp>(std::cerr,":"));
      std::cerr<<std::endl;
      */
      for(cc=Glist.begin();cc!=Glist.end();cc++)
      {
        std::vector<Acomp>::const_iterator ux,vx;
        vx=V.begin();
        for(ux=U.begin();ux!=U.end() && vx!=V.end();vx++,ux++)
        {
          if (!vx->isNull() && ux->contains(*cc))
          {
            Hpart.addComp(*vx);
          }
        }
        // H exists then combine (note: must do this if a composite divisor)
        if (!Hpart.isNull())
        {
          H*=Hpart;
          H.joinDepth();        // Up shift suitable objects.
          Hpart.Comp.clear();
          Hpart.Units.clear();   // Just in case
        }
      }
      if (!H.isDNF())
        H.makeDNFobject();
      Acomp Rem(*this);
      Acomp Factor(H);
      Factor*=G;
      Rem-=Factor;
      return std::pair<Acomp,Acomp>(H,Rem);
    }

    int 
      Acomp::contains(const Acomp& A) const
      /**
      Checks the Units of A to  see if they are in this->Units.
      Assumes that Units is sorted.
      @param A :: Object to cross compare
      @retval 0 :: all literals in A are in this
      @retval 1 :: A is unique from this
      */
    {
      std::vector<int>::const_iterator vc,tc;
      tc=Units.begin();
      for(vc=A.Units.begin();vc!=A.Units.end();vc++)
      {
        while(tc!=Units.end() && *tc<*vc)
          tc++;
        if (tc==Units.end() || *tc!=*vc)
          return 0;
      }
      return 1;
    }

    int
      Acomp::joinDepth()
      /**
      Searchs down the tree to find if any
      singles exist and up-promotes them.
      @throw ColErr::ExBase on mal-formed state
      @return number of up-promotions.
      */
    {

      // Deal with case that this is a singular object::
      std::pair<int,int> Stype=size();
      if (Stype.first+Stype.second==0)
        throw std::runtime_error("Pair Count wrong");
      //SINGULAR
      int cnt(0);        // number upgraded
      if (Stype.first==0 && Stype.second==1)            // Comp to up-premote
      {
        const Acomp* Lower=itemC(0);                    // returns pointer
        Units.clear();
        Intersect=Lower->Intersect;
        if (Lower->Units.size())
        {
          Units.resize(Lower->Units.size());
          copy(Lower->Units.begin(),Lower->Units.end(),Units.begin());
        }
        if (!Lower->Comp.empty())
        {
          Comp.resize(Lower->Comp.size()+1);     // +1 has space for our initial object
          copy(Lower->Comp.begin(),Lower->Comp.end(),Comp.begin()+1);
        }
        Comp.erase(Comp.begin(),Comp.begin()+1);
        cnt++;
      }
      // Loop over each component.  (if identical type upshift and remove)
      for(unsigned int ix=0;ix<Comp.size();ix++)
      {
        Acomp& AX=Comp[ix];
        Stype=AX.size();
        if (Stype.first+Stype.second==0)      
          throw std::runtime_error("Pair Count wrong");

        // If Singular then can be up premoted.
        if (Stype.first+Stype.second==1)       //single unit component.
        {
          if (Stype.first==1)      
            Units.push_back(AX.itemN(0));
          else
            Comp.push_back(*AX.itemC(0)); 
          // delete memory and the component.
          Comp.erase(Comp.begin()+ix,Comp.begin()+ix+1);
          ix--;
          cnt++;
        }
        else if (Intersect==AX.Intersect)    // Same type thus use the bits.
        {
          Units.resize(Units.size()+AX.Units.size());
          copy(AX.Units.begin(),AX.Units.end(),
            Units.end()-AX.Units.size());
          Comp.resize(Comp.size()+AX.Comp.size());
          copy(AX.Comp.begin(),AX.Comp.end(),
            Comp.end()-AX.Comp.size());

          Comp.erase(Comp.begin()+ix,Comp.begin()+ix+1);
          ix--;
          cnt++;
        }	  
      }
      if (cnt)
      {
        Sort();
        removeEqComp();
      }
      std::vector<Acomp>::iterator xc;
      for(xc=Comp.begin();xc!=Comp.end();xc++)
        cnt+=xc->joinDepth();

      return cnt;
    }



    void
      Acomp::setString(const std::string& Line)
      /**
      Sort out stuff like abc'+efg
      given a inner bracket expand that etc.
      @param Line :: string of for abc'.
      */
    {
      Acomp CM;            /// Complementary object
      // DELETE ALL
      deleteComp();
      Units.clear();
      Intersect=1;
      // 
      // Process #( ) sub units
      // 
      std::string Ln = Line;
      std::string::size_type sPos=Ln.find('#');

      // REMOVE ALL COMPLEMENTS
      while (sPos!=std::string::npos && Ln[sPos+1]=='(')
      {
        int bLevel(1);
        int ePos;
        for(ePos=sPos+2;bLevel>0 && ePos<static_cast<int>(Ln.size());ePos++)
        {
          if (Ln[ePos]=='(')
            bLevel++;
          else if (Ln[ePos]==')')
            bLevel--;
        }
        if (bLevel)
          throw std::invalid_argument("Acomp::setString error in line Ln\"" + Ln + "\"");
        std::string Part= Ln.substr(sPos,ePos-sPos);
        CM.setString(Ln.substr(sPos+2,ePos-sPos-3));
        CM.complement();
        Ln.replace(sPos,ePos-sPos,"("+CM.display()+")");
        sPos=Ln.find('#');
      }

      //
      // First:: Union take presidence over Intersection
      //      :: check brackets
      int blevel=0;    // bracket level
      for(unsigned int i=0;i<Ln.size();i++)
      {
        if (Ln[i]=='(')
          blevel++;
        if (Ln[i]==')')
        {
          if (!blevel)      // error condition
          {
            deleteComp();
            throw std::runtime_error(Ln);
          }
          blevel--;
        }
        if (Ln[i]=='+' && !blevel)      //must be union
          Intersect=0;
      }
      if (blevel!=0)
        throw std::runtime_error(Ln);

      if (Intersect)
        processIntersection(Ln);
      else
        processUnion(Ln);
      sort(Units.begin(),Units.end());      /// Resort the list.
      return;
    }

    std::pair<int,int>
      Acomp::size() const
      /**
      Gets the size of the Units and the Comp
      @return size of Unit, Comp
      */
    {
      return std::pair<int,int>(Units.size(),Comp.size());
    }

    int
      Acomp::itemN(const int Index) const
      /**
      Assessor function to get a unit number
      @param Index :: Number of Unit to aquire
      @return Units[Index] or 0 on failure
      */
    {
      if (Index>=0 && Index<static_cast<int>(Units.size()))
        return Units[Index];
      return 0;
    }

    const Acomp*
      Acomp::itemC(const int Index) const
      /**
      Assessor function to get a Comp points
      @param Index :: Number of Comp to aquire
      @return Comp[Index] or 0 on failure
      */
    {
      if (Index<0 || Index>=static_cast<int>(Comp.size()))
        return 0;
      return &Comp[Index];
    }


    void
      Acomp::complement() 
      /**
      Take a complement of the current object
      This will reverse the type since 
      union<->intersection as a+b -> a'b' and
      ab -> a'+b'
      */
    {
      Intersect=1-Intersect;
      transform(Units.begin(),Units.end(),
        Units.begin(),std::bind2nd(std::multiplies<int>(),-1) );
      sort(Units.begin(),Units.end());      /// Resort the list. use reverse?

      for_each(Comp.begin(),Comp.end(),
        std::mem_fun_ref(&Acomp::complement) );
      sort(Comp.begin(),Comp.end());
      return;
    }


    void
      Acomp::writeFull(std::ostream& OXF,const int Indent) const
      /**
      Real pretty print out statement  :-)
      @param OXF :: output stream
      @param Indent :: level of indentation (allows a cascaded call system)
      */
    {
      for(int i=0;i<Indent;i++)
        OXF<<" ";
      OXF<<((Intersect==1) ? "Inter" : "Union");
      OXF<<" "<<Units.size()<<" "<<Comp.size()<<std::endl;
      for(int i=0;i<Indent;i++)
        OXF<<" ";
      OXF<<display()<<std::endl;
      std::vector<Acomp>::const_iterator vc;
      for(vc=Comp.begin();vc!=Comp.end();vc++)
      {
        vc->writeFull(OXF,Indent+2);
      }
      return;
    }

    std::string
      Acomp::display() const
      /**
      Real pretty print out statement  
      @return Full string of the output in abc+efg type form
      */
    {
      std::string out;
      std::stringstream cx;
      std::vector<int>::const_iterator ic;
      int sign,val;      // sign and value of unit
      for(ic=Units.begin();ic!=Units.end();ic++)
      {
        if (!Intersect && ic!=Units.begin())
          cx<<'+';
        split(*ic,sign,val);
        if (val<27)
          cx<<static_cast<char>(static_cast<int>('a')+(val-1));
        else if (val<53)
          cx<<static_cast<char>(static_cast<int>('A')+(val-27));
        else
          cx<<"%"<<val-52;
        if (sign<0)
          cx<<'\'';
      }
      // Now do composites
      std::vector<Acomp>::const_iterator vc;
      for(vc=Comp.begin();vc!=Comp.end();vc++)
      {
        if (!Intersect && (vc!=Comp.begin() || !Units.empty()))
          cx<<'+';
        //      if ( join && (*vc)->type() )
        if ( !vc->Intersect )
          cx<<'('<<vc->display()<<')';
        else
          cx<<'('<<vc->display()<<')';
        //	cx<<vc->display();

      }
      return cx.str();
    }

    std::string
      Acomp::displayDepth(const int dval) const
      /**
      Real pretty print out statement  :-)
      @param dval :: parameter to keep track of depth
      @return Full string of print line
      */
    {
      std::string out;
      std::stringstream cx;
      std::vector<int>::const_iterator ic;
      int sign,val;      // sign and value of unit
      for(ic=Units.begin();ic!=Units.end();ic++)
      {
        if (!Intersect && ic!=Units.begin())
          cx<<'+';
        split(*ic,sign,val);
        if (val<27)
          cx<<static_cast<char>(static_cast<int>('a')+(val-1));
        else if (val<53)
          cx<<static_cast<char>(static_cast<int>('A')+(val-27));
        else
          cx<<"%"<<val-52;
        if (sign<0)
          cx<<'\'';
      }
      // Now do composites
      std::vector<Acomp>::const_iterator vc;
      for(vc=Comp.begin();vc!=Comp.end();vc++)
      {
        if (!Intersect && (vc!=Comp.begin() || !Units.empty()))
          cx<<'+';
        //      if ( join && (*vc)->type() )
        if ( !vc->Intersect )
          cx<<"D"<<dval<<" "<<
          '('<<vc->displayDepth(dval+1)<<')'<<" "<<dval<<"E";
        else
          cx<<"D"<<dval<<" "<<vc->displayDepth(dval+1)<<" "<<dval<<"E";

      }
      return cx.str();
    }

    void
      Acomp::printImplicates(const std::vector<BnId>& PIform,
      const Geometry::Matrix<int>& Grid) const
      /**
      Debug function to print out 
      PI and Grid :
      @param PIform :: Principle implicates
      @param Grid :: grid form
      */

    {
      const std::pair<int,int> RX=Grid.size();
      for(unsigned int pc=0;pc!=PIform.size();pc++)
      {
        std::cerr<<PIform[pc]<<":";
        for(int ic=0;ic!=RX.second;ic++)
          std::cerr<<((Grid[pc][ic]) ? " 1" : " 0");
        std::cerr<<std::endl;
      }
      return;
    }

  }  // NAMESPACE Geometry

}  // NAMESPACE Mantid
