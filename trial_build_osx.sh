#!/bin/bash

export CFLAGS='-O0 -g -w -pipe -m32 -march=native -mmacosx-version-min=10.8 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export CMAKE_FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/System/Library/Frameworks
export CMAKE_PREFIX_PATH=/usr/local/cbits-build-i386/homebrew/opt/icu4c:/usr/local/cbits-build-i386/homebrew:/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/usr
export CPATH=/usr/local/cbits-build-i386/homebrew/include:/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk/usr/include
export CPPFLAGS='-I/usr/local/cbits-build-i386/homebrew/opt/icu4c/include -isystem/usr/local/cbits-build-i386/homebrew/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export CXXFLAGS='-O0 -g -w -pipe -m32 -march=native -mmacosx-version-min=10.8 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export HOMEBREW_BREW_FILE=/usr/local/cbits-build-i386/homebrew/bin/brew
export HOMEBREW_BUILD_FOR_DEBUGGING=1
export HOMEBREW_NO_EMOJI=1
export HOMEBREW_PRESERVE_STAGE=1
export HOMEBREW_TEMP=/usr/local/cbits-build-i386/homebrew-build-temp
export LDFLAGS='-L/usr/local/cbits-build-i386/homebrew/opt/icu4c/lib -L/usr/local/cbits-build-i386/homebrew/lib  -Wl,-headerpad_max_install_names -Wl,-rpath,@executable_path/../libs -Wl,-rpath,@executable_path/../ruby19/lib -arch i386 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export MACHTYPE=x86_64-apple-darwin13
export MACOSX_DEPLOYMENT_TARGET=10.8
export OBJCFLAGS='-O0 -g -w -pipe -m32 -march=native -mmacosx-version-min=10.8 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export OBJCXXFLAGS='-O0 -g -w -pipe -m32 -march=native -mmacosx-version-min=10.8 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk'
export PKG_CONFIG_FOR_BUILD=/usr/local/cbits-build-i386/homebrew/bin/pkg-config
export PKG_CONFIG_LIBDIR=/usr/local/cbits-build-i386/homebrew/lib/pkgconfig:/usr/local/cbits-build-i386/homebrew/Library/ENV/pkgconfig/10.9:/usr/lib/pkgconfig
export PKG_CONFIG_PATH=/usr/local/cbits-build-i386/homebrew/opt/icu4c/lib/pkgconfig
export VERSIONER_PYTHON_PREFER_32_BIT=yes

export PATH="/usr/local/cbits-build-i386/homebrew/bin:$PATH"

#./configure --enable-debug=yes --disable-dependency-tracking --prefix=/usr/local/cbits-build-i386/homebrew/Cellar/gtk3-cbits/cbits --disable-glibtest --enable-introspection=yes --disable-schemas-compile --enable-quartz-backend --enable-quartz-relocation
#exit

#make V=1
make "$@"
