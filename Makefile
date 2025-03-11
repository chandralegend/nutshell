CC = clang
CFLAGS = -std=c17 -Wall -Wextra -I./include -I/usr/local/include -I/opt/homebrew/include

# Base libraries that are required - add OpenSSL
LDFLAGS = -lreadline -lcurl -ldl -lssl -lcrypto -L/usr/local/lib

# Check if pkg-config exists
PKG_CONFIG_EXISTS := $(shell which pkg-config >/dev/null 2>&1 && echo "yes" || echo "no")

# Check if jansson is available through brew
ifeq ($(PKG_CONFIG_EXISTS),yes)
    JANSSON_AVAILABLE := $(shell pkg-config --exists jansson && echo "yes" || echo "no")
    ifeq ($(JANSSON_AVAILABLE),yes)
        JANSSON_CFLAGS := $(shell pkg-config --cflags jansson)
        JANSSON_LIBS := $(shell pkg-config --libs jansson)
    endif
else
    JANSSON_AVAILABLE := $(shell brew list jansson >/dev/null 2>&1 && echo "yes" || echo "no")
    ifeq ($(JANSSON_AVAILABLE),yes)
        JANSSON_CFLAGS := -I$(shell brew --prefix jansson 2>/dev/null || echo "/usr/local")/include
        JANSSON_LIBS := -L$(shell brew --prefix jansson 2>/dev/null || echo "/usr/local")/lib -ljansson
    endif
endif

ifeq ($(JANSSON_AVAILABLE),yes)
    CFLAGS += -DJANSSON_AVAILABLE=1 $(JANSSON_CFLAGS)
    LDFLAGS += $(JANSSON_LIBS)
else
    CFLAGS += -DJANSSON_AVAILABLE=0
    $(warning Jansson library not found. Package management functionality will be limited.)
endif

SRC = $(wildcard src/core/*.c) \
      $(wildcard src/ai/*.c) \
      $(wildcard src/pkg/*.c) \
      $(wildcard src/utils/*.c)
OBJ = $(SRC:.c=.o)

TEST_SRC = $(wildcard tests/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BINS = $(TEST_SRC:.c=.test)

.PHONY: all clean install install-user test test-pkg test-theme test-ai test-config release uninstall uninstall-user

all: nutshell

nutshell: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Make sure install depends on building nutshell first
install: nutshell
	@echo "Installing nutshell..."
	@if [ -w $(DESTDIR)/usr/local/bin ] && mkdir -p $(DESTDIR)/usr/local/nutshell/packages 2>/dev/null; then \
		mkdir -p $(DESTDIR)/usr/local/bin && \
		cp nutshell $(DESTDIR)/usr/local/bin && \
		echo "Installation completed successfully in system directories."; \
	else \
		echo "Error: Permission denied when installing to system directories."; \
		echo "You can either:"; \
		echo "  1. Use 'sudo make install' to install with admin privileges"; \
		echo "  2. Use 'make install-user' to install in your home directory"; \
		exit 1; \
	fi

# User-level installation (doesn't require sudo)
install-user: nutshell
	@echo "Installing nutshell in user's home directory..."
	@mkdir -p $(HOME)/bin
	@cp nutshell $(HOME)/bin/
	@mkdir -p $(HOME)/.nutshell/packages
	@echo "Installation completed successfully in $(HOME)/bin."
	@echo "Make sure $(HOME)/bin is in your PATH variable."
	@echo "You can add it by running: echo 'export PATH=\$$PATH:\$$HOME/bin' >> ~/.bashrc"

# Uninstall system-wide installation (requires sudo)
uninstall:
	@echo "Uninstalling nutshell from system directories..."
	@rm -f $(DESTDIR)/usr/local/bin/nutshell
	@rm -rf $(DESTDIR)/usr/local/nutshell
	@echo "Nutshell has been uninstalled from system directories."

# Uninstall user-level installation
uninstall-user:
	@echo "Uninstalling nutshell from user directory..."
	@rm -f $(HOME)/bin/nutshell
	@rm -rf $(HOME)/.nutshell
	@echo "Nutshell has been uninstalled from user directory."
	@echo "You may want to remove ~/bin from your PATH if you don't use it for other programs."

# Standard test target
test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		./$$test; \
	done

# Add a new target for package tests
test-pkg: tests/test_pkg_install.test
	@echo "Running package installation tests..."
	@chmod +x scripts/run_pkg_test.sh
	@./scripts/run_pkg_test.sh

# Add a new target for theme tests
test-theme: tests/test_theme.test
	@echo "Running theme tests..."
	@./tests/test_theme.test

# Add a new target for AI tests
test-ai: tests/test_ai_integration.test tests/test_openai_commands.test tests/test_ai_shell_integration.test
	@echo "Running AI integration tests..."
	@./tests/test_ai_integration.test
	@./tests/test_openai_commands.test
	@./tests/test_ai_shell_integration.test
	@echo "All AI tests completed!"

# Add a new target for config tests
test-config: tests/test_config.test
	@echo "Running configuration system tests..."
	@./tests/test_config.test

# Update the test build rule to exclude main.o
tests/%.test: tests/%.o $(filter-out src/core/main.o, $(OBJ))
	$(CC) -o $@ $^ $(LDFLAGS)

# Add a release target that calls the build script
release:
	@echo "Building release package..."
	@chmod +x scripts/build_release.sh
	@./scripts/build_release.sh $(VERSION)
	@echo "Release build complete."

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TEST_BINS) nutshell