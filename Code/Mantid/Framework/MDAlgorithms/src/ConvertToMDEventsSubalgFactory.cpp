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
template<Q_state Q, size_t AlgoNum>
class LOOP_ALGS{
private:
    enum{
        CONV = AlgoNum%NConvUintsStates,                        // internal loop over conversion modes, the variable changes first
        MODE = (AlgoNum/NConvUintsStates)%ANY_Mode,             // level one loop over momentum conversion mode  
        SAMPL= ((AlgoNum/NConvUintsStates)/ANY_Mode)%NSampleTypes, //level two loop over crystal or powder sample
        WS   = (((AlgoNum/NConvUintsStates)/ANY_Mode)/NSampleTypes)%NInWSTypes //level three loop ws type;
    
    };
  public:
    static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){
        // cast loop integers to proper enum type
        CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);
        AnalMode   Mode = static_cast<AnalMode>(MODE);
        SampleType Sampl= static_cast<SampleType>(SAMPL);
        InputWSType Ws  = static_cast<InputWSType>(WS);

        std::string  Key = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
        pH->alg_selector.insert(std::pair<std::string, IConvertToMDEventsMethods *>(Key,
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
    static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH){

      // cast loop integers to proper enum type
      CnvrtUnits Conv = static_cast<CnvrtUnits>(CONV);
      InputWSType Ws  = static_cast<InputWSType>(WS);

      std::string  Key   = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,Ws,NSampleTypes);
      pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
         (new ConvertToMDEventsWS<InputWSType(WS),NoQ,ANY_Mode,CnvrtUnits(CONV),NSampleTypes>())));
           
//#ifdef _DEBUG
//            std::cout<<" Instansiating algorithm with ID: "<<Key<<std::endl;
//#endif

             LOOP_ALGS<NoQ,AlgoNum-1>::EXEC(AlgoKey,pH);
    }
};

//static_cast<size_t>(ANY_Mode*NConvUintsStates)
/** Q3d and modQ metaloop terminator */
template<Q_state Q >
class LOOP_ALGS<Q,0>{
  public:
      static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {
          CnvrtUnits Conv = static_cast<CnvrtUnits>(0);
          AnalMode   Mode = static_cast<AnalMode>(0);
          SampleType Sampl= static_cast<SampleType>(0);
          InputWSType Ws  = static_cast<InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(Q,Mode,Conv,Ws,Sampl);
          pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
            (new ConvertToMDEventsWS<InputWSType(0),Q,AnalMode(0),CnvrtUnits(0),SampleType(0)>())));     
      } 
};

/** ANY_Mode (NoQ) metaloop terminator */
template<>
class LOOP_ALGS<NoQ,0>{
  public:
      static inline void EXEC(const ConvertToMDEventsParams &AlgoKey,ConvertToMDEventsSubalgFactory *pH)
      {     
          CnvrtUnits Conv = static_cast<CnvrtUnits>(0);
          InputWSType Ws  = static_cast<InputWSType>(0);

          std::string  Key   = AlgoKey.getAlgoID(NoQ,ANY_Mode,Conv,Ws,NSampleTypes);
          pH->alg_selector.insert(std::pair<std::string,IConvertToMDEventsMethods *>(Key,
          (new ConvertToMDEventsWS<InputWSType(0),NoQ,ANY_Mode,CnvrtUnits(0),NSampleTypes>())));

      } 
};

/** initiate the subalgorithms and made them availible for getAlg function
  * @param SubAlgDescriptor  -- the class which has infromation about existing subalgorthms and used to generate subalgorithms keys.
*/
void ConvertToMDEventsSubalgFactory::init(const ConvertToMDEventsParams &SubAlgDescriptor)
{
    if (alg_selector.empty()) // Instanciate the subalgorithms for different cases
    {    
    // NoQ --> any Analysis mode will do as it does not depend on it; we may want to convert unuts
        LOOP_ALGS<NoQ,NInWSTypes*NConvUintsStates>::EXEC(SubAlgDescriptor,this); 
    // MOD Q
        LOOP_ALGS<modQ,NInWSTypes*NConvUintsStates*ANY_Mode*NSampleTypes>::EXEC(SubAlgDescriptor,this);
    // Q3D
        LOOP_ALGS<Q3D,NInWSTypes*NConvUintsStates*ANY_Mode*NSampleTypes>::EXEC(SubAlgDescriptor,this);
    }

}

} // endnamespace MDAlgorithms
} // endnamespace Mantid