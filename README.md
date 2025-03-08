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

# Nutshell - A Modern Shell with Nut-themed Commands

Nutshell is a modern shell environment that combines traditional Unix-like command syntax with friendly nut-themed aliases and powerful package management capabilities.

## Features

- Familiar Unix-like command syntax
- Nut-themed command aliases (e.g., `peekaboo` for `ls`)
- Package management with integrity verification
- Input/output redirection
- Background process handling
- AI assistant integration (OpenAI)

## Directory Structure

```
nutshell/
├── src/
│   ├── core/        # Main shell functionality
│   ├── ai/          # AI integration components
│   ├── pkg/         # Package management
│   ├── utils/       # Utility functions
│   └── plugins/     # Loadable plugins
├── include/         # Header files
├── lib/             # 3rd party libraries
├── scripts/         # Build/install scripts
├── packages/        # Local package cache
├── tests/           # Test suite
├── Makefile
└── README.md
```

## Installation

### Prerequisites

- C compiler (clang or gcc)
- libreadline
- libcurl
- libjansson
- libssl

### Building from source

```bash
# Clone the repository
git clone https://github.com/nuttydev/nutshell.git
cd nutshell

# Build
make

# Install
sudo make install
```

## Usage

Start the shell by running:

```bash
nutshell
```

### Basic Commands

- `peekaboo` (`ls`): List files in the current directory
- `hop` (`cd`): Change directory
- `roast` (`exit`): Exit the shell

### Package Management

Nutshell uses a package system with `.nut` files:

```bash
# Install a package
nutpkg install nut-git

# List installed packages
nutpkg list

# Use a package command
acorn "Initial commit"  # From nut-git, equivalent to `git commit -m "Initial commit"`
```

## Development

### Running tests

```bash
make test
```

### Creating Packages

Packages are defined in `.nut` files with the following structure:

```json
{
  "name": "nut-example",
  "version": "1.0.0",
  "description": "Example package for Nutshell",
  "commands": {
    "nutcommand": "unix-command",
  },
  "dependencies": [
    "nut-dependency"
  ],
  "author": "Your Name"
}
```

## License

MIT License

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.