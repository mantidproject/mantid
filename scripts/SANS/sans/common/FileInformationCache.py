# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Caches the results from File Finder """
import os

from mantid import config


class CachedRequest(object):
    def __init__(self, requested_name, found_path):
        self.requested_name = requested_name
        self.found_path = found_path


class FileInformationCache(object):
    def __init__(self, max_len):
        self._max_len = max_len

        self._reset()

    def add_new_element(self, cached_request):
        """
        Adds a new found run to the cache, removing the least recently used one if it exceeds the maximum length
        :param cached_request: The cached request object to insert
        :return: None
        """
        assert(isinstance(cached_request, CachedRequest))

        self._cached_entries[cached_request.requested_name] = cached_request
        self._update_element_time(cached_request.requested_name)

        if len(self._cached_entries) > self._max_len:
            self._pop_old_element()

    def get_element(self, requested_name):
        """
        Returns a cached file information entry if it is present in the cache and the file still exists
        :param requested_name: The name to be passed to file finder
        :return: The associated file path, if it still exists. Or None
        """
        self._check_data_dirs_havent_changed()

        if requested_name not in self._cached_entries:
            return None

        found_entry = self._cached_entries[requested_name]

        if not os.path.exists(found_entry.found_path):
            self._del_element(requested_name)
            return None

        self._update_element_time(requested_name)
        return found_entry

    def _check_data_dirs_havent_changed(self):
        # If data dirs change we should reset the cache so unexpected results (such as a file from
        # an old dir appearing) happen
        returned = list(config.getDataSearchDirs())
        if returned != self._data_search_dirs:
            self._reset()

    def _del_element(self, element_name):
        del self._cached_entries[element_name]
        self._access_order.remove(element_name)

    def _pop_old_element(self):
        if len(self._access_order) == 0:
            return

        element_name = self._access_order.pop(0)
        del self._cached_entries[element_name]

    def _reset(self):
        self._cached_entries = {}
        self._access_order = []
        self._data_search_dirs = list(config.getDataSearchDirs())

    def _update_element_time(self, element_name):
        """
        Rotates the specified element to the back of the LRU queue or adds it if not seen
        :param element_name: The request element name
        """
        try:
            self._access_order.append(self._access_order.pop(self._access_order.index(element_name)))
        except ValueError:
            self._access_order.append(element_name)
