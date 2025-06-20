#!/bin/bash

# Build script for Bitcoin Puzzle #73 Kangaroo Solver
# Cross-platform build script

set -e

echo "Bitcoin Puzzle #73 Kangaroo Solver - Build Script"
echo "=================================================="

# Detect operating system
OS=$(uname -s)
ARCH=$(uname -m)

echo "Detected OS: $OS"
echo "Detected Architecture: $ARCH"

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install dependencies on different systems
install_dependencies() {
    echo "Checking dependencies..."
    
    # In Replit, dependencies should be installed via Nix
    if [ -n "$REPL_ID" ]; then
        echo "Running in Replit environment - dependencies should be managed via Nix"
        echo "Required dependencies: gcc, g++, gmp, openssl, jsoncpp, pkg-config"
        
        # Check if essential tools are available
        if ! command_exists g++; then
            echo "Error: g++ compiler not found. Please install gcc via Nix."
            exit 1
        fi
        
        if ! command_exists pkg-config; then
            echo "Error: pkg-config not found. Please install pkg-config via Nix."
            exit 1
        fi
        
        echo "Essential build tools found."
        return 0
    fi
    
    case "$OS" in
        Linux*)
            if command_exists apt-get; then
                # Ubuntu/Debian
                echo "Installing dependencies for Ubuntu/Debian..."
                sudo apt-get update
                sudo apt-get install -y \
                    build-essential \
                    libgmp-dev \
                    libssl-dev \
                    libjsoncpp-dev \
                    python3-dev \
                    python3-pip \
                    pkg-config
            elif command_exists yum; then
                # CentOS/RHEL
                echo "Installing dependencies for CentOS/RHEL..."
                sudo yum install -y \
                    gcc-c++ \
                    gmp-devel \
                    openssl-devel \
                    jsoncpp-devel \
                    python3-devel \
                    python3-pip
            elif command_exists pacman; then
                # Arch Linux
                echo "Installing dependencies for Arch Linux..."
                sudo pacman -S --needed \
                    gcc \
                    gmp \
                    openssl \
                    jsoncpp \
                    python \
                    python-pip
            else
                echo "Unsupported Linux distribution. Please install dependencies manually."
                echo "Required packages: gcc/g++, libgmp-dev, libssl-dev, libjsoncpp-dev, python3-dev"
                exit 1
            fi
            ;;
        Darwin*)
            # macOS
            echo "Installing dependencies for macOS..."
            if command_exists brew; then
                brew install gmp openssl jsoncpp python3
            else
                echo "Homebrew not found. Please install Homebrew first:"
                echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            ;;
        MINGW*|MSYS*|CYGWIN*)
            # Windows
            echo "Windows detected. Please use the Windows build instructions."
            echo "Install dependencies using vcpkg or similar package manager."
            exit 1
            ;;
        *)
            echo "Unsupported operating system: $OS"
            exit 1
            ;;
    esac
}

# Function to install Python dependencies
install_python_deps() {
    echo "Installing Python dependencies..."
    
    if command_exists pip3; then
        pip3 install --user flask requests
    elif command_exists pip; then
        pip install --user flask requests
    else
        echo "pip not found. Please install pip first."
        exit 1
    fi
}

# Function to build the library
build_library() {
    echo "Building Kangaroo solver library..."
    
    # Set compiler flags - remove OpenMP for now to simplify build
    CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -fPIC"
    LDFLAGS="-lgmp -lcrypto"
    
    # Platform-specific adjustments
    case "$OS" in
        Darwin*)
            # macOS specific flags
            CXXFLAGS="$CXXFLAGS -I/usr/local/include -I/opt/homebrew/include"
            LDFLAGS="$LDFLAGS -L/usr/local/lib -L/opt/homebrew/lib"
            LIB_EXT="dylib"
            ;;
        Linux*)
            LIB_EXT="so"
            ;;
        *)
            LIB_EXT="so"
            ;;
    esac
    
    # Check for pkg-config and use it if available
    if command_exists pkg-config; then
        echo "Using pkg-config for library detection..."
        
        # Try to find libraries with pkg-config
        if pkg-config --exists gmp 2>/dev/null; then
            CXXFLAGS="$CXXFLAGS $(pkg-config --cflags gmp)"
            LDFLAGS="$LDFLAGS $(pkg-config --libs gmp)"
        fi
        
        if pkg-config --exists openssl 2>/dev/null; then
            CXXFLAGS="$CXXFLAGS $(pkg-config --cflags openssl)"
            LDFLAGS="$LDFLAGS $(pkg-config --libs openssl)"
        fi
        
        if pkg-config --exists jsoncpp 2>/dev/null; then
            CXXFLAGS="$CXXFLAGS $(pkg-config --cflags jsoncpp)"
            LDFLAGS="$LDFLAGS $(pkg-config --libs jsoncpp)"
        else
            # Fallback for jsoncpp
            LDFLAGS="$LDFLAGS -ljsoncpp"
        fi
    fi
    
    echo "Using CXXFLAGS: $CXXFLAGS"
    echo "Using LDFLAGS: $LDFLAGS"
    
    # Compile simplified version
    echo "Compiling simplified kangaroo solver..."
    
    echo "Compiling simple_kangaroo.cpp..."
    if ! g++ $CXXFLAGS -c simple_kangaroo.cpp -o simple_kangaroo.o; then
        echo "Failed to compile simple_kangaroo.cpp"
        return 1
    fi
    
    # Link shared library
    echo "Linking shared library..."
    if ! g++ -shared simple_kangaroo.o -o libkangaroo.$LIB_EXT -lpthread; then
        echo "Failed to link shared library"
        return 1
    fi
    
    echo "Build completed successfully!"
    echo "Library: libkangaroo.$LIB_EXT"
    return 0
}

# Function to test the build
test_build() {
    echo "Testing the build..."
    
    # Test library loading
    python3 -c "
import ctypes
import os
import sys

# Try to load the library
lib_names = ['./libkangaroo.so', './libkangaroo.dylib', './kangaroo.dll']
lib = None

for lib_name in lib_names:
    if os.path.exists(lib_name):
        try:
            lib = ctypes.CDLL(lib_name)
            print(f'✓ Successfully loaded {lib_name}')
            break
        except Exception as e:
            print(f'✗ Failed to load {lib_name}: {e}')

if lib is None:
    print('✗ Could not load any library')
    sys.exit(1)
else:
    print('✓ Library test passed')
"
    
    if [ $? -eq 0 ]; then
        echo "✓ Build test passed!"
    else
        echo "✗ Build test failed!"
        exit 1
    fi
}

# Function to clean build files
clean_build() {
    echo "Cleaning build files..."
    rm -f *.o
    rm -f libkangaroo.so libkangaroo.dylib kangaroo.dll
    rm -f *.checkpoint *.dat *.log
    echo "Clean completed!"
}

# Main script logic
case "${1:-build}" in
    deps|dependencies)
        install_dependencies
        install_python_deps
        ;;
    build)
        build_library
        ;;
    test)
        test_build
        ;;
    clean)
        clean_build
        ;;
    all|setup)
        install_dependencies
        install_python_deps
        build_library
        test_build
        echo ""
        echo "🎉 Setup completed successfully!"
        echo ""
        echo "To run the solver:"
        echo "  python3 main.py"
        echo ""
        echo "To access the web interface:"
        echo "  Open http://localhost:5000 in your browser"
        ;;
    help|--help|-h)
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  deps, dependencies - Install system dependencies"
        echo "  build              - Build the library (default)"
        echo "  test               - Test the build"
        echo "  clean              - Clean build files"
        echo "  all, setup         - Full setup (deps + build + test)"
        echo "  help               - Show this help"
        echo ""
        echo "Examples:"
        echo "  $0                 # Build the library"
        echo "  $0 setup           # Full setup from scratch"
        echo "  $0 clean           # Clean build files"
        ;;
    *)
        echo "Unknown command: $1"
        echo "Use '$0 help' for usage information."
        exit 1
        ;;
esac

echo "Done!"
