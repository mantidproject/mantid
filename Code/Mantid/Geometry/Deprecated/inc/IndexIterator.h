#ifndef IndexIterator_h
#define IndexIterator_h

/*!
  \class IndexIterator
  \author S. Ansell
  \version 1.0
  \date February 2006
  \brief Allows stepping through an XML schema
  
  Maintains a stack of the positions within
  a set of XMLgroups, so that a group can be
  completed and then stepped out of
  - BaseT :: Current base object
  - GroupT :: Top level group 
*/

template <typename BaseT,typename GroupT>
class IndexIterator
{
 private:

  const BaseT* CPtr;           /// Pointer to the current object
  const GroupT* Master;        ///< Pointer to the top object 
  std::stack<int> Pos;         ///< Positions in the stack
  std::list<const GroupT*> XG;
  
 public:
  
  IndexIterator(const GroupT* Bptr) 
    /*!
      Constructor :
      \param Bptr :: Base pointer 
     */
    { 
      Master=Bptr;
      init();
      this->operator++();
    }
  
  /// Copy constructor
  IndexIterator(const IndexIterator<BaseT,GroupT>& A) :
    CPtr(A.CPtr),Master(A.Master),Pos(A.Pos),XG(A.XG) {}

  IndexIterator&
  operator=(const IndexIterator<BaseT,GroupT>& A) 
    /*!
      Assignement operator=
      \param A :: 
     */
    {
      if (this!=&A)
	{
	  CPtr=A.CPtr;
	  Master=A.Master;
	  Pos=A.Pos;
	  XG=A.XG;
	}
      return *this;
    }

  /// Destructor
  ~IndexIterator() {}
  
  void init()
    /*!
      Initialise the stack.
      Take the current position in the XMLgroup 
    */
    {
      CPtr=0;
      XG.clear();
      Pos=std::stack<int>();
      XG.push_back(Master);
      Pos.push(-1);
      operator++();
      return;
    }

  int operator++(const int)
    /*!
      Get the next object
      \retval 0 :: normal object
      \retval 1 :: looped over
    */
    {
      return operator++();
    }

  int operator++() 
    /*!
      Get the next object / group :: Updated from the original which
      only did objects
      \retval 0 :: new object has be found
      \reval 1 :: close group
      \retval -1 :: re-init
    */
    {
      const GroupT* Gptr(0);
      CPtr=XG.back()->getItem(++Pos.top());  // add one to the top item on index-stack 
      Gptr=dynamic_cast<const GroupT*>(CPtr); // determine if it is a group
      if (!CPtr)     // Looped over:  (i.e the current group is finished
	{
	  Pos.pop();
	  XG.pop_back();
	  if (Pos.empty())
	    {
	      init();
	      return -1;
	    }
	}
      else if (Gptr)          // Found a new group
	{
	  Pos.push(-1);
	  XG.push_back(Gptr);
	}
      return 0;
    }

  /// Get the number depth of items
  int getLevel() const { return Pos.size(); } 
  
  /// Accessor to the base object
  const BaseT* operator*() const
    {
      return CPtr;
    }
  /// Accessor to the base object
  const BaseT* operator->()
    {
      return CPtr;
    }
  
};


#endif
  
