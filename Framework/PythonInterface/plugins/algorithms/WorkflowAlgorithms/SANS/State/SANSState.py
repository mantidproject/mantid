# import copy
# from SANSStateBase import SANSStateBase
# from SANSStateData import SANSStateData
#
#
# # -----------------------------------------------
# # SANSState class
# # -----------------------------------------------
# class SANSState(object, SANSStateBase):
#     def __init__(self):
#         super(SANSState, self).__init__()
#         self._data = None
#         self._mask = None
#
#     def copy(self):
#         return copy.copy(self)
#
#     @property
#     def property_manager(self):
#         pass
#
#     @property_manager.setter
#     def property_manager(self, value):
#         pass
#
#     def validate(self):
#         pass
#
#     def set_state(self, data):
#         if SANSState._sub_state_is_of_correct_type(data, SANSStateData):
#             self._data = data
#
#     @property
#     def data(self):
#         pass
#
#     @staticmethod
#     def _sub_state_is_of_correct_type(value, expected_type):
#         if value is None or not isinstance(value, expected_type):
#             raise ValueError("SANSState: Expected a {} object, instead received a {} object".format(type(expected_type),
#                                                                                                     type(value)))
#
#     def get_property_manager(self):
#         pass
