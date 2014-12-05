#git:/slp/pkgs/e/emulator-plugin-gyro-pkgs
Name: sf-plugin-gyro-emul-pkgs
Version: 0.2.10
Release: 1
Summary: gyro-sim plugins for sensor framework (using setting)
Group: System Environment/Libraries
License: GNUv2
Source0: %{name}-%{version}.tar.gz
Source1001: packaging/sf-plugin-gyro-emul-pkgs.manifest
BuildRequires: cmake
BuildRequires: pkgconfig(sf_common)
BuildRequires: pkgconfig(vconf)

%description

%prep
%setup -q

%build
export LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--as-needed"  
  
LDFLAGS="$LDFLAGS" cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}  

make

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.LGPLv2.1 %{buildroot}/usr/share/license/%{name}

%make_install

%clean
make clean
rm -rf CMakeCache.txt
rm -rf CMakeFiles
rm -rf cmake_install.cmake
rm -rf Makefile
rm -rf install_manifes.txt
rm -rf *.so
rm -rf sensor/CMakeFiles
rm -rf sensor/cmake_install.cmake
rm -rf sensor/Makefile
rm -rf sensor/*.so

rm -rf processor/CMakeFiles
rm -rf processor/cmake_install.cmake
rm -rf processor/Makefile
rm -rf processor/*.so

%post

%postun

%files
%defattr(-,root,root,-)
%{_prefix}/lib/sensor_framework/*.so*
/usr/share/license/%{name}

%changelog
