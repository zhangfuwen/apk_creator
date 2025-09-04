export VCPKG_ROOT="$HOME/bin/vcpkg"
[[ -d $VCPKG_ROOT ]] ||  git clone https://github.com/microsoft/vcpkg "$HOME/bin/vcpkg"
export PATH=$PATH:$VCPKG_ROOT/
export VCPKG_DEFAULT_TRIPLET=arm64-android
export EDITOR=nvim

# rust specific env vars
VCPKGRS_TRIPLET=arm64-android
