from itertools import chain, ifilterfalse
import string, os

# Authors in Git that do not have a translation listed here or that have not
# been blacklisted will cause the DOI script to fail.
#
# The DOI schema asks that names be of the form "last_name, first_name(s)."
#
# Translations exist for Git aliases, even where users have more than one.  We
# prefer multiple translations over blacklist entries in case users log back on
# to machines and start using old aliases again.
_translations = {
#    Name in Git.             :  Preffered name for DOI.
    'Freddie Akeroyd'         : 'Akeroyd, Freddie',
    'Stuart Ansell'           : 'Ansell, Stuart',
    'Sofia Antony'            : 'Antony, Sofia',
    'owen'                    : 'Arnold, Owen',
    'Owen Arnold'             : 'Arnold, Owen',
    'Arturs Bekasovs'         : 'Bekasovs, Arturs',
    'Jean Bilheux'            : 'Bilheux, Jean',
    'JeanBilheux'             : 'Bilheux, Jean',
    'Jose Borreguero'         : 'Borreguero, Jose',
    'Keith Brown'             : 'Brown, Keith',
    'Alex Buts'               : 'Buts, Alex',
    'Stuart Campbell'         : 'Campbell, Stuart',
    'Dickon Champion'         : 'Champion, Dickon',
    'Laurent Chapon'          : 'Chapon, Laurent',
    'Matt Clarke'             : 'Clarke, Matt',
    'Robert Dalgliesh'        : 'Dalgliesh, Robert',
    'mathieu'                 : 'Doucet, Mathieu',
    'mdoucet'                 : 'Doucet, Mathieu',
    'Mathieu Doucet'          : 'Doucet, Mathieu',
    'Doucet, Mathieu'         : 'Doucet, Mathieu',
    'Nick Draper'             : 'Draper, Nick',
    'Ronald Fowler'           : 'Fowler, Ronald',
    'Martyn Gigg'             : 'Gigg, Martyn',
    'Samuel Jackson'          : 'Jackson, Samuel',
    'Dereck Kachere'          : 'Kachere, Dereck',
    'Ricardo Leal'            : 'Leal, Ricardo',
    'VickieLynch'             : 'Lynch, Vickie',
    'Vickie Lynch'            : 'Lynch, Vickie',
    'Pascal Manuel'           : 'Manuel, Pascal',
    'Anders Markvardsen'      : 'Markvardsen, Anders',
    'Anders-Markvardsen'      : 'Markvardsen, Anders',
    'Dennis Mikkelson'        : 'Mikkelson, Dennis',
    'Ruth Mikkelson'          : 'Mikkelson, Ruth',
    'Miller R G'              : 'Miller, Ross',
    'Ross Miller'             : 'Miller, Ross',
    'Sri Nagella'             : 'Nagella, Sri',
    'T Nielsen'               : 'Nielsen, Torben',
    'Karl Palmen'             : 'Palmen, Karl',
    'Peter Parker'            : 'Parker, Peter George',
    'Gesner Passos'           : 'Passos, Gesner',
    'Pete Peterson'           : 'Peterson, Peter',
    'Peter Peterson'          : 'Peterson, Peter',
    'Jay Rainey'              : 'Rainey, Jay',
    'Shelly Ren'              : 'Ren, Shelly',
    'Michael Reuter'          : 'Reuter, Michael',
    'Lakshmi Sastry'          : 'Sastry, Lakshmi',
    'AndreiSavici'            : 'Savici, Andrei',
    'Andrei Savici'           : 'Savici, Andrei',
    'Russell Taylor'          : 'Taylor, Russell',
    'Roman Tolchenov'         : 'Tolchenov, Roman',
    'Robert Whitley'          : 'Whitley, Robert',
    'Michael Whitty'          : 'Whitty, Michael',
    'Steve Williams'          : 'Williams, Steve',
    'Marie Yao'               : 'Yao, Marie',
    'Wenduo Zhou'             : 'Zhou, Wenduo',
    'Janik Zikovsky'          : 'Zikovsky, Janik'
}

# Used to ensure a Git author does not appear on the list.  NOT to be used
# where a translation entry would suffice.
_blacklist = [
    '',
    'unknown'
]

# Used for sponsors / contributors who should be included, but who are not
# listed as authors on Git.
_whitelist = [

]

import subprocess

def run_from_script_dir(func):
    '''Decorator that changes the working directory to the directory of this
    script for the duration of the decorated function.  Basically it means we
    can be sure that calls to "git tag" and "git log" still work, even if this
    script is called from outside the Git tree.
    '''
    def change_dir_wrapper(*args, **kwargs):
        cwd = os.getcwd()
        os.chdir(os.path.dirname(os.path.abspath(__file__)))
        result = func(*args, **kwargs)
        os.chdir(cwd)
        return result

    return change_dir_wrapper

@run_from_script_dir
def _get_all_git_tags():
    '''Returns a list of all the tags in the tree.
    '''
    return subprocess.check_output(['git', 'tag']).replace('"', '').split('\n')

def _clean_up_author_list(author_list):
    '''Apply translations, blacklist and whitelist, and get rid of duplicates.
    '''
    # Double check that all names have no leading or trailing whitespace.
    result = map(string.strip, author_list)

    # Remove any blacklisted names.
    result = set(ifilterfalse(_blacklist.__contains__, result))

    # Make sure there are no names in Git without a corresponding translation.
    untranslated = set(ifilterfalse(_translations.keys().__contains__, result))
    if untranslated:
        raise Exception(
            'No translation exists for the following Git author(s): \n' + \
            '\n'.join(untranslated) + '\n' + \
            'Please edit the translations table accordingly.')

    # Translate all remaining names.
    result = [_translations[a] for a in result]

    # Return the translated names, plus any whitelisted names.
    return sorted(set(result + _whitelist))

@run_from_script_dir
def _authors_from_tag_info(tag_info):
    '''Given some tag/commit information, will return the corresponding Git
    authors.
    '''
    args = [
        'git', 'log',
        '--pretty=short',
        tag_info,
        '--format="%aN"',
        '--reverse'
    ]

    return subprocess.check_output(args).replace('"', '').split('\n')

def _find_tag(major, minor, patch):
    '''Return the Git tag, if it actually exists.  Where the patch number is
    zero, check for "v[major].[minor].[patch]" as well as "v[major].[minor]".
    '''
    suggested_tags = []
    if patch == 0:
        suggested_tags.append('v%d.%d' % (major, minor))
    suggested_tags.append('v%d.%d.%d' % (major, minor, patch))

    for tag in suggested_tags:
        if tag in _get_all_git_tags():
            return tag

    raise Exception(
        "Could not find the following tag(s): " + str(suggested_tags))

def authors_up_to_git_version(major, minor, patch):
    '''Get a list of all authors who have made a commit, up to and including
    the tag of the given version.
    '''
    authors = _authors_from_tag_info(_find_tag(major, minor, patch))    
    return _clean_up_author_list(authors)

def authors_under_git_tag(major, minor, patch):
    '''Get a list of all authors who have made a commit, up to and including
    the tag of the given version, but from the tag of the previous version.
    I.e. if given "2, 6, 1" as inputs, then only commits between the tags
    "v2.6.0" and "v2.6.1" will be included.
    '''
    tag = _find_tag(major, minor, patch)
    all_tags = _get_all_git_tags()

    previous_tag = all_tags[all_tags.index(tag) - 1]
    
    authors = _authors_from_tag_info(previous_tag + '..' + tag)
    return _clean_up_author_list(authors)