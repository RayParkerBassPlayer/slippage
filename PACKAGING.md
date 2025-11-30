# Packaging Guide

This document explains how to build and distribute Slippage packages.

## Prerequisites

### For Debian Package Building

```bash
sudo apt-get install dpkg-dev debhelper cmake g++
```

Optional (for signing packages):
```bash
sudo apt-get install devscripts
```

## Building a Debian Package

### Quick Build

Use the provided script:

```bash
./build-deb.sh
```

This creates an unsigned `.deb` package in the parent directory.

### Manual Build

```bash
# Build unsigned package
dpkg-buildpackage -us -uc -b

# Build signed package (requires GPG key)
dpkg-buildpackage -b
```

### Package Output

The build creates:
- `../slippage_1.0.0_amd64.deb` - The installable package
- `../slippage_1.0.0_amd64.buildinfo` - Build information
- `../slippage_1.0.0_amd64.changes` - Changes description

## Installing the Package

```bash
# Install
sudo dpkg -i ../slippage_1.0.0_amd64.deb

# Verify installation
which slippage
slippage --help

# Check installed files
dpkg -L slippage
```

## Uninstalling

```bash
# Remove package
sudo apt-get remove slippage

# Remove package and configuration
sudo apt-get purge slippage
```

## Package Contents

The Debian package installs:

| Path | Description |
|------|-------------|
| `/usr/bin/slippage` | Main executable |
| `/usr/share/doc/slippage/README.md` | User documentation |
| `/usr/share/doc/slippage/ASSIGNMENT_RULES.md` | Assignment rules guide |
| `/usr/share/doc/slippage/examples/` | Example CSV files |
| `/usr/share/doc/slippage/copyright` | License information |
| `/usr/share/doc/slippage/changelog.gz` | Version history |

## CMake Installation

Alternative to Debian packaging for systems without dpkg.

### System-wide Installation

```bash
# Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4

# Install to /usr/local (default)
sudo cmake --install build

# Binary installed to: /usr/local/bin/slippage
# Docs installed to: /usr/local/share/doc/slippage/
```

### Custom Installation Prefix

```bash
# Install to custom location
cmake --install build --prefix /opt/slippage

# Binary: /opt/slippage/bin/slippage
# Docs: /opt/slippage/share/doc/slippage/
```

### User-local Installation

```bash
# Install to home directory (no sudo needed)
cmake --install build --prefix ~/.local

# Add to PATH if needed
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
```

## Customizing the Package

### Version Number

Edit `debian/changelog`:

```
slippage (1.1.0) unstable; urgency=medium

  * Your changes here

 -- Your Name <email@example.com>  Date
```

### Package Metadata

Edit `debian/control`:
- Change maintainer information
- Update dependencies
- Modify package description

### Build Options

Edit `debian/rules` to change CMake options:

```makefile
override_dh_auto_configure:
	dh_auto_configure -- \
		-DCMAKE_BUILD_TYPE=Release \
		-DCUSTOM_OPTION=ON
```

## Testing the Package

### Before Publishing

1. **Build the package:**
   ```bash
   ./build-deb.sh
   ```

2. **Test installation in clean environment:**
   ```bash
   # Using Docker
   docker run -it --rm -v $PWD/..:/packages ubuntu:22.04
   apt-get update && apt-get install -y /packages/slippage_*.deb
   slippage --help
   ```

3. **Check package quality:**
   ```bash
   lintian ../slippage_*.deb
   ```

4. **Verify all files:**
   ```bash
   dpkg -c ../slippage_*.deb
   ```

## Distribution

### Local Repository

Create a simple repository:

```bash
# Create repository directory
mkdir -p ~/apt-repo

# Copy package
cp ../slippage_*.deb ~/apt-repo/

# Create Packages index
cd ~/apt-repo
dpkg-scanpackages . /dev/null | gzip -9c > Packages.gz

# Add to sources.list
echo "deb [trusted=yes] file:///home/user/apt-repo ./" | \
  sudo tee /etc/apt/sources.list.d/slippage.list
```

### PPA (Ubuntu)

For Ubuntu PPA distribution, see: https://help.launchpad.net/Packaging/PPA

### GitHub Releases

1. Tag the release:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. Build the package:
   ```bash
   ./build-deb.sh
   ```

3. Upload to GitHub releases:
   - Go to repository → Releases → Create new release
   - Upload the `.deb` file

## Troubleshooting

### Build Fails

Check build dependencies:
```bash
dpkg-checkbuilddeps
```

### Package Won't Install

Check dependencies:
```bash
dpkg -I ../slippage_*.deb
```

### Missing Files

Verify install rules in `debian/slippage.install` and `CMakeLists.txt`

### Permission Errors

Ensure `debian/rules` is executable:
```bash
chmod +x debian/rules
```

## Updating the Package

1. Update version in `debian/changelog`
2. Make code changes
3. Rebuild package: `./build-deb.sh`
4. Test installation
5. Distribute new package
