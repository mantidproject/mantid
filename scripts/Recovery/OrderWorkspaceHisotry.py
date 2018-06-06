from __future__ import print_function
import csv
from os import listdir
from os.path import isfile, join
import datetime

def _concatenate_iso_datetime(datetime):
    '''
    Concatenates the datetime string provided by Mantid into a single integer
    Args:
        datetime: ISO 8601 standard datetime string
    Returns:
        int
    '''
    date = datetime.split('T')[0]
    time = datetime.split('T')[1]
    yy = str(date.split('-')[0])
    mo = str(date.split('-')[1])
    dd = str(date.split('-')[2])
    hh = str(time.split(':')[0])
    mm = str(time.split(':')[1])
    secs = time.split(':')[2]
    ss = str(secs.split('.')[0])
    ms = str(secs.split('.')[1])

    return int(yy+mo+dd+hh+mm+ss+ms)


def _has_duplicates(values):
    '''
    Tests the list of commands to see if any are duplicated
    Args:
        values: a list of tuples
    Returns:
        True/False: a bool
    '''
    # For each element, check all following elements for a duplicate.
    for i in range(len(values)):
        for x in range(i + 1, len(values)):
    # Both command and date/time must be the same to deem a duplicate
            if values[i][0] == values[x][0] and values[i][1] == values[x][1]:
                return True
    return False

def _remove_duplicates(values):
    '''
    Removes duplciated commands
    Args:
        values: a list of tuples
    Returns:
        output: a list of tuples
    '''
    output = [values[0]]    # The first command is never seen before
    for i in range(1, len(values)):
        duplicated = False
        for j in range(len(output)):
            if values[i][1] == output[j][1] and values[i][0] == output[j][0]:
                duplicated = True

        if not duplicated:    
            output.append(values[i])
            
    return output

# Get list of all workspace histories
onlyfiles = [f for f in listdir('.') if isfile(join('.', f))]
historyfiles = [x for x in onlyfiles if x.endswith('ws.py')]

# Read each history in as a list of tuples
commands = []
for infile in historyfiles:
    with open(infile) as f:
        reader = csv.reader(f, delimiter='#')
        commands.append([tuple(r) for r in reader])

# Add lists of histories together 
all_commands = [i for sublist in commands for i in sublist]

# Remove duplicate commands
if _has_duplicates(all_commands):
    all_unique_commands = _remove_duplicates(all_commands)

# Convert the datetime into a sortable integer
all_unique_commands = [(i[0], _concatenate_iso_datetime(i[1]))
        for i in all_unique_commands]

# Sort the new list on datetime integer
all_unique_commands.sort(key=lambda x: (x[1])) 

# Write to file
with open ('OrderedHistory.py', 'w') as outfile:
    for x in all_unique_commands:
        outfile.write('{} # {} \n'.format(x[0], x[1]))

