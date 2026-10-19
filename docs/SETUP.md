# Setting up your laptop (do this once)

You need: **Git**, a **C++17 compiler**, **CMake 3.20+** and **Ninja**. GoogleTest is downloaded automatically the first time you build (needs internet).

## Windows (recommended: MSYS2)
1. Install Git for Windows: https://git-scm.com/download/win (keep all default options).
2. Install MSYS2: https://www.msys2.org (default folder `C:\msys64`).
3. Open the **"MSYS2 UCRT64"** terminal from the Start menu and run:
   ```bash
   pacman -Syu          # if it asks to close the window, close it, reopen "MSYS2 UCRT64" and run it again
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```
4. Add `C:\msys64\ucrt64\bin` to your Windows PATH (Start → "Edit the system environment variables" → Environment Variables → Path → New).
5. Open a **new** Command Prompt or PowerShell and check:
   ```
   g++ --version
   cmake --version
   ninja --version
   git --version
   ```
   All four must print a version. If one says "not recognised", step 4 was not done or the terminal was opened before it.

Build on Windows with:
```
cmake -S . -B build -G Ninja
cmake --build build
build\hms.exe
ctest --test-dir build --output-on-failure
```

## Linux / WSL (Ubuntu)
```bash
sudo apt update
sudo apt install -y git g++ cmake ninja-build
```

## macOS
```bash
xcode-select --install          # compiler + git
brew install cmake ninja        # needs Homebrew: https://brew.sh
```

## Editor
Any editor works. VS Code with the "C/C++" and "CMake Tools" extensions is easiest.

## Git identity (once per laptop)
```bash
git config --global user.name "Your Name"
git config --global user.email "the-email-on-your-github-account"
```

## GitHub login from the terminal
Install the GitHub CLI (https://cli.github.com) and run `gh auth login` (choose GitHub.com → HTTPS → login with browser).
After this, `git push` works without asking for a password.

## Common problems
| Problem | Fix |
|---|---|
| `cmake` says it cannot find a compiler | Run from a new terminal after fixing PATH (Windows), or install `g++` (Linux). |
| First build hangs at "googletest" | It is downloading. Needs internet; wait 1–2 minutes. |
| `undefined reference` after pulling new code | Delete the `build` folder and build again. |
| Typing the password shows nothing | That is intended (HMS-SEC-004). Type it and press Enter. |
