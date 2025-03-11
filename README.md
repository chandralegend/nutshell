# Nutshell 🥜

Nutshell is an enhanced Unix shell that provides a simplified command language, package management, and AI-powered assistance.

## Features

- Friendly command aliases (e.g., `peekaboo` for `ls`, `hop` for `cd`)
- Built-in package management system
- Dynamic package installation without rebuilding
- Debug logging for development
- Interactive Git commit helper (gitify package)
- Shell command history
- Redirection and background process support
- AI-powered command assistance (NEW)

## Installation

### Prerequisites

- C compiler (clang or gcc)
- readline library
- OpenSSL
- libcurl
- Jansson (optional, for package management)

### Building from source

```bash
git clone https://github.com/chandralegend/nutshell.git
cd nutshell
make
```

### Installing 

#### System-wide installation (requires sudo)
```bash
sudo make install
```

#### User-level installation
```bash
make install-user
```
Then add `$HOME/bin` to your PATH if it's not already there:
```bash
echo 'export PATH=$PATH:$HOME/bin' >> ~/.bashrc  # or ~/.zshrc or ~/.bash_profile
```

## Usage

### Basic commands

Nutshell command | Unix equivalent | Description
---------------- | --------------- | -----------
`peekaboo`       | `ls`            | List directory contents
`hop`            | `cd`            | Change directory
`roast`          | `exit`          | Exit the shell

### Running commands

Commands work just like in a standard Unix shell:

```bash
🥜 ~/projects ➜ peekaboo -la
🥜 ~/projects ➜ hop nutshell
🥜 ~/projects/nutshell ➜ command arg1 arg2
```

## AI Command Assistance

Nutshell includes AI features to help with shell commands:

### Setup

1. Get an OpenAI API key from [OpenAI Platform](https://platform.openai.com/)
2. Set your API key:
   ```bash
   🥜 ~ ➜ set-api-key YOUR_API_KEY
   ```
   
   Alternatively, set it as an environment variable:
   ```bash
   export OPENAI_API_KEY=your_api_key
   ```

### AI Commands

#### Ask for a command

Convert natural language to shell commands:

```bash
🥜 ~ ➜ ask find all PDF files modified in the last week
```

The shell will return the proper command and ask if you want to execute it.

#### Explain commands

Get explanations for complex commands:

```bash
🥜 ~ ➜ explain find . -name "*.txt" -mtime -7 -exec grep -l "important" {} \;
```

#### Fix commands

Automatically fix common command errors:

```bash
🥜 ~ ➜ torch apple.txt
🥜 ~ ➜ fix # Will suggest to use touch apple.txt instead.
```

The shell will suggest corrections for common mistakes and ask if you want to apply them.

### Debug Options

Enable AI debugging with environment variables:

```bash
# Run with AI debugging enabled
NUT_DEBUG_AI=1 ./nutshell

# Run with verbose API response logging
NUT_DEBUG_AI=1 NUT_DEBUG_AI_VERBOSE=1 ./nutshell

# Use the debug script
./scripts/debug_ai.sh
```

## Package Management

Nutshell has a built-in package system for extending functionality.

### Installing packages

```
🥜 ~/projects ➜ install-pkg gitify
```

You can also install from a local directory:

```
🥜 ~/projects ➜ install-pkg /path/to/package
```

### Using the gitify package

The gitify package provides an interactive Git commit helper:

```
🥜 ~/projects/my-git-repo ➜ gitify
```

Follow the prompts to stage files and create semantic commit messages.

## Customizing Themes

Nutshell comes with a customizable theme system:

### Listing available themes

```bash
🥜 ~ ➜ theme
```

This will show all available themes, with the current theme marked with an asterisk (*).

### Switching themes

```bash
🥜 ~ ➜ theme minimal
```

This will switch to the "minimal" theme.

### Creating your own theme

1. Create a new JSON file in `~/.nutshell/themes/` named `mytheme.json`
2. Use the following template:

```json
{
  "name": "mytheme",
  "description": "My custom theme",
  "colors": {
    "reset": "\u001b[0m",
    "primary": "\u001b[1;35m",  // Purple
    "secondary": "\u001b[1;33m", // Yellow
    "error": "\u001b[1;31m",
    "warning": "\u001b[0;33m",
    "info": "\u001b[0;34m",
    "success": "\u001b[0;32m"
  },
  "prompt": {
    "left": {
      "format": "{primary}{icon} {directory}{reset} ",
      "icon": "🌟"
    },
    "right": {
      "format": "{git_branch}"
    },
    "multiline": false,
    "prompt_symbol": "→ ",
    "prompt_symbol_color": "primary"
  },
  "segments": {
    "git_branch": {
      "enabled": true,
      "format": "{secondary}git:({branch}){reset} ",
      "command": "git branch --show-current 2>/dev/null"
    },
    "directory": {
      "format": "{directory}",
      "command": "pwd | sed \"s|$HOME|~|\""
    }
  }
}
```

3. Switch to your theme with `theme mytheme`

## Directory-level Configuration

Nutshell now supports project-specific configurations through directory-level config files:

### How it works

- Nutshell searches for a `.nutshell.json` configuration file in the current directory
- If not found, it looks in parent directories until reaching your home directory
- Directory configs take precedence over user configs which take precedence over system configs
- Configurations are automatically reloaded when you change directories using `cd`

### Creating a directory config

Create a `.nutshell.json` file in your project directory:

```json
{
  "theme": "minimal",
  "aliases": {
    "build": "make all",
    "test": "make test",
    "deploy": "scripts/deploy.sh"
  },
  "packages": ["gitify"]
}
```

### Benefits

- Project-specific aliases and settings
- Different themes for different projects
- Shared configurations for team projects (add `.nutshell.json` to version control)
- Hierarchical configuration (team settings in parent dir, personal tweaks in subdirs)

## Debugging

For development or troubleshooting, run the debug script:

```bash
./scripts/debug-nutshell.sh
```

This enables detailed logging of command parsing and execution.

### Debug Flags

Nutshell supports the following debug environment variables:

- `NUT_DEBUG=1` - Enable general debug output
- `NUT_DEBUG_THEME=1` - Enable theme system debugging
- `NUT_DEBUG_PKG=1` - Enable package system debugging
- `NUT_DEBUG_PARSER=1` - Enable command parser debugging
- `NUT_DEBUG_EXEC=1` - Enable command execution debugging
- `NUT_DEBUG_REGISTRY=1` - Enable command registry debugging
- `NUT_DEBUG_AI=1` - Enable AI integration debugging
- `NUT_DEBUG_AI_SHELL=1` - Enable AI shell integration debugging
- `NUT_DEBUG_AI_VERBOSE=1` - Enable verbose API response logging

Example:

```bash
# Run with theme debugging enabled
NUT_DEBUG_THEME=1 ./nutshell

# Run with all debugging enabled
NUT_DEBUG=1 NUT_DEBUG_THEME=1 NUT_DEBUG_PARSER=1 NUT_DEBUG_EXEC=1 NUT_DEBUG_REGISTRY=1 ./nutshell
```

## Creating Packages

Packages are directories containing:
- `manifest.json`: Package metadata
- `[package-name].sh`: Main executable script
- Additional files as needed

Example manifest.json:
```json
{
  "name": "mypackage",
  "version": "1.0.0",
  "description": "My awesome package",
  "author": "Your Name",
  "dependencies": [],
  "sha256": "checksum"
}
```

Generate a checksum for your package with:
```bash
./scripts/generate_checksum.sh mypackage
```

## Testing

```bash
make test       # Run all tests
make test-pkg   # Test package installation
make test-ai    # Test AI integration
```

## Contributing

Contributions are welcome! Please read the [contributing guidelines](CONTRIBUTING.md) before submitting a pull request.

```bash
nutshell/
├── src/
│   ├── core/
│   │   ├── shell.c          # Main shell loop
│   │   ├── parser.c         # Command parsing
│   │   └── executor.c       # Command execution
│   ├── ai/
│   │   ├── openai.c         # OpenAI integration
│   │   └── local_ml.c       # On-device ML
│   ├── pkg/
│   │   ├── nutpkg.c         # Package manager core
│   │   └── registry.c       # Package registry handling
│   ├── utils/
│   │   ├── security.c       # Security features
│   │   ├── autocomplete.c   # Tab completion
│   │   └── helpers.c        # Utility functions
│   └── plugins/             # Loadable plugins
├── include/
│   ├── nutshell/
│   │   ├── core.h
│   │   ├── ai.h
│   │   ├── pkg.h
│   │   └── utils.h
├── lib/                     # 3rd party libs
├── scripts/                 # Build/install scripts
├── packages/                # Local package cache
├── tests/                   # Test suite
├── Makefile
└── README.md
```

## License

[MIT License](LICENSE)