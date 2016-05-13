# from mantid.api import FileFinder
# from mantid.kernel import DateAndTime
# import h5py as h5
# import abc
#
#
# # ------------------------------------
# # Types
# # ------------------------------------
# class SANSFileType(object):
#     class ISISNexus(object):
#         pass
#
#     class ISISNexusAdded(object):
#         pass
#
#     class ISISRaw(object):
#         pass
#
#     class None(object):
#         pass
#
#
# class SANSInstrument(object):
#     class LOQ(object):
#         pass
#
#     class LARMOR(object):
#         pass
#
#     class SANS2D(object):
#         pass
#
#     class None(object):
#         pass
#
#
# # -----------------------------------
# # Free Functions
# # -----------------------------------
# def find_sans_file(file_name):
#     """
#     The file can be specified as:
#     1. file.ext or \path1\path2\file.ext
#     2. run number
#     """
#     full_path = FileFinder.getFullPath(file_name)
#     if not full_path:
#         runs = FileFinder.findRuns(full_path)
#         full_path = runs[0]
#     return full_path
#
#
# # ISIS Nexus
# def get_isis_nexus_info(file_name):
#     with h5.File(file_name) as h5_file:
#         keys = h5_file.keys()
#         is_isis_nexus = "raw_data_1" in keys
#         number_of_periods = len(keys)
#     return is_isis_nexus, number_of_periods
#
#
# def is_isis_nexus_single_period(file_name):
#     is_isis_nexus, number_of_periods = get_isis_nexus_info(file_name)
#     return is_isis_nexus and number_of_periods == 1
#
#
# def is_isis_nexus_multi_period(file_name):
#     is_isis_nexus, number_of_periods = get_isis_nexus_info(file_name)
#     return is_isis_nexus and number_of_periods > 1
#
#
# def get_number_periods_for_isis_nexus(file_name):
#     is_isis_nexus, number_of_periods = get_isis_nexus_info(file_name)
#     return number_of_periods if is_isis_nexus else 0
#
#
# def get_instrument_name_for_isis_nexus(file_name):
#     """
#     Instrument inforamtion is
#     file|
#         |--mantid_workspace_1/raw_data_1|
#                                         |--instrument|
#                                                      |--name
#     """
#     with h5.File(file_name) as h5_file:
#         # Open first entry
#         keys = h5_file.keys()
#         first_entry = h5_file[keys[0]]
#         # Open instrument group
#         instrument_group = first_entry["instrument"]
#         # Open name data set
#         name_data_set = instrument_group["name"]
#         # Read value
#         instrument_name = name_data_set[0]
#     return instrument_name
#
#
# def get_date_for_isis_nexus(file_name):
#     with h5.File(file_name) as h5_file:
#         # Open first entry
#         keys = h5_file.keys()
#         first_entry = h5_file[keys[0]]
#         # Open Start time data set
#         start_time = first_entry["start_time"]
#         value = start_time[0]
#     return DateAndTime(value)
#
#
# # ISIS Raw
# # TODO add raw
# def get_instrument(instrument_name):
#     instrument_name = instrument_name.upper()
#     if instrument_name == "SANS2D":
#         instrument = SANSInstrument.SANS2D
#     elif instrument_name == "LARMOR":
#         instrument = SANSInstrument.LARMOR
#     elif instrument_name == "LOQ":
#         instrument = SANSInstrument.LOQ
#     elif instrument_name == "ANSTO":
#         instrument = SANSInstrument.ANSTO
#     else:
#         instrument = SANSInstrument.None
#     return instrument
#
#
# # -----------------------------------------------
# # Classes
# # -----------------------------------------------
# class SANSFileInformation(abc):
#     @staticmethod
#     @abc.abstractmethod
#     def get_instrument(file_name):
#         pass
#
#     @staticmethod
#     @abc.abstractmethod
#     def get_date(file_name):
#         pass
#
#     @staticmethod
#     @abc.abstractmethod
#     def get_number_of_periods(file_name):
#         pass
#
#     @staticmethod
#     @abc.abstractmethod
#     def get_type():
#         pass
#
#     @staticmethod
#     def get_full_file_name(file_name):
#         return find_sans_file(file_name)
#
#
# class SANSFileInformationISISNexus(SANSFileInformation):
#     def __init__(self):
#         super(SANSFileInformationISISNexus, self).__init__()
#
#     @staticmethod
#     def get_instrument(file_name):
#         full_file_name = SANSFileInformation.get_full_file_name(file_name)
#         instrument_name = get_instrument_name_for_isis_nexus(full_file_name)
#         return get_instrument(instrument_name)
#
#     @staticmethod
#     def get_date(file_name):
#         full_file_name = SANSFileInformation.get_full_file_name(file_name)
#         return get_date_for_isis_nexus(full_file_name)
#
#     @staticmethod
#     def get_number_of_periods(file_name):
#         full_file_name = SANSFileInformation.get_full_file_name(file_name)
#         return get_number_periods_for_isis_nexus(full_file_name)
#
#     @staticmethod
#     @abc.abstractmethod
#     def get_type():
#         return SANSFileType.ISISNexus
#
#
# class SANSFileInformationRaw(SANSFileInformation):
#     def __init__(self):
#         super(SANSFileInformationRaw, self).__init__()
#
#     @staticmethod
#     def get_instrument(file_name):
#         pass
#
#     @staticmethod
#     def get_date(file_name):
#         pass
#
#     @staticmethod
#     def get_number_of_periods(file_name):
#         pass
#
#     @staticmethod
#     @abc.abstractmethod
#     def get_type():
#         return SANSFileType.ISISRaw
#
#
# class SANSFileInformationFactory(object):
#     def __init__(self):
#         super(SANSFileInformationFactory, self).__init__()
#
#     def create_sans_file_information(self, file_name):
#         full_file_name = find_sans_file(file_name)
#         if is_isis_nexus_single_period(full_file_name) or is_isis_nexus_multi_period(full_file_name):
#             file_information = SANSFileInformationISISNexus()
#         else:
#             raise NotImplementedError("Not Implemented yet") # TODO add more file types
#         return file_information
