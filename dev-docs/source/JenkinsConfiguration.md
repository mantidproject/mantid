# Jenkins Configuration

```{contents}
:local:
```

## Summary

Mantid uses the [Jenkins](https://jenkins.io/) automation server to
support its continuous integration needs. This document describes the
general setup of the system.

## Introduction

Jenkins works on a 'controller -\> agent' principle. The controller is
responsible for orchestrating jobs and managing the agents where the
work is actually performed. The controller node is located at
<https://builds.mantidproject.org> and each facility is responsible for
providing hardware to act as agents for the various required
configurations.

## Setting up a New Agent

### Windows

First install the prerequisites tools that cannot be provided through
conda:

1.  Install the [Command line Visual C++ 2019 build
    tools](https://visualstudio.microsoft.com/downloads/). 2019 matches
    the version used by conda-forge. You may need to scroll to the
    botton and "click Older Downloads" to find it. Once the installer
    has downloaded run it as administrator and select the C++ workload.
2.  Install [Git for Windows](https://git-scm.com/download/win) using
    the 64-bit Standalone Installer. Once downloaded run it as
    administrator and ensure the installation location is
    `C:\ProgramFiles\Git`. When the installer has finished, open a
    PowerShell prompt as administrator and run
    `git config --system --get core.longpaths true` to configure Git for
    long paths.
3.  Configure long paths in the registry by running `regedit` from a
    Powershell administrator prompt and setting the
    `Computer\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled`
    key to `1`.
4.  Install Java from
    [Adoptium](https://adoptium.net/en-GB/temurin/releases/?version=11)
    Select `OS=Windows`, `Architecture=x64`,
    <span class="title-ref">Package
    Type=JRE</span><span class="title-ref">,
    </span><span class="title-ref">Version=11</span><span class="title-ref">.
    Install as administrator and add to that
    </span><span class="title-ref">PATH</span>` for all users.
5.  Reboot the machine

#### Agent Connection

To connect the agent to Jenkins you will need to login to the controller
at <https://builds.mantidproject.org>. Once logged in:

1.  Click "New Node" at <https://builds.mantidproject.org/computer/>.
2.  It is recommended to select "Copy Existing Node" and choose another
    Windows-based agent.
3.  Set the name to following the pattern `FACILITY-HOSTNAME`.
4.  Edit the description to describe where the machine is located.
5.  Edit the `win-64` label to `xwin-64` to avoid the node being added
    to the pool straight away.
6.  Under the "Environment variables" section at the bottom change the
    values to match the node properties, e.g. build threads to match
    number of cores.
7.  Click "Save" to be returned to the front page of the new node.
8.  On the desktop of the node visit
    <https://builds.mantidproject.org/computer/>\<node-name\>/jenkins-agent.jnlp,
    where `<node-name>` is the name of the node when created above. Take
    note of the download location for the next step.
9.  Open PowerShell as an administrator and change directory to that
    noted in the previous step. Run the agent file:
    <span class="title-ref">.jenkins-agent.jnlp</span>.
10. In the GUI that pops up a `File` menu will appear after a few
    seconds
11. Click `File->Install Service`.
12. Check back on the Jenkins agent description page and it should be
    connected.

#### Connecting Through a Proxy Server

It is a little more tricky to add Windows agents connected through a
proxy. To do this you must modify the Java arguments that are used to
start the `jenkins-slave` process. Once the "Install as a Windows
Service" has completed you should:

1.  Find the directory on the machine, such as `C:\Jenkins`, that was
    configured as the Jenkins filesystem root
2.  Open the `jenkins-slave.xml` file
3.  Edit the java arguments tag and add
    `-Dhttp.proxyHost=PROXYHOST -Dhttp.proxyPort=PROXYPORT` to the list
4.  Save the file and restart the service (or machine)

### Linux

Linux-based nodes should use the Docker-based setup described
[here](https://github.com/mantidproject/dockerfiles/tree/main/jenkins-node).
The base OS does not need to be RancherOS and can be anything as long as
Docker will run.

The agent will connect automatically when the Docker container starts
running.

### Mac OS

Enable [SSH ("Remote Login") and VNC ("Remote
Management")](https://apple.stackexchange.com/a/73919). If you have
connection issues from a non-OS X client then try adjusting your color
depth settings (True Color 32bpp works on Remmina).

In order to run the Qt tests, which require a connection to the
windowing system, the user that is running the Jenkins agent must be
left logged in and the automatic screen lock must be disabled. This is
most easily done by VNC - connect, log in, then disconnect. If you see
errors such as:

    _RegisterApplication(), FAILED TO establish the default connection to the WindowServer,
    _CGSDefaultConnection() is NULL.

then no one is logged in to the system.

Disable saved application states that cause a dialog to be raised after
a program crash resulting in a test hanging waiting for a user to click
ok on a dialog:

    defaults write org.python.python NSQuitAlwaysKeepsWindows -bool false
    defaults write org.mantidproject.MantidPlot NSQuitAlwaysKeepsWindows -bool false

Finally, install Java 11 JRE from
<https://adoptium.net/temurin/releases/> by selecting `macOS`, `x64`,
`JRE` and `11` for the respective options. Download the `.pkg` and
install following the instructions.

Restart the machine and ensure you leave the jenkins-agent user logged
in as per the instructions above.

#### Agent Connection

The Jenkins JNLP connections are maintained by a crontab entry. The
script is in the [mantid
repository](https://github.com/mantidproject/mantid/blob/main/buildconfig/Jenkins/jenkins-slave.sh).
The comments at the top describe a how to customize the script for a new
agent.

## Misc Groovy Scripts

The following is a collection of groovy scripts that can be run either
at <https://builds.mantidproject.org/script> (for master node) or on a
given node, e.g
[isis-mantidx3](https://builds.mantidproject.org/computer/isis-mantidlx3/script).
You must have admin privileges to run them.

<https://github.com/jenkinsci/jenkins-scripts/tree/main/scriptler> was
helpful for coming up with some of these.

### Print the Value of an Environment Variable on All Nodes

``` groovy
import jenkins.model.*
import hudson.model.*
import hudson.slaves.*

VARIABLE_NAME = "ENV_VARIABLE_NAME"

nodes = Jenkins.instance.getNodes()
println("Displaying values of " + VARIABLE_NAME + " on all nodes")
println()
for(node in nodes) {
  node_props = node.nodeProperties.getAll(hudson.slaves.EnvironmentVariablesNodeProperty.class)
  if(node_props.size() == 1) {
    env_vars = node_props[0].getEnvVars()
  if(env_vars.containsKey(VARIABLE_NAME)) {
    pv_dir = env_vars.get(VARIABLE_NAME, "")
  } else {
    pv_dir = VARIABLE_NAME + " not set."
  }
  println(node.getDisplayName() + ": " + pv_dir)
  } else {
    pv_dir = VARIABLE_NAME + " not set."
  }
}
```

### Remove directories across multiple nodes

The example here will remove the build directories from pull request
build and test jobs. Useful, for example, when a dependency change
requires a clean build. It is advised to ensure nothing is running and
pause the build queue.

``` groovy
import hudson.model.*

nodes = Jenkins.instance.slaves

JOB_PREFIX = "pull_requests-"
suffixes = ["conda-linux", "conda-osx", "conda-windows"];

for (node in nodes) {
  for (suffix in suffixes) {
    FilePath fp = node.createPath(node.getRootPath().toString() + File.separator + "workspace" + File.separator + JOB_PREFIX + suffix + File.separator +  "build");
    if(fp!=null && fp.exists()) {
      println(fp.toString())
      fp.deleteRecursive()
    }
  }
}
```

### Remove directories from single node

It is advised to take the target node offline.

``` groovy
import hudson.model.*

// Example: "isis-ndw1597"
String agentName = <agent/node name>

// Example: "pull_requests-conda-windows" , "build_branch"
jobs = [<job 1 string> , <job 2 string>, ...]

nodes = Jenkins.instance.slaves
for (node in nodes) {
  if(node.toString() == "hudson.slaves.DumbSlave[$agentName]") {
    for (job in jobs) {
      FilePath fp = node.createPath(node.getRootPath().toString() + File.separator + "workspace" + File.separator + job)
      if(fp!=null && fp.exists()) {
        println(node.toString())
        println(fp.toString())
        fp.deleteRecursive()
      }
    }
  }
}
```

### Update Branches For Jobs

``` groovy
import hudson.plugins.git.GitSCM
import hudson.plugins.git.BranchSpec
import static com.google.common.collect.Lists.newArrayList;

def NEW_BRANCH = "*/release-next"

// Access to the Hudson Singleton
def jenkins = jenkins.model.Jenkins.instance;

// Retrieve matching jobs
def allItems = jenkins.items
def chosenJobs = allItems.findAll{job -> job.name =- /release_/};

println "Updating branch for chosen jobs to $NEW_BRANCH"
println ""
// Do work
chosenJobs.each { job ->
    def scm = job.scm;
    if (scm instanceof GitSCM && job.name != "release_nightly_deploy" ) {
      //def newScm = scm.clone()
      println "Updating branch for " + job.name
      scm.branches = newArrayList(new BranchSpec(NEW_BRANCH))
      println "Branch for " + job.name + ": " + scm.branches
      println ""
    }
}
```

### List All SCM Urls

``` groovy
import jenkins.model.*;
import hudson.model.*;
import hudson.tasks.*;
import hudson.plugins.git.*;
import org.eclipse.jgit.transport.RemoteConfig;
import org.eclipse.jgit.transport.URIish;

for(project in Hudson.instance.items) {
  try {
    scm = project.scm;
  } catch(Exception) {
    continue
  }
  if (scm instanceof hudson.plugins.git.GitSCM) {
    for (RemoteConfig cfg : scm.getRepositories()) {
      for (URIish uri : cfg.getURIs()) {
        println("SCM " + uri.toString() + " for project " + project);
      }
    }
  }
}
```

### Update Urls on All Jobs

``` groovy
import jenkins.model.*;
import hudson.model.*;
import hudson.tasks.*;
import hudson.plugins.git.*;
import org.eclipse.jgit.transport.RemoteConfig;

def modifyGitUrl(url) {
  if(url.startsWith('git://')) {
    return "https://" + url.substring(6);
  } else {
    return url;
  }
}

for(project in Hudson.instance.items) {
  try{
    oldScm = project.scm;
  } catch(Exception) {
    continue
  }
  if (oldScm instanceof hudson.plugins.git.GitSCM) {
    def newUserRemoteConfigs = oldScm.userRemoteConfigs.collect {
      new UserRemoteConfig(modifyGitUrl(it.url), it.name, it.refspec, it.credentialsId)
    }
    def newScm = new GitSCM(newUserRemoteConfigs, oldScm.branches, oldScm.doGenerateSubmoduleConfigurations,
                            oldScm.submoduleCfg, oldScm.browser, oldScm.gitTool, oldScm.extensions)
    project.scm = newScm;
    project.save();
  }
}
```

### Print All Loggers

``` groovy
import java.util.logging.*;

LogManager.getLogManager().getLoggerNames().each() {
  println "${it}";
}
```

### Run a Process On a Single Node

``` groovy
Process p = "cmd /c dir".execute()
println "${p.text}"

// kill process on windows slave
Process p = "cmd /c Taskkill /F /IM MantidPlot.exe".execute()
println "${p.text}"
```

### Run a Process Across All Nodes

``` groovy
import hudson.util.RemotingDiagnostics;

for (slave in hudson.model.Hudson.instance.slaves) {
   println slave.name;
   // is it connected?
   if(slave.getChannel()) {
    println RemotingDiagnostics.executeGroovy("println "ls".execute().text", slave.getChannel());
  }
}
```

### Update default values for job parameters

``` groovy
import hudson.model.*

def SUFFIX_VARIABLE = "PACKAGE_SUFFIX"
def NEW_SUFFIX = "nightly"

// Access to the Hudson Singleton
def jenkins = jenkins.model.Jenkins.instance;

// Retrieve matching jobs
def chosenJobs = ["release_clean-rhel7"] //, "release_clean-ubuntu-16.04", "release_clean-ubuntu"]

println "Updating default package suffix for chosen jobs to ${NEW_SUFFIX}"
println ""
// Do work
chosenJobs.each { jobName ->
  job = jenkins.getItem(jobName)
  println(job)
  paramsDef = job.getAction(ParametersDefinitionProperty)
  params = paramsDef.getParameterDefinitions()
  params.each { it ->
    if(it.getName() == SUFFIX_VARIABLE) {
      println("Updating default value of '${SUFFIX_VARIABLE}' variable to '${NEW_SUFFIX}'")
      it.setDefaultValue(NEW_SUFFIX)
    }
  }

}
```
