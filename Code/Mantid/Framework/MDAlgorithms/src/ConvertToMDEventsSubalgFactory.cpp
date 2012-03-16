#include "MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h"


namespace Mantid
{
namespace MDAlgorithms
{


/** Get access to a subalgorithm defined by its name
  * @param   AlgName -- symbolic name, which defines the algorithm
  * @returns    Pointer fo the algorithm, which do the conversion
  *
  * Thows invalid_agrument if subalgorithm, defined by the name is not exist (not initated);
*/
IConvertToMDEventsMethods * ConvertToMDEventsSubalgFactory::getAlg(const std::string &AlgName)
{
    auto algoIt  = alg_selector.find(AlgName);
    if (algoIt == alg_selector.end())
    {
        std::string Error = "Undefined subalgoritm: "+AlgName+" requested ";
        throw(std::invalid_argument(Error));
    }
    return algoIt->second;

}

ConvertToMDEventsSubalgFactory::ConvertToMDEventsSubalgFactory()
{
}
ConvertToMDEventsSubalgFactory::~ConvertToMDEventsSubalgFactory()
{
    // clear all initated subalgorithms
    auto it = alg_selector.begin();
    for(; it!=alg_selector.end();++it){
        delete it->second;  
    }
    alg_selector.clear();
}


// TEMPLATES INSTANTIATION: User encouraged to specialize its own specific algorithm 
//e.g.
// template<> void ConvertToMDEvents::processQND<modQ,Elastic,ConvertNo>()
// {
//   User specific code for workspace  processed to obtain modQ in elastic mode, without unit conversion:
// }
//----------------------------------------------------------------------------------------------
// AUTOINSTANSIATION OF EXISTING CODE:
/** helper class to orginize metaloop instantiating various subalgorithms dealing with particular 
  * workspaces and implementing particular user requests */
template<Q_state Q, size_t NumAlgorithms=0>
class LOOP_ALGS{
private:
    enum{
        CONV = NumAlgorithms%NConvUintsStates,           // internal oop over conversion modes, the variable changes first
        MODE = (NumAlgorithms/NConvUintsStates)%ANY_Mode // one level up loop over momentum conversion mode  
    
    };
  public:
    static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){
        // cast loop integers to proper enum type
        CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);
        AnalMode   Mode = static_cast<AnalMode>(MODE);

        std::string  Key = AlgoKey.getAlgoID(Q,Mode,Conv,Ws2DRuggedType);
        pH->alg_selector.insert(std::pair<std::string, IConvertToMDEventsMethods *>(Key,
                (new ConvertToMDEventsWS<Ws2DRuggedType,Q,static_cast<AnalMode>(MODE),static_cast<CnvrtUnits>(CONV)>())));

        Key = AlgoKey.getAlgoID(Q,Mode,Conv,EventWSType);
        pH->alg_selector.insert(std::pair<std::string, IConvertToMDEventsMethods *>(Key,
            (new  ConvertToMDEventsWS<EventWSType,Q,static_cast<AnalMode>(MODE),static_cast<CnvrtUnits>(CONV)>())));

/*#ifdef _DEBUG
            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
#endif*/
            LOOP_ALGS<Q, NumAlgorithms+1>::EXEC(AlgoKey,pH);
    }
};

/** Templated metaloop specialization for noQ case */
template< size_t NumAlgorithms>
class LOOP_ALGS<NoQ,NumAlgorithms>{
private:
    enum{
        CONV = NumAlgorithms%NConvUintsStates       // internal Loop over conversion modes, the variable changes first
        //MODE => noQ -- no mode conversion ANY_Mode, 
    
    };
  public:
    static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){

      // cast loop integers to proper enum type
      CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);

      std::string  Key0  = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,Ws2DRuggedType);
      pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key0,
                         (new ConvertToMDEventsWS<Ws2DRuggedType,NoQ,ANY_Mode,static_cast<CnvrtUnits>(CONV)>())));

       std::string  Key1 = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,EventWSType);
       pH->alg_selector.insert(std::pair<std::string, IConvertToMDEventsMethods *>(Key1,
                       (new ConvertToMDEventsWS<Ws2DRuggedType,NoQ,ANY_Mode,static_cast<CnvrtUnits>(CONV)>())));

           
//#ifdef _DEBUG
//            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
//#endif

             LOOP_ALGS<NoQ,NumAlgorithms+1>::EXEC(AlgoKey,pH);
    }
};

/** Q3d and modQ metaloop terminator */
template<Q_state Q >
class LOOP_ALGS<Q,static_cast<size_t>(ANY_Mode*NConvUintsStates) >{
  public:
      static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {
          UNUSED_ARG(AlgoKey);UNUSED_ARG(pH);
      } 
};

/** ANY_Mode (NoQ) metaloop terminator */
template<>
class LOOP_ALGS<NoQ,static_cast<size_t>(NConvUintsStates) >{
  public:
      static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {          
          UNUSED_ARG(AlgoKey);UNUSED_ARG(pH);
      } 
};
//-------------------------------------------------------------------------------------------------------------------------------
/** Function Initiate the subalgorithm creation for further possible usage
  * @param SubAlgDescriptor  -- the class which has infromation about existing subalgorthms and used to generate subalgorithms keys.
*/
void ConvertToMDEventsSubalgFactory::initSubalgorithms(const ConvertToMDEventsParams &SubAlgDescriptor)
{
    if (alg_selector.empty())
    {
 // Instanciate the subalgorithms for different cases
 // NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
        LOOP_ALGS<NoQ,0>::EXEC(SubAlgDescriptor,this); 
    // MOD Q
        LOOP_ALGS<modQ,0>::EXEC(SubAlgDescriptor,this);
    // Q3D
        LOOP_ALGS<Q3D,0>::EXEC(SubAlgDescriptor,this);
    }

}

} // endnamespace MDAlgorithms
} // endnamespace Mantid