# Agent Configuration

This file configures automated tasks and agents for the Mantid project.

## Pre-commit Hook

### Task: Run Pre-commit Checks

**Description**: Runs all pre-commit hooks across all files before committing code.

**Command**:
```bash
pre-commit run --all-files
```

**When to Run**: Before committing code changes

**Purpose**:
- Ensures code quality and consistency
- Catches formatting issues early
- Validates file structure and content
- Runs security checks with gitleaks
- Enforces project coding standards

**Prerequisites**:
- pre-commit must be installed (`pip install pre-commit`)
- Pre-commit hooks must be installed (`pre-commit install`)

**Configuration**: See `.pre-commit-config.yaml` for the list of hooks that will be executed.

## Usage

To run the pre-commit checks manually:

```bash
pre-commit run --all-files
```

To run pre-commit on staged files only:

```bash
pre-commit run
```

To skip pre-commit hooks temporarily (not recommended):

```bash
git commit --no-verify
```
