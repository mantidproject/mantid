# Jenkins to GitHub Actions Migration Research

## Executive Summary

This document provides a comprehensive analysis of migrating the Mantid project's CI/CD infrastructure from Jenkins to GitHub Actions. Based on the issue #39497 requirements and analysis of the current Jenkins setup in `buildconfig/Jenkins/`, this research identifies the current Jenkins jobs, their GitHub Actions equivalents, runner requirements, and migration recommendations.

## Current Jenkins Infrastructure

### Jenkins Jobs Overview

The current Jenkins setup consists of several categories of jobs:

#### 1. **Nightly Build and Deploy Pipeline**
- **File**: `buildconfig/Jenkins/Conda/nightly_build_and_deploy.jenkinsfile`
- **Purpose**: Builds Mantid packages for multiple platforms and deploys to Anaconda and GitHub releases
- **Platforms**: linux-64, win-64, osx-arm64
- **Key Features**:
  - Parallel builds across platforms
  - Developer environment build and test
  - Conda package creation
  - Standalone package creation (.tar.xz, .exe, .dmg)
  - Publishing to Anaconda channel with labels (nightly, unstable)
  - Publishing to GitHub releases
  - Automated cleanup of old nightly packages
  - Test execution (unit tests, system tests, doctests)
- **Dependencies**: Requires self-hosted runners with specific labels

#### 2. **Pull Request Builds (Leeroy)**
- **File**: `buildconfig/Jenkins/leeroy.md`
- **Purpose**: Automated PR testing triggered by GitHub webhooks
- **Jobs**:
  - RHEL7 + System Tests
  - RHEL6
  - Ubuntu + Documentation Tests
  - Windows
  - OSX
  - cppcheck
  - Doxygen
  - ClangFormat
- **Current Status**: Uses Leeroy (Go-based PR builder) for orchestration
- **Features**:
  - Auto-merge PR branches for testing
  - Status reporting back to GitHub
  - Manual rebuild capability
  - Cron-based retry for missed builds

#### 3. **Developer Documentation Build**
- **File**: `buildconfig/Jenkins/Conda/build-and-publish-devsite.jenkinsfile`
- **Purpose**: Build and publish developer website to gh-pages
- **Script**: `build-and-publish-devsite.sh`
- **Process**:
  - Clones gh-pages branch
  - Uses Sphinx to build documentation
  - Pushes updates to GitHub

#### 4. **Code Quality Checks**
- **cppcheck**: Static analysis for C++ code
  - File: `buildconfig/Jenkins/Conda/cppcheck.sh`
  - Creates HTML reports
  - Configurable error thresholds
- **ClangFormat**: Code formatting validation
  - File: `buildconfig/Jenkins/clangformat`
  - Creates patches for formatting fixes
- **Doxygen**: API documentation generation
  - File: `buildconfig/Jenkins/Conda/doxygen.sh`
  - Conditional on C++ file changes

#### 5. **Performance Tests**
- **File**: `buildconfig/Jenkins/performancetests`
- **Purpose**: Run performance benchmarks and generate reports
- **Features**:
  - Uses SQLite database for historical data
  - Generates performance plots
  - Compares against previous runs

#### 6. **Usage Data Packaging**
- **File**: `buildconfig/Jenkins/usagedata.sh`
- **Purpose**: Package test data for distribution

### Build Scripts Analysis

#### Core Build Script: `conda-buildscript`
- **Location**: `buildconfig/Jenkins/Conda/conda-buildscript`
- **Features**:
  - CMake preset-based configuration
  - Parallel build support (configurable threads)
  - Conditional execution (unit tests, system tests, docs, doctests)
  - Coverity support
  - Clean build options
  - Platform detection (linux-64, win-64, osx-arm64)
- **Used by**: All platform builds

#### Package Scripts
1. **package-conda**: Creates Conda packages
2. **package-standalone**: Creates platform-specific installers
3. **publish-to-anaconda**: Uploads to Anaconda Cloud
4. **publish-to-github**: Creates GitHub releases
5. **delete-old-packages.sh**: Cleanup script for old nightly builds

#### Utility Scripts
- **check_for_changes**: Detects file changes to skip unnecessary builds
- **generate-version-number**: Creates version tags for releases
- **mamba-utils**: Shared functions for Conda environment setup
- **download-and-install-miniforge**: Bootstrap script for Conda

## Current GitHub Actions

The repository already has some GitHub Actions workflows in `.github/workflows/`:

1. **automerge.yml**: Merges protected branches (ornl-next, release-next) into main
2. **close_issues.yml**: Automated issue management
3. **delete_old_nightly_releases.yml**: Removes old nightly releases from GitHub
4. **label_merge_conflicts.yml**: Labels PRs with merge conflicts
5. **stale.yml**: Marks stale issues
6. **update-pixi-lockfile.yml**: Updates pixi dependencies

Additionally, the project uses:
- **pre-commit.ci**: Automated pre-commit hook enforcement (external service)
- **CodeRabbit**: Code review automation (external service)

## Migration Strategy

### Runner Requirements (As Per Issue #39497)

Based on the issue description, jobs should be distributed as follows:

| Job Type | Runner Type | Rationale |
|----------|-------------|-----------|
| Linux + System Tests | Self-hosted | Resource intensive |
| macOS | Self-hosted | Resource intensive |
| Windows | Self-hosted | Resource intensive |
| Linux Docs Build | Self-hosted | Resource intensive |
| Linux Package Build | (Probably) Self-hosted | Resource intensive |
| cppcheck | Self-hosted | Resource intensive |
| Doxygen | GitHub-hosted | Lightweight |
| Check for merge conflicts | GitHub-hosted | Already implemented |
| Formatting + Static Analysis | Drop/pre-commit.ci | Redundant with pre-commit.ci |

### Self-Hosted Runner Setup

#### Required Labels
- `linux-64`: Linux x86_64 build machines
- `win-64`: Windows x86_64 build machines
- `osx-arm64`: macOS ARM64 build machines

#### Runner Configuration
GitHub Actions self-hosted runners need to be configured with:
1. **OS-specific build tools**:
   - Linux: gcc, g++, cmake, ninja, conda/mamba
   - Windows: Visual Studio 2022, cmake, conda/mamba
   - macOS: Xcode, cmake, ninja, conda/mamba

2. **Environment variables** (migrated from Jenkins):
   - `GITHUB_TOKEN`: For API access
   - `ANACONDA_TOKEN`: For package publishing
   - `BUILD_THREADS`: Number of build threads

3. **Storage**:
   - Sufficient disk space for builds (~50GB+ per platform)
   - Conda package cache directory
   - Test data storage

4. **Network access**:
   - Access to Anaconda Cloud
   - Access to GitHub releases API
   - Access to external dependencies

### GitHub Actions Workflow Structure

#### Proposed Workflow Organization

```
.github/workflows/
├── pr-build-linux.yml          # PR builds for Linux
├── pr-build-windows.yml        # PR builds for Windows
├── pr-build-macos.yml          # PR builds for macOS
├── pr-cppcheck.yml             # C++ static analysis
├── pr-doxygen.yml              # Documentation generation
├── nightly-build.yml           # Nightly builds and deployment
├── build-and-publish-docs.yml  # Developer site updates
├── performance-tests.yml       # Performance benchmarking
└── release.yml                 # Release builds
```

### Key Migration Challenges

#### 1. **Matrix Builds**
Jenkins uses parallel stages for multi-platform builds. GitHub Actions equivalent:
```yaml
strategy:
  matrix:
    platform: [linux-64, win-64, osx-arm64]
    include:
      - platform: linux-64
        os: [self-hosted, linux, x64]
      - platform: win-64
        os: [self-hosted, windows, x64]
      - platform: osx-arm64
        os: [self-hosted, macOS, ARM64]
```

#### 2. **Cross-Job Artifact Sharing**
Jenkins uses `copyArtifacts` plugin. GitHub Actions equivalent:
```yaml
- uses: actions/upload-artifact@v4
  with:
    name: conda-packages-${{ matrix.platform }}
    path: conda-bld/**/*.conda
```

#### 3. **Conditional Execution**
The `check_for_changes` script needs to be adapted to GitHub Actions path filters:
```yaml
on:
  pull_request:
    paths:
      - '**.cpp'
      - '**.h'
      - '**.py'
```

#### 4. **Secret Management**
Jenkins credentials need to be migrated to GitHub Secrets:
- `ANACONDA_TOKEN` (per channel: mantid, mantid-ornl, mantid-test)
- `GITHUB_TOKEN` (built-in for Actions)
- `MANTID_BUILDER_TOKEN` (already exists)

#### 5. **Build Retention**
Jenkins "Keep latest successful build" logic needs GitHub Actions equivalent:
```yaml
- uses: actions/gh-release@v1
  with:
    tag_name: ${{ env.GIT_TAG }}
    prerelease: true
    files: |
      standalone-packages/*
      conda-packages.tar
```

#### 6. **Leeroy Replacement**
GitHub Actions has native PR build triggers, eliminating the need for Leeroy:
- Webhook handling: Built-in
- Status reporting: Automatic
- Retry mechanism: Can be manually triggered or automated with workflow_dispatch

### Pre-commit.ci Integration

The project already uses pre-commit.ci for:
- trailing-whitespace
- check-added-large-files
- check-xml/yaml
- end-of-file-fixer
- gitleaks (security scanning)
- clang-format
- cmake-format
- rstcheck (RST documentation)
- ruff (Python linting/formatting)

This makes the standalone Jenkins jobs for formatting and some static analysis redundant.

### CMake Presets

The project uses CMake presets defined in `CMakePresets.json`:
- `linux-64-ci`: Linux CI builds
- `win-64-ci`: Windows CI builds (Visual Studio 2022)
- `osx-arm64-ci`: macOS ARM CI builds
- `cppcheck-ci`: CppCheck analysis
- `doxygen-ci`: Doxygen generation

These presets can be directly used in GitHub Actions workflows.

## Recommended Migration Plan

### Phase 1: Preparation (Weeks 1-2)
1. **Set up self-hosted runners**:
   - Configure Linux, Windows, and macOS runners
   - Install required build tools and dependencies
   - Test runner connectivity and labels

2. **Migrate secrets**:
   - Add Anaconda tokens to GitHub Secrets (organization or repository level)
   - Configure repository settings for required status checks

3. **Create base workflows**:
   - Start with simpler jobs (doxygen, merge conflict checks)
   - Test artifact upload/download mechanisms

### Phase 2: PR Build Migration (Weeks 3-5)
1. **Implement PR workflows**:
   - Create platform-specific PR build workflows
   - Implement matrix builds for parallel execution
   - Add test result reporting (using dorny/test-reporter or similar)

2. **Migrate code quality checks**:
   - CppCheck workflow
   - Doxygen workflow (can use GitHub-hosted runner)
   - Remove redundant formatting checks (handled by pre-commit.ci)

3. **Parallel testing**:
   - Run both Jenkins and GitHub Actions in parallel
   - Compare results and timing
   - Fix any discrepancies

### Phase 3: Nightly Build Migration (Weeks 6-8)
1. **Create nightly workflow**:
   - Implement multi-platform build and test
   - Add Conda packaging steps
   - Add standalone package creation

2. **Publishing integration**:
   - Anaconda Cloud publishing
   - GitHub Releases creation
   - Cleanup jobs for old nightlies

3. **Testing and validation**:
   - Test on feature branches
   - Validate package quality
   - Confirm deployment to all channels

### Phase 4: Additional Jobs (Weeks 9-10)
1. **Performance tests**: Migrate performance benchmarking
2. **Developer docs**: Migrate gh-pages publishing
3. **Release builds**: Create release workflow

### Phase 5: Cutover and Decommission (Weeks 11-12)
1. **Final validation**: Full regression testing
2. **Documentation updates**:
   - Update dev-docs/source/AutomatedBuildProcess.rst
   - Update README.md badges
   - Create GitHub Actions runbook
3. **Jenkins decommission**:
   - Archive Jenkins jobs
   - Redirect build status badges
   - Update links in documentation

## GitHub Actions Features to Leverage

### 1. **Reusable Workflows**
Create shared workflows for common tasks:
```yaml
# .github/workflows/reusable-build.yml
on:
  workflow_call:
    inputs:
      platform:
        required: true
        type: string
```

### 2. **Composite Actions**
Package common steps into composite actions for reuse:
- Setup Conda environment
- Run CMake with preset
- Upload test results

### 3. **Environment Protection**
Use GitHub Environments for deployment:
- Anaconda production channel (requires approval)
- Anaconda test channel (automatic)
- GitHub Releases (automatic)

### 4. **Concurrency Control**
Prevent duplicate builds:
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

### 5. **Job Dependencies**
Define explicit job dependencies:
```yaml
jobs:
  build:
    # ...
  test:
    needs: build
    # ...
  package:
    needs: [build, test]
    # ...
```

## Cost and Resource Considerations

### GitHub-Hosted Runners
- **Limitation**: Insufficient resources for Mantid builds (as noted in issue #39497)
- **Not recommended** for: Main builds, system tests, packaging
- **Suitable for**: Doxygen, documentation, lightweight checks

### Self-Hosted Runners
- **Advantages**:
  - Full control over resources
  - No minute limits
  - Can use existing Jenkins infrastructure
  - Better for large builds

- **Considerations**:
  - Maintenance responsibility
  - Security (isolation between jobs)
  - Scaling (number of concurrent jobs)

### Estimated Timeline
- **Total duration**: 10-12 weeks
- **Parallel running period**: 4-6 weeks (Jenkins + GitHub Actions)
- **Buffer for issues**: 2 weeks

## Example Workflow Templates

### PR Build (Linux)
```yaml
name: PR Build - Linux

on:
  pull_request:
    branches: [main, release-next]
    paths:
      - '**.cpp'
      - '**.h'
      - '**.py'
      - 'CMakeLists.txt'
      - '.github/workflows/pr-build-linux.yml'

concurrency:
  group: pr-linux-${{ github.ref }}
  cancel-in-progress: true

jobs:
  build-and-test:
    runs-on: [self-hosted, linux, x64, linux-64]
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Build and Test
        run: |
          buildconfig/Jenkins/Conda/conda-buildscript \
            ${{ github.workspace }} \
            linux-64-ci \
            --clean-build \
            --clean-external-projects \
            --enable-systemtests \
            --enable-doctests

      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: test-results-linux
          path: build/Testing/**/*.xml

      - name: Publish Test Results
        if: always()
        uses: EnricoMi/publish-unit-test-result-action@v2
        with:
          files: build/Testing/**/*.xml
```

### Nightly Build (Simplified)
```yaml
name: Nightly Build and Deploy

on:
  schedule:
    - cron: '0 2 * * *'  # 2 AM UTC daily
  workflow_dispatch:
    inputs:
      platforms:
        description: 'Platforms to build'
        required: true
        default: 'all'
        type: choice
        options:
          - all
          - linux-64
          - win-64
          - osx-arm64

jobs:
  build:
    strategy:
      matrix:
        platform: [linux-64, win-64, osx-arm64]
    runs-on: [self-hosted, '${{ matrix.platform }}']
    if: inputs.platforms == 'all' || inputs.platforms == matrix.platform
    
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Build and Test
        run: |
          buildconfig/Jenkins/Conda/conda-buildscript \
            ${{ github.workspace }} \
            ${{ matrix.platform }}-ci \
            --clean-build \
            --enable-systemtests

      - name: Package Conda
        run: |
          buildconfig/Jenkins/Conda/package-conda \
            ${{ github.workspace }} \
            --build-mantid-developer \
            --build-mantid \
            --build-qt \
            --platform ${{ matrix.platform }}

      - name: Upload Conda Packages
        uses: actions/upload-artifact@v4
        with:
          name: conda-packages-${{ matrix.platform }}
          path: conda-bld/**/*.conda

  publish:
    needs: build
    runs-on: [self-hosted, linux, x64]
    steps:
      - name: Download Artifacts
        uses: actions/download-artifact@v4

      - name: Publish to Anaconda
        env:
          ANACONDA_TOKEN: ${{ secrets.ANACONDA_TOKEN_MANTID }}
        run: |
          buildconfig/Jenkins/Conda/publish-to-anaconda \
            ${{ github.workspace }} \
            $ANACONDA_TOKEN \
            mantid \
            nightly \
            conda-packages-*/*.conda
```

### Doxygen (GitHub-hosted)
```yaml
name: Doxygen Documentation

on:
  pull_request:
    paths:
      - '**.cpp'
      - '**.h'
      - 'Framework/Doxygen/**'

jobs:
  doxygen:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup Conda
        uses: conda-incubator/setup-miniconda@v3
        with:
          auto-activate-base: true
          activate-environment: ""

      - name: Build Doxygen
        run: |
          buildconfig/Jenkins/Conda/doxygen.sh ${{ github.workspace }}

      - name: Upload Documentation
        uses: actions/upload-artifact@v4
        with:
          name: doxygen-docs
          path: build/docs/html
```

## Security Considerations

### 1. **Secret Exposure**
- Use GitHub Secrets for all sensitive data
- Never log secrets in workflow output
- Use single quotes for token variables in scripts

### 2. **Pull Request Builds**
- Use `pull_request` trigger (not `pull_request_target`) for untrusted code
- Self-hosted runners should be in isolated environments
- Consider approval requirements for first-time contributors

### 3. **Self-Hosted Runner Security**
- Regularly update runner software
- Use ephemeral runners when possible
- Implement network restrictions
- Monitor resource usage

### 4. **Artifact Security**
- Set retention policies for artifacts
- Restrict artifact download permissions
- Clean up workspace after jobs

## Monitoring and Observability

### 1. **Status Badges**
Update README.md with GitHub Actions badges:
```markdown
![Build Status](https://github.com/mantidproject/mantid/workflows/PR%20Build/badge.svg)
```

### 2. **Notifications**
Configure workflow notifications:
- Slack integration
- Email on failure
- GitHub Discussions posts

### 3. **Metrics**
Track key metrics:
- Build success rate
- Average build time per platform
- Test pass rate
- Package deployment success

## Documentation Updates Required

1. **dev-docs/source/AutomatedBuildProcess.rst**:
   - Replace Jenkins references with GitHub Actions
   - Update screenshots and links
   - Document new workflow triggers and manual runs

2. **README.md**:
   - Update build status badge
   - Update link to builds (from builds.mantidproject.org to GitHub Actions)

3. **New Documentation**:
   - GitHub Actions runbook (how to trigger, monitor, debug)
   - Self-hosted runner setup guide
   - Troubleshooting guide

4. **buildconfig/Jenkins/leeroy.md**:
   - Archive or mark as deprecated
   - Add note about GitHub Actions migration

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Self-hosted runner downtime | Build failures | Implement runner monitoring, have backup runners |
| Migration bugs | Build/deployment failures | Parallel run period, gradual migration |
| Knowledge gap | Slow troubleshooting | Training, documentation, gradual rollout |
| Secret exposure | Security breach | Use GitHub Secrets, audit logs, least privilege |
| Package quality issues | User impact | Extensive testing in test channel before production |
| Performance degradation | Slower builds | Optimize workflows, use caching, monitor metrics |

## Open Questions

1. **Runner Infrastructure**:
   - Will existing Jenkins build machines be repurposed as GitHub Actions runners?
   - What is the provisioning process for new runners?

2. **ORNL-specific Builds**:
   - Special handling for ornl-next, ornl-qa, ornl branches
   - Anaconda channel `mantid-ornl` access and credentials

3. **Build History**:
   - Should historical Jenkins build data be archived?
   - How to preserve performance test database?

4. **Deployment Approvals**:
   - Should production deployments require manual approval?
   - Who should have approval permissions?

## Conclusion

Migrating from Jenkins to GitHub Actions is feasible and will provide several benefits:

### Benefits
1. **Native GitHub integration**: Better PR experience, no external service (Leeroy)
2. **Modern workflow syntax**: Easier to maintain and extend
3. **Community ecosystem**: Large library of actions and examples
4. **Reduced infrastructure**: Fewer services to maintain (eliminate Leeroy)
5. **Better visibility**: Integrated into GitHub UI

### Challenges
1. **Self-hosted runner setup**: Requires infrastructure work
2. **Migration effort**: 10-12 weeks estimated
3. **Learning curve**: Team needs to learn GitHub Actions
4. **Testing complexity**: Need to validate package quality during migration

### Recommendation
Proceed with migration using the phased approach outlined above. Start with simpler workflows (doxygen, documentation) to gain experience, then progressively migrate more complex jobs. Maintain parallel Jenkins/GitHub Actions execution during migration to ensure continuity and allow for comparison and validation.

The investment in migration will pay off through:
- Simplified CI/CD pipeline
- Better integration with GitHub workflow
- Reduced maintenance burden (one less service to maintain)
- Improved developer experience
- Modern, well-documented platform

## Next Steps

1. Present this research to the team for review and feedback
2. Get approval for self-hosted runner infrastructure
3. Begin Phase 1 (Preparation) activities
4. Create tracking issues for each phase of migration
5. Assign ownership and timeline for migration tasks
