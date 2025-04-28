# Build & Development Guide

### Clone the Repository (3 Ways)

1. **Without Terminal**  
   Download `.zip` from the GitHub page.  
   > Tip: `cd` means "Change Directory (Folder)". `cd ../..` means go up two directories.

2. **Using Git**
```sh
git clone https://github.com/setbe/your_echolyps
```

3. **Using SSH**
```sh
git clone git@github.com:setbe/your_echolyps.git
```

Then:
```sh
cd your_echolyps
```

---

## 🐧 Linux

### Install dependencies
**Debian / Ubuntu / Linux Mint**
```sh
sudo apt install libx11-dev libgl-dev
```

**Arch Linux / Manjaro**
```sh
sudo pacman -S libx11 mesa
```

**Fedora / RHEL / CentOS (dnf)**
```sh
sudo dnf install libX11-devel mesa-libGL-devel
```

**openSUSE (zypper)**
```sh
sudo zypper install libX11-devel Mesa-libGL-devel
```

**Alpine Linux**
```sh
sudo apk add libx11-dev mesa-dev
```

### Resources
**Shaders**
```sh
make shaders
```
Search for expected output:
```
...
Generated "/home/.../your_echolyps/src/shaders.hpp"
...
```

**Font**
```sh
make font
```
Search for expected output:
```
...
[info] Generated ../../src/fonts.hpp with XXX glyphs in WWWxHHH atlas.
...
```

### Compile
```sh
make clean && make release
```

Output binary: `build/echolyps`

### Run
```sh
./build/echolyps
```

### 🧪 Developer Utilities

- (debug) Clean + Build + Run:
  ```sh
  make
  ```

  Create public release binary:
  ```sh
  make clean && make public
  ``` 

---

## 🪟 Windows

### Visual Studio Setup

Ensure you've cloned the repo.  
[Download Visual Studio](https://visualstudio.microsoft.com/downloads/) with C/C++ support.

### Shaders

```sh
cd your_echolyps/resources/shaders/vs
```

1. Open `shader2header.sln`
2. In Visual Studio: **Build → Build Solution**
3. Confirm that `shader2header.exe` appears in `vs/build`
4. Run `compile.bat` from `your_echolyps/resources/shaders`

Expected output:
```
Generated "/.../your_echolyps/src/shaders.hpp"
```

### Font
SOON. IT WON'T WORK RIGHT NOW I MEAN GAME BUILD.

### Compile

1. Open `your_echolyps/vs/echolyps.sln` in Visual Studio  
2. Set configuration:
   - Build: `Release`
   - Platform: `x64` (or `x86` for very old PCs)
3. Build the solution

Output: `your_echolyps/vs/build/echolyps.exe`

---

## Facts

The compiled game is fully portable.  
All external libraries can be modified by the project.

---

## 🙏 Acknowledgements

- [datenwolf](https://github.com/datenwolf) – for the excellent linear math library [linmath.h](https://github.com/datenwolf/linmath.h)
- [glad](https://glad.dav1d.de/) – OpenGL function loader
- Khronos Group – for defining OpenGL
- GPU manufacturers – for supporting OpenGL
