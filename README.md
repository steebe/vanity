# vanity

A CLI utility for attempting to appear to look cool.

## dependencies

- g++ 13.3.0 or newer (C++20 support required)
- CMake 3.14+
- stb_image, stb_image_write
- gtest

## dev

```bash

# build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

# test
ctest --test-dir build --output-on-failure

# install
sudo cmake --install build --prefix /usr/local

# use
vanity <command> [options]
```

<details>

<summary>nitty</summary>

### Available build types:

`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`

### Isolating targets:

```bash
cmake --build build --target vanity      # Build CLI only
cmake --build build --target libvanity   # Build library only
cmake --build build --target test_runner # Build tests only
```

### Build with multiple jobs:

```bash
cmake --build build -j$(nproc)
```

output:
- `build/bin/vanity` - Main CLI executable with subcommand architecture
- `build/lib/libvanity.a` - Static library with core image processing functions
- `build/bin/test_runner` - Test executable (if `BUILD_TESTS=ON`)

### build options

**Disable tests:**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build
```

**Custom install prefix:**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
```

## testing options

Run all tests using CTest:

```bash
ctest --test-dir build --output-on-failure
```

Run tests with verbose output:

```bash
ctest --test-dir build --verbose
```

Run specific test:

```bash
ctest --test-dir build -R test_name
```

Alternatively, run the test executable directly:

```bash
./build/bin/test_runner
```

Filter tests with Google Test arguments:

```bash
./build/bin/test_runner --gtest_filter=ImageBuffer.*
```

Install to custom location:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build
```

result:
- `<prefix>/bin/vanity` - CLI executable
- `<prefix>/lib/libvanity.a` - Static library
- `<prefix>/include/vanity/*.hpp` - Public headers


## Development

### Adding New Commands

1. Create a new command file in `src/cli/commands/`, e.g., `my_command.cpp`
2. Implement the `Command` interface (see `add_border_command.cpp` as example)
3. Add a factory function: `std::unique_ptr<Command> create_my_command()`
4. Register the command in `src/cli/commands.cpp`
5. Add the source file to `CLI_SOURCES` in `CMakeLists.txt`
6. Reconfigure and rebuild:
   ```bash
   cmake -B build
   cmake --build build
   ```

### Adding New Library Functions

1. Add function declarations to appropriate header in `include/vanity/`
2. Implement in corresponding file in `src/lib/`
3. Write tests in `tests/`
4. Add any new source files to `LIB_SOURCES` or `TEST_SOURCES` in `CMakeLists.txt`
5. Reconfigure and rebuild:
   ```bash
   cmake -B build
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```

## Dependencies

The project uses the stb libraries (header-only):
- `stb_image.h` - Image loading
- `stb_image_write.h` - Image writing

These are included in the `include/` directory and require no external dependencies.

</details>


## License Details

```
    Copyright (C) 2025  steebe (steve@stevebass.me)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

```
