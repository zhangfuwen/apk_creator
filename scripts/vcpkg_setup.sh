export VCPKG_ROOT="$HOME/bin/vcpkg"
if [[ ! -d $VCPKG_ROOT ]] ; then  
    mkdir -p "$HOME/bin/vcpkg"
    git clone https://github.com/microsoft/vcpkg "$HOME/bin/vcpkg"
fi

export PATH=$PATH:$VCPKG_ROOT/
export VCPKG_DEFAULT_TRIPLET=arm64-android
export EDITOR=nvim

# rust specific env vars
export VCPKGRS_TRIPLET=arm64-android
