#include "MantidMDAlgorithms/ConvertToMDEventsChildAlgFactory.h"
/**TODO: FOR DEPRICATION */ 




using namespace Mantid::MDAlgorithms::ConvertToMD;

namespace Mantid
{
namespace MDAlgorithms
{


/** Get access to a ChildAlgorithm defined by its name
  * @param   AlgName -- symbolic name, which defines the algorithm
  * @returns    Pointer fo the algorithm, which do the conversion
  *
  * Thows invalid_agrument if ChildAlgorithm, defined by the name is not exist (not initated);
*/
ConvertToMDEventsWSBase* ConvertToMDEventsChildAlgFactory::getAlg(const std::string &AlgName)
{
    auto algoIt  = alg_selector.find(AlgName);
    if (algoIt == alg_selector.end())
    {
        std::string Error = "Undefined ChildAlgoritm: "+AlgName+" requested ";
        throw(std::invalid_argument(Error));
    }
    return algoIt->second;

}

ConvertToMDEventsChildAlgFactory::ConvertToMDEventsChildAlgFactory()
{
}
ConvertToMDEventsChildAlgFactory::~ConvertToMDEventsChildAlgFactory()
{
    // clear all initated ChildAlgorithms
    auto it = alg_selector.begin();
    for(; it!=alg_selector.end();++it){
        delete it->second;  
    }
    alg_selector.clear();
}


//----------------------------------------------------------------------------------------------
// AUTOINSTANTIATION OF EXISTING CODE:
/** helper class to orginize metaloop instansiating various ChildAlgorithms dealing with particular 
  * workspaces and implementing particular user requests */
template<QMode Q, size_t AlgoNum>
class LOOP_ALGS{
private:
    enum{
        CONV = AlgoNum%NConvUintsStates,                        // internal loop over conversion modes, the variable changes first
        MODE = (AlgoNum/NConvUintsStates)%ANY_Mode,             // level one loop over momentum conversion mode  
        SAMPL= ((AlgoNum/NConvUintsStates)/ANY_Mode)%NSampleTypes, //level two loop over crystal or powder sample
        WS   = (((AlgoNum/NConvUintsStates)/ANY_Mode)/NSampleTypes)%NInWSTypes //level three loop over ws type;
    
    };
  public:
    static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsChildAlgFactory *pH){
        // cast loop integers to proper enum type
        CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);
        AnalMode   Mode = static_cast<AnalMode>(MODE);
        SampleType Sampl= static_cast<SampleType>(SAMPL);
        InputWSType Ws  = static_cast<InputWSType>(WS);

        std::string  Key = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
        pH->alg_selector.insert(std::pair<std::string, ConvertToMDEventsWSBase *>(Key,
                (new ConvertToMDEventsWS<InputWSType(WS),Q,AnalMode(MODE),
                 CnvrtUnits(CONV),SampleType(SAMPL)>())));

/*#ifdef _DEBUG
            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
#endif*/
            LOOP_ALGS<Q, AlgoNum-1>::EXEC(AlgoKey,pH);
    }
};

/** Templated metaloop specialization for noQ case */
template< size_t AlgoNum>
class LOOP_ALGS<NoQ,AlgoNum>{
private:
    enum{
        CONV = AlgoNum%NConvUintsStates,       // internal Loop over conversion modes, the variable changes first
        WS   = ((AlgoNum/NConvUintsStates))%NInWSTypes  //level one loop over ws type;
        //MODE => noQ -- no mode conversion ANY_Mode,     
    };
  public:
    static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsChildAlgFactory *pH){

      // cast loop integers to proper enum type
      CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);
      InputWSType Ws  = static_cast<InputWSType>(WS);

      std::string  Key   = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,Ws,NSampleTypes);
      pH->alg_selector.insert(std::pair<std::string,ConvertToMDEventsWSBase *>(Key,
         (new ConvertToMDEventsWS<InputWSType(WS),ConvertToMD::NoQ,ANY_Mode,CnvrtUnits(CONV),NSampleTypes>())));
           
//#ifdef _DEBUG
//            std::cout<<" Instantiating algorithm with ID: "<<Key<<std::endl;
//#endif

             LOOP_ALGS<ConvertToMD::NoQ,AlgoNum-1>::EXEC(AlgoKey,pH);
    }
};

//static_cast<size_t>(ANY_Mode*NConvUintsStates)
/** Q3d and ModQ metaloop terminator */
template<ConvertToMD::QMode Q >
class LOOP_ALGS<Q,0>{
  public:
      static inline void EXEC(const ConvertToMD::ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsChildAlgFactory *pH)
      {
          CnvrtUnits Conv = static_cast<CnvrtUnits>(0);
          AnalMode   Mode = static_cast<AnalMode>(0);
          SampleType Sampl= static_cast<SampleType>(0);
          InputWSType Ws  = static_cast<InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
          pH->alg_selector.insert(std::pair<std::string,ConvertToMDEventsWSBase *>(Key,
            (new ConvertToMDEventsWS<InputWSType(0),Q,AnalMode(0),CnvrtUnits(0),SampleType(0)>())));     
      } 
};

/** ANY_Mode (NoQ) metaloop terminator */
template<>
class LOOP_ALGS<ConvertToMD::NoQ,0>{
  public:
      static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsChildAlgFactory *pH)
      {     
          CnvrtUnits Conv = static_cast<CnvrtUnits>(0);
          InputWSType Ws  = static_cast<InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,Ws,NSampleTypes);
          pH->alg_selector.insert(std::pair<std::string,ConvertToMDEventsWSBase *>(Key,
          (new ConvertToMDEventsWS<InputWSType(0),NoQ,ANY_Mode,CnvrtUnits(0),NSampleTypes>())));

      } 
};

/** initiate the ChildAlgorithms and made them availible for getAlg function
  * @param ChildAlgDescriptor  -- the class which has infromation about existing ChildAlgorthms and used to generate ChildAlgorithms keys.
*/
void ConvertToMDEventsChildAlgFactory::init(const ConvertToMD::ConvertToMDEventsParams &ChildAlgDescriptor)
{
    if (alg_selector.empty()) // Instantiate the ChildAlgorithms for different cases
    {    
    // NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
        LOOP_ALGS<NoQ,NInWSTypes*NConvUintsStates>::EXEC(ChildAlgDescriptor,this); 
    // MOD Q
       LOOP_ALGS<ModQ,NInWSTypes*NConvUintsStates*ANY_Mode*NSampleTypes>::EXEC(ChildAlgDescriptor,this);
    // Q3D
        LOOP_ALGS<Q3D,NInWSTypes*NConvUintsStates*ANY_Mode*NSampleTypes>::EXEC(ChildAlgDescriptor,this);
    }

}

//
} // endnamespace MDAlgorithms
} // endnamespace Mantid