# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from itertools import ifilterfalse
import os
import re
import subprocess

# Authors in Git that do not have a translation listed here or that have not
# been blacklisted will cause the DOI script to fail.
#
# The DOI schema asks that names be of the form "last_name, first_name(s)."
#
# Translations exist for Git aliases, even where users have more than one.  We
# prefer multiple translations over blacklist entries in case users log back on
# to machines and start using old aliases again.
_translations = {
    #Name in Git.             :  Preffered name for DOI.
    'Freddie Akeroyd'         : 'Akeroyd, Freddie',
    'Stuart Ansell'           : 'Ansell, Stuart',
    'Sofia Antony'            : 'Antony, Sofia',
    'owen'                    : 'Arnold, Owen',
    'Owen Arnold'             : 'Arnold, Owen',
    'Arturs Bekasovs'         : 'Bekasovs, Arturs',
    'Jean Bilheux'            : 'Bilheux, Jean',
    'JeanBilheux'             : 'Bilheux, Jean',
    'Bilheux'                 : 'Bilheux, Jean',
    'Jose Borreguero'         : 'Borreguero, Jose',
    'Keith Brown'             : 'Brown, Keith',
    'Alex Buts'               : 'Buts, Alex',
    'abuts'                   : 'Buts, Alex',
    'Stuart Campbell'         : 'Campbell, Stuart',
    'Stuart I. Campbell'      : 'Campbell, Stuart',
    'Dickon Champion'         : 'Champion, Dickon',
    'Laurent Chapon'          : 'Chapon, Laurent',
    'Matt Clarke'             : 'Clarke, Matt',
    'Robert Dalgliesh'        : 'Dalgliesh, Robert',
    'mathieu'                 : 'Doucet, Mathieu',
    'mdoucet'                 : 'Doucet, Mathieu',
    'Mathieu Doucet'          : 'Doucet, Mathieu',
    'Doucet, Mathieu'         : 'Doucet, Mathieu',
    'Nick Draper'             : 'Draper, Nick',
    'NickDraper'              : 'Draper, Nick',
    'Ronald Fowler'           : 'Fowler, Ronald',
    'Martyn Gigg'             : 'Gigg, Martyn A.',
    'Samuel Jackson'          : 'Jackson, Samuel',
    'Dereck Kachere'          : 'Kachere, Dereck',
    'Mark Koennecke'          : 'Koennecke, Mark',
    'Ricardo Leal'            : 'Leal, Ricardo',
    'Ricardo Ferraz Leal'     : 'Leal, Ricardo',
    'Ricardo M. Ferraz Leal'  : 'Leal, Ricardo',
    'Christophe Le Bourlot'   : 'Le Bourlot, Christophe',
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
    'Peter Parker'            : 'Parker, Peter G.',
    'Gesner Passos'           : 'Passos, Gesner',
    'Pete Peterson'           : 'Peterson, Peter F.',
    'Peter Peterson'          : 'Peterson, Peter F.',
    'Peter F. Peterson'       : 'Peterson, Peter F.',
    'Jay Rainey'              : 'Rainey, Jay',
    'Yannick Raoul'           : 'Raoul, Yannick',
    'Shelly Ren'              : 'Ren, Shelly',
    'Michael Reuter'          : 'Reuter, Michael',
    'Lakshmi Sastry'          : 'Sastry, Lakshmi',
    'AndreiSavici'            : 'Savici, Andrei',
    'Andrei Savici'           : 'Savici, Andrei',
    'Russell Taylor'          : 'Taylor, Russell J.',
    'Mike Thomas'             : 'Thomas, Mike',
    'Roman Tolchenov'         : 'Tolchenov, Roman',
    'MichaelWedel'            : 'Wedel, Michael',
    'Michael Wedel'           : 'Wedel, Michael',
    'Ross Whitfield'          : 'Whitfield, Ross',
    'Robert Whitley'          : 'Whitley, Robert',
    'Michael Whitty'          : 'Whitty, Michael',
    'Steve Williams'          : 'Williams, Steve',
    'Marie Yao'               : 'Yao, Marie',
    'Wenduo Zhou'             : 'Zhou, Wenduo',
    'Janik Zikovsky'          : 'Zikovsky, Janik',
    'Harry Jeffery'           : 'Jeffery, Harry',
    'Federico M Pouzols'      : 'Pouzols, Federico M',
    'FedeMPouzols'            : 'Pouzols, Federico M',
    'Federico Montesino Pouzols': 'Pouzols, Federico M',
    'Fede'                    : 'Pouzols, Federico M',
    'Anton Piccardo-Selg'     : 'Piccardo-Selg, Anton',
    'Lottie Greenwood'        : 'Greenwood, Lottie',
    'Dan Nixon'               : 'Nixon, Dan',
    'Raquel Alvarez Banos'    : 'Banos, Raquel Alvarez',
    'John Hill'               : 'Hill, John',
    'Ian Bush'                : 'Bush, Ian',
    'Steven Hahn'             : 'Hahn, Steven',
    'Steven E. Hahn'          : 'Hahn, Steven',
    'Joachim Wuttke (o)'      : 'Wuttke, Joachim',
    'DiegoMonserrat'          : 'Monserrat, Diego',
    'Diego Monserrat'         : 'Monserrat, Diego',
    'David Mannicke'          : 'Mannicke, David',
    'Garrett Granroth'        : 'Granroth, Garrett',
    'Hahn'                    : 'Hahn, Steven',
    'Marina Ganeva'           : 'Ganeva, Marina',
    'Raquel Alvarez'          : 'Alvarez, Raquel',
    'Raquel'                  : 'Alvarez, Raquel',
    'jmborr'                  : 'Borreguero, Jose',
    'Tobias Richter'          : 'Richter, Tobias',
    'ianbush'                 : 'Bush, Ian',
    'KarlPalmen'              : 'Palmen, Karl',
    'Matthew D Jones'         : 'Jones, Matthew D.',
    'Matt King'               : 'King, Matt',
    'Jiao Lin'                : 'Lin, Jiao',
    'Jiao'                    : 'Lin, Jiao',
    'Simon Heybrock'          : 'Heybrock, Simon',
    'Elliot Oram'             : 'Oram, Elliot',
    'Dominic Oram'            : 'Oram, Dominic',
    'Shahroz Ahmed'           : 'Ahmed, Shahroz',
    'celinedurniak'           : 'Durniak, Celine',
    'Celine Durniak'          : 'Durniak, Celine',
    'Michael Hart'            : 'Hart, Michael',
    'Lamar Moore'             : 'Moore, Lamar',
    'LamarMoore'              : 'Moore, Lamar',
    'Moore'                   : 'Moore, Lamar',
    'Tom Perkins'             : 'Perkins, Tom',
    'Jan Burle'               : 'Burle, Jan',
    'Duc Le'                  : 'Le, Duc',
    'David Fairbrother'       : 'Fairbrother, David',
    'DavidFair'               : 'Fairbrother, David',
    'Eltayeb Ahmed'           : 'Ahmed, Eltayeb',
    'Dimitar Tasev'           : 'Tasev, Dimitar',
    'Dimitar Borislavov Tasev': 'Tasev, Dimitar',
    'Antti Soininen'          : 'Soininen, Antti',
    'Antti Soininnen'         : 'Soininen, Antti',
    'Pranav Bahuguna'         : 'Bahuguna, Pranav',
    'Louise McCann'           : 'McCann, Louise',
    'louisemccann'            : 'McCann, Louise',
    'Gagik Vardanyan'         : 'Vardanyan, Gagik',
    'Verena Reimund'          : 'Reimund, Verena',
    'reimundILL'              : 'Reimund, Verena',
    'Krzysztof Dymkowski'     : 'Dymkowski, Krzysztof',
    'dymkowsk'                : 'Dymkowski, Krzysztof',
    'krzych'                  : 'Dymkowski, Krzysztof',
    'Gemma Guest'             : 'Guest, Gemma',
    'Anthony Lim'             : 'Lim, Anthony',
    'Anthony Lim'             : 'Lim, Anthony',
    'AnthonyLim23'            : 'Lim, Anthony',
    'Anthony'                 : 'Lim, Anthony',
    'CipPruteanu'             : 'Ciprian Pruteanu',
    'Tasev'                   : 'Tasev, Dimitar',
    'Mayer Alexandra'         : 'Mayer, Alexandra',
    'simonfernandes'          : 'Fernandes, Simon',
    'Simon Fernandes'         : 'Fernandes, Simon',
    'brandonhewer'            : 'Hewer, Brandon',
    'Brandon Hewer'            : 'Hewer, Brandon',
    'Thomas Lohnert'          : 'Lohnert, Thomas',
    'James Tricker'           : 'Tricker, James',
    'Matthew Bowles'          : 'Bowles, Matthew',
    'MatthewBowles'           : 'Bowles, Matthew',
    'josephframsay'           : 'Ramsay, Joseph F.',
    'Joseph Ramsay'           : 'Ramsay, Joseph F.',
    '='                       : 'Ramsay, Joseph F.',
    'Joe Ramsay'              : 'Ramsay, Joseph F.',
    'Adam Washington'         : 'Wahington, Adam',
    'Edward Brown'            : 'Brown, Edward',
    'Matthew Andrew'          : 'Andrew, Matthew',
    'Mantid-Matthew'          : 'Andrew, Matthew',
    'Keith Butler'            : 'Butler, Keith T.',
    'fodblog'                 : 'Butler, Keith T.',
    'Marshall McDonnell'      : 'McDonnell, Marshall',
    'McDonnell, Marshall T'      : 'McDonnell, Marshall',
    'Neil Vaytet'             : 'Vaytet, Neil',
    'Sam'                     : 'Sam Jones',
    'Tom Jubb'                : 'Jubb, Tom',
    'T Jubb'                  : 'Jubb, Tom',
    'TWJubb'                  : 'Jubb, Tom',
    'Brendan Sullivan'        : 'Sullivan, Brendan',
    'Joachim Coenen'          : 'Coenen, Joachim'
}

# Used to ensure a Git author does not appear in any of the DOIs.  This is NOT
# to be used in the case where a Git user has multiple accounts; a translation
# entry would suffice in such an instance.
_blacklist = [
    '',
    'unknown',
    'Yao, Marie',
    'Utkarsh Ayachit',
    'Chris Kerr',
    'Thomas Brooks',
    'mantid-builder',
    'Erik B Knudsen',
    'Bartomeu Llopis',
    'dpaj',
    'Daniel Pajerowski',
    'thomueller'
]

# The whitelist is used for sponsors / contributors who should be included,
# but who are not listed as authors on Git.  These names will be shown in the
# "main" DOI only.
whitelist = [
    'Cottrell, Stephen',
    'Dillow, David',
    'Hagen, Mark',
    'Hillier, Adrian',
    'Heller, William',
    'Howells, Spencer',
    'McGreevy, Robert',
    'Pascal, Manuel',
    'Perring, Toby',
    'Pratt, Francis',
    'Proffen, Thomas',
    'Radaelli, Paolo',
    'Taylor, Jon',
    'Granroth, Garrett'
]


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
    return subprocess.check_output(['git', 'tag', '--sort=version:refname']).replace('"', '').split('\n')


def _clean_up_author_list(author_list):
    '''Apply translations and blacklist, and get rid of duplicates.
    '''
    # Double check that all names have no leading or trailing whitespace.
    result = map(str.strip, author_list)

    # Remove any blacklisted names.
    result = set(ifilterfalse(_blacklist.__contains__, result))

    # Make sure there are no names in Git without a corresponding translation.
    untranslated = set(ifilterfalse(_translations.keys().__contains__, result))
    if untranslated:
        raise Exception(
            'No translation exists for the following Git author(s): \n' +
            '\n'.join(untranslated) + '\n' +
            'Please edit the translations table accordingly.')

    # Translate all remaining names.
    result = [_translations[a] for a in result]

    # Another check for any blacklisted names, in case we want to remove the
    # translated name.
    result = set(ifilterfalse(_blacklist.__contains__, result))

    # Return the unique list of translated names.
    return sorted(set(result))


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

    authors = subprocess.check_output(args).replace('"', '').split('\n')
    return _clean_up_author_list(authors)


def find_tag(version_str):
    '''Return the Git tag, if it actually exists, for a given version".
    '''
    tag_title = 'v' + version_str
    if tag_title in _get_all_git_tags():
        return tag_title
    else:
        raise RuntimeError("Cannot find expected git tag '{0}'".format(tag_title))


def get_previous_tag(tag):
    '''Given an existing git tag, will return the tag that is found before it.
    '''
    all_tags = _get_all_git_tags()
    if tag not in all_tags:
        return None
    return all_tags[all_tags.index(tag) - 1]


def get_major_minor_patch(version_str):
    '''Return the major, minor & patch revision numbers as integers
    '''
    version_components = version_str.split('.')
    if len(version_components) != 3:
        raise RuntimeError("Invalid format for version string. Expected X.Y.Z")
    return map(int, version_components)


def get_shortened_version_string(version_str):
    '''We use the convention whereby the patch number is ignored if it is zero,
    i.e. "3.0.0" becomes "3.0".
    '''
    major, minor, patch = get_major_minor_patch(version_str)
    if patch == 0:
        return '{0}.{1}'.format(major, minor)
    else:
        return '{0}.{1}.{2}'.format(major, minor, patch)


def get_version_from_git_tag(tag):
    '''Given a tag from Git, extract the major, minor and patch version
    numbers.
    '''
    short_regexp = r'^v(\d+).(\d+)$'
    long_regexp  = r'^v(\d+).(\d+).(\d+)$'

    if re.match(short_regexp, tag):
        a, b = [int(x) for x in re.findall(r'\d+', tag)]
        c = 0
    elif re.match(long_regexp, tag):
        a, b, c = [int(x) for x in re.findall(r'\d+', tag)]
    else:
        raise RuntimeError(
            "Unable to parse version information from \"" + tag + "\"")
    return '{0}.{1}.{2}'.format(a, b, c)


def authors_up_to_git_tag(tag):
    '''Get a list of all authors who have made a commit, up to and including
    the given tag.
    '''
    return _authors_from_tag_info(tag)


def authors_under_git_tag(tag):
    '''Get a list of all authors who have made a commit, up to and including
    the given tag, but after the tag of the previous version.  I.e. if given
    "2, 6, 1" as inputs, then only commits between the tags "v2.6.0" and
    "v2.6.1" will be included.
    '''
    all_tags = _get_all_git_tags()

    previous_tag = all_tags[all_tags.index(tag) - 1]

    return _authors_from_tag_info(previous_tag + '..' + tag)
