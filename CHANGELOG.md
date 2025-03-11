# Changelog

All notable changes to Nutshell will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.4] - 2025-03-11

### Added
- Directory-level configuration with `.nutshell.json` files
- Configuration hierarchy: directory configs override user configs which override system configs
- Automatic config reloading when changing directories with `cd`
- Project-specific aliases and settings through directory configs
- Support for different themes per project

### Fixed
- Memory leak in directory path traversal
- Config loading order to properly respect precedence rules