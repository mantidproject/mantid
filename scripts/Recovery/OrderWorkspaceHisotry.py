from __future__ import print_function
import csv
from os import listdir
from os.path import isfile, join
import datetime

# Because Mantid has an awkward date format
# we need to convert string to number for sorting later
months = {
    'Jan': '01',
    'Feb': '02',
    'Mar': '03',
    'Apr': '04',
    'May': '05',
    'Jun': '06',
    'Jul': '07',
    'Aug': '08',
    'Sep': '09',
    'Oct': '10',
    'Nov': '11',
    'Dec': '12'
    }

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

# Convert the date to a sensible format
## Split date and time into separate elements
all_unique_commands = [(i[0], i[1].split(' ')[1], i[1].split(' ')[2]) 
        for i in all_unique_commands]

## Change the date string to a number
all_unique_commands = [(i[0], i[1].split('-')[0], months[i[1].split('-')[1]], 
        i[1].split('-')[2], i[2]) for i in all_unique_commands]
all_unique_commands = [(i[0], "%s-%s-%s" % (i[1], i[2], i[3]), i[4]) for 
        i in all_unique_commands]

# Sort the new list on date-time
# Write to file
with open ('PreOrderedHistory.py', 'w') as outfile:
    for x in all_unique_commands:
        outfile.write('{} # {} {} \n'.format(x[0], x[1], x[2]))

all_unique_commands.sort(key=lambda x: datetime.datetime.strptime(x[1], '%Y-%m-%d'))  # date sort
#all_unique_commands.sort(key=lambda x: x[2]) # time sort

# Write to file
with open ('OrderedHistory.py', 'w') as outfile:
    for x in all_unique_commands:
        outfile.write('{} # {} {} \n'.format(x[0], x[1], x[2]))
