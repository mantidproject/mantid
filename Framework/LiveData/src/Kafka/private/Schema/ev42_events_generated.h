// clang-format off
// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_EV42EVENTS_H_
#define FLATBUFFERS_GENERATED_EV42EVENTS_H_

#include "flatbuffers/flatbuffers.h"

#include "dtdb_adc_pulse_debug_generated.h"
#include "is84_isis_events_generated.h"

struct EventMessage;

enum class FacilityData : uint8_t {
  NONE = 0,
  ISISData = 1,
  AdcPulseDebug = 2,
  MIN = NONE,
  MAX = AdcPulseDebug
};

inline const FacilityData (&EnumValuesFacilityData())[3] {
  static const FacilityData values[] = {
    FacilityData::NONE,
    FacilityData::ISISData,
    FacilityData::AdcPulseDebug
  };
  return values;
}

inline const char * const *EnumNamesFacilityData() {
  static const char * const names[] = {
    "NONE",
    "ISISData",
    "AdcPulseDebug",
    nullptr
  };
  return names;
}

inline const char *EnumNameFacilityData(FacilityData e) {
  const size_t index = static_cast<int>(e);
  return EnumNamesFacilityData()[index];
}

template<typename T> struct FacilityDataTraits {
  static const FacilityData enum_value = FacilityData::NONE;
};

template<> struct FacilityDataTraits<ISISData> {
  static const FacilityData enum_value = FacilityData::ISISData;
};

template<> struct FacilityDataTraits<AdcPulseDebug> {
  static const FacilityData enum_value = FacilityData::AdcPulseDebug;
};

bool VerifyFacilityData(flatbuffers::Verifier &verifier, const void *obj, FacilityData type);
bool VerifyFacilityDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types);

struct EventMessage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
    return "EventMessage";
  }
  enum {
    VT_SOURCE_NAME = 4,
    VT_MESSAGE_ID = 6,
    VT_PULSE_TIME = 8,
    VT_TIME_OF_FLIGHT = 10,
    VT_DETECTOR_ID = 12,
    VT_FACILITY_SPECIFIC_DATA_TYPE = 14,
    VT_FACILITY_SPECIFIC_DATA = 16
  };
  const flatbuffers::String *source_name() const {
    return GetPointer<const flatbuffers::String *>(VT_SOURCE_NAME);
  }
  flatbuffers::String *mutable_source_name() {
    return GetPointer<flatbuffers::String *>(VT_SOURCE_NAME);
  }
  uint64_t message_id() const {
    return GetField<uint64_t>(VT_MESSAGE_ID, 0);
  }
  bool mutate_message_id(uint64_t _message_id) {
    return SetField<uint64_t>(VT_MESSAGE_ID, _message_id, 0);
  }
  uint64_t pulse_time() const {
    return GetField<uint64_t>(VT_PULSE_TIME, 0);
  }
  bool mutate_pulse_time(uint64_t _pulse_time) {
    return SetField<uint64_t>(VT_PULSE_TIME, _pulse_time, 0);
  }
  const flatbuffers::Vector<uint32_t> *time_of_flight() const {
    return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_TIME_OF_FLIGHT);
  }
  flatbuffers::Vector<uint32_t> *mutable_time_of_flight() {
    return GetPointer<flatbuffers::Vector<uint32_t> *>(VT_TIME_OF_FLIGHT);
  }
  const flatbuffers::Vector<uint32_t> *detector_id() const {
    return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_DETECTOR_ID);
  }
  flatbuffers::Vector<uint32_t> *mutable_detector_id() {
    return GetPointer<flatbuffers::Vector<uint32_t> *>(VT_DETECTOR_ID);
  }
  FacilityData facility_specific_data_type() const {
    return static_cast<FacilityData>(GetField<uint8_t>(VT_FACILITY_SPECIFIC_DATA_TYPE, 0));
  }
  bool mutate_facility_specific_data_type(FacilityData _facility_specific_data_type) {
    return SetField<uint8_t>(VT_FACILITY_SPECIFIC_DATA_TYPE, static_cast<uint8_t>(_facility_specific_data_type), 0);
  }
  const void *facility_specific_data() const {
    return GetPointer<const void *>(VT_FACILITY_SPECIFIC_DATA);
  }
  template<typename T> const T *facility_specific_data_as() const;
  const ISISData *facility_specific_data_as_ISISData() const {
    return facility_specific_data_type() == FacilityData::ISISData ? static_cast<const ISISData *>(facility_specific_data()) : nullptr;
  }
  const AdcPulseDebug *facility_specific_data_as_AdcPulseDebug() const {
    return facility_specific_data_type() == FacilityData::AdcPulseDebug ? static_cast<const AdcPulseDebug *>(facility_specific_data()) : nullptr;
  }
  void *mutable_facility_specific_data() {
    return GetPointer<void *>(VT_FACILITY_SPECIFIC_DATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SOURCE_NAME) &&
           verifier.VerifyString(source_name()) &&
           VerifyField<uint64_t>(verifier, VT_MESSAGE_ID) &&
           VerifyField<uint64_t>(verifier, VT_PULSE_TIME) &&
           VerifyOffset(verifier, VT_TIME_OF_FLIGHT) &&
           verifier.VerifyVector(time_of_flight()) &&
           VerifyOffset(verifier, VT_DETECTOR_ID) &&
           verifier.VerifyVector(detector_id()) &&
           VerifyField<uint8_t>(verifier, VT_FACILITY_SPECIFIC_DATA_TYPE) &&
           VerifyOffset(verifier, VT_FACILITY_SPECIFIC_DATA) &&
           VerifyFacilityData(verifier, facility_specific_data(), facility_specific_data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const ISISData *EventMessage::facility_specific_data_as<ISISData>() const {
  return facility_specific_data_as_ISISData();
}

template<> inline const AdcPulseDebug *EventMessage::facility_specific_data_as<AdcPulseDebug>() const {
  return facility_specific_data_as_AdcPulseDebug();
}

struct EventMessageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_source_name(flatbuffers::Offset<flatbuffers::String> source_name) {
    fbb_.AddOffset(EventMessage::VT_SOURCE_NAME, source_name);
  }
  void add_message_id(uint64_t message_id) {
    fbb_.AddElement<uint64_t>(EventMessage::VT_MESSAGE_ID, message_id, 0);
  }
  void add_pulse_time(uint64_t pulse_time) {
    fbb_.AddElement<uint64_t>(EventMessage::VT_PULSE_TIME, pulse_time, 0);
  }
  void add_time_of_flight(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> time_of_flight) {
    fbb_.AddOffset(EventMessage::VT_TIME_OF_FLIGHT, time_of_flight);
  }
  void add_detector_id(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> detector_id) {
    fbb_.AddOffset(EventMessage::VT_DETECTOR_ID, detector_id);
  }
  void add_facility_specific_data_type(FacilityData facility_specific_data_type) {
    fbb_.AddElement<uint8_t>(EventMessage::VT_FACILITY_SPECIFIC_DATA_TYPE, static_cast<uint8_t>(facility_specific_data_type), 0);
  }
  void add_facility_specific_data(flatbuffers::Offset<void> facility_specific_data) {
    fbb_.AddOffset(EventMessage::VT_FACILITY_SPECIFIC_DATA, facility_specific_data);
  }
  explicit EventMessageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  EventMessageBuilder &operator=(const EventMessageBuilder &);
  flatbuffers::Offset<EventMessage> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<EventMessage>(end);
    return o;
  }
};

inline flatbuffers::Offset<EventMessage> CreateEventMessage(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> source_name = 0,
    uint64_t message_id = 0,
    uint64_t pulse_time = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint32_t>> time_of_flight = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint32_t>> detector_id = 0,
    FacilityData facility_specific_data_type = FacilityData::NONE,
    flatbuffers::Offset<void> facility_specific_data = 0) {
  EventMessageBuilder builder_(_fbb);
  builder_.add_pulse_time(pulse_time);
  builder_.add_message_id(message_id);
  builder_.add_facility_specific_data(facility_specific_data);
  builder_.add_detector_id(detector_id);
  builder_.add_time_of_flight(time_of_flight);
  builder_.add_source_name(source_name);
  builder_.add_facility_specific_data_type(facility_specific_data_type);
  return builder_.Finish();
}

inline flatbuffers::Offset<EventMessage> CreateEventMessageDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *source_name = nullptr,
    uint64_t message_id = 0,
    uint64_t pulse_time = 0,
    const std::vector<uint32_t> *time_of_flight = nullptr,
    const std::vector<uint32_t> *detector_id = nullptr,
    FacilityData facility_specific_data_type = FacilityData::NONE,
    flatbuffers::Offset<void> facility_specific_data = 0) {
  return CreateEventMessage(
      _fbb,
      source_name ? _fbb.CreateString(source_name) : 0,
      message_id,
      pulse_time,
      time_of_flight ? _fbb.CreateVector<uint32_t>(*time_of_flight) : 0,
      detector_id ? _fbb.CreateVector<uint32_t>(*detector_id) : 0,
      facility_specific_data_type,
      facility_specific_data);
}

inline bool VerifyFacilityData(flatbuffers::Verifier &verifier, const void *obj, FacilityData type) {
  switch (type) {
    case FacilityData::NONE: {
      return true;
    }
    case FacilityData::ISISData: {
      auto ptr = reinterpret_cast<const ISISData *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case FacilityData::AdcPulseDebug: {
      auto ptr = reinterpret_cast<const AdcPulseDebug *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return false;
  }
}

inline bool VerifyFacilityDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyFacilityData(
        verifier,  values->Get(i), types->GetEnum<FacilityData>(i))) {
      return false;
    }
  }
  return true;
}

inline const EventMessage *GetEventMessage(const void *buf) {
  return flatbuffers::GetRoot<EventMessage>(buf);
}

inline const EventMessage *GetSizePrefixedEventMessage(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<EventMessage>(buf);
}

inline EventMessage *GetMutableEventMessage(void *buf) {
  return flatbuffers::GetMutableRoot<EventMessage>(buf);
}

inline const char *EventMessageIdentifier() {
  return "ev42";
}

inline bool EventMessageBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, EventMessageIdentifier());
}

inline bool VerifyEventMessageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<EventMessage>(EventMessageIdentifier());
}

inline bool VerifySizePrefixedEventMessageBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<EventMessage>(EventMessageIdentifier());
}

inline void FinishEventMessageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<EventMessage> root) {
  fbb.Finish(root, EventMessageIdentifier());
}

inline void FinishSizePrefixedEventMessageBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<EventMessage> root) {
  fbb.FinishSizePrefixed(root, EventMessageIdentifier());
}

#endif  // FLATBUFFERS_GENERATED_EV42EVENTS_H_
