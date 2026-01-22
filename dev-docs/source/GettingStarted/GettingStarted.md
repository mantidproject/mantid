# Getting Started

::: {.contents local=""}
:::

::: {.toctree hidden=""}
GettingStartedCondaWindows GettingStartedCondaLinux
GettingStartedCondaOSX GettingStartedNoneConda
:::

## Environment Setup

With the addition of conda to our workflow we at present have two
different methods of Getting Started with mantid development. Therefore
you can choose which method to setup with (We recommend conda as it will
be the only supported method eventually).

- `GettingStartedCondaWindows`
- `GettingStartedCondaLinux`
- `GettingStartedCondaOSX`
- `GettingStartedNoneConda`

### Custom git setup for inside the ORNL firewall:

Due to security configuration at ORNL one needs to do additional
configuration to access github from within the lab. One option is to use
the `https` protocol listed above The alternative is to access github
through ssh.github.com. Add the following lines to `~/.ssh/config`:

``` bash
Host github.com
    Hostname ssh.github.com
    Port 443
    PreferredAuthentications publickey
```

If you need further help, ask another developer at the facility how to
configure ssh.

## Building Mantid

From this point, you can proceed to build Mantid. See
`BuildingWithCMake` for information.  
For developers at ISIS, continue reading for information on accessing
the archive.

## Archive access - ISIS

It is very convenient to be able to access the data archive directly. At
ISIS, this is automatically done on the Windows machines, however OSX
and Linux require some extra setup.

### OSX

- In Finder "command"+k opens a mounting dialogue
- For <span class="title-ref">Server address</span> enter
  <span class="title-ref">smb://isisdatar80.isis.cclrc.ac.uk/inst\$/</span>
  hit Connect
- This should prompt you for federal ID
  <span class="title-ref">clrc....</span> and password
- After completing this the drive is now mounted
- It can be found at <span class="title-ref">/Volumes/inst\$</span>

**NB** the address in step 2 sometimes changes - if it does not work,
replace <span class="title-ref">80</span> with
<span class="title-ref">55</span> or <span class="title-ref">3</span>.

### Linux

1.  Install packages:

`sudo apt-get install -y autofs cifs-utils keyutils`

2.  Create an `/archive.creds` file in the root directory containing
    this, filling in the relevant details:

This should only be done if full disk encryption is enabled or if the
`archive.creds` file is stored in a secure (encrypted) location; to
ensure passwords are kept safe.

``` text
username=FEDERAL_ID_HERE
password=FED_PASSWD_HERE
domain=CLRC
```

3.  Edit `/etc/auto.master` and add the line:

``` text
/archive      /etc/auto.archive
```

4.  Create `/etc/auto.archive` and add the single line:

``` text
*     -fstype=cifs,ro,credentials=/archive.creds,file_mode=0444,dir_mode=0555,vers=3.0,noserverino,nounix    ://isis.cclrc.ac.uk/inst\$/&
```

5.  Enter the following commands:

``` bash
sudo chmod 400 /archive.creds
sudo mkdir /archive
service autofs restart
```

Done. You can now access directories in the archive. Test it by doing:

``` bash
ls /archive/ndxalf
```

If it's working the command should return
`ls: cannot access '/archive/ndxalf/DfsrPrivate': Permission denied`
