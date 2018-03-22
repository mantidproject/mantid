import os
import re
import subprocess


LAST_MODIFIED_UNKNOWN = 'unknown'


def cache_subtree(cache, root, path):
    proc = subprocess.Popen(
        ['git', 'log', '--pretty=format:%cd', '--date=short', '--name-only', path],
        cwd=root,
        stdout=subprocess.PIPE)

    current_date_str = None

    date_regex = re.compile('\d\d\d\d\-\d\d\-\d\d')
    filename_regex = re.compile('[\/\w,\s\-\_]+\.[A-Za-z]+')

    for line in proc.stdout:
        line = str(line.decode('utf-8')).strip()

        if date_regex.match(line):
            # This line contains the date that the subsequent files were last modified
            current_date_str = line

        # Only take the first (most recent) appearence of each file
        elif filename_regex.match(line) and line not in cache:
            # This line contains a file that was modified on the last mentioned date
            cache[line] = current_date_str


def get_file_last_modified_time(cache, root, filename):
    # If cache is empty then start by caching all of Framework
    # Will usually catch all the files you need
    if len(cache) == 0:
        cache_subtree(cache, root, 'Framework')

    source_filename = filename.replace(root, '')
    if source_filename.startswith(os.path.sep):
        source_filename = source_filename[1:]

    # Check if details for this file have already been cached
    if source_filename not in cache:
        # Cache the subtree for this file
        subtree_path = os.path.dirname(filename)
        cache_subtree(cache, root, subtree_path)

        # Make sure it is cached now
        if source_filename not in cache:
            return LAST_MODIFIED_UNKNOWN

    return cache[source_filename]
