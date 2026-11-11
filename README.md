# Pocket Ledger

A small C++17 command-line income and expense tracker. It stores entries in a local CSV file and uses integer paise internally, so money totals do not depend on floating-point arithmetic.

## Features

- Add income and expenses with a date, category, description, and amount.
- List entries and view monthly or all-time totals.
- Delete an entry by its ID.
- Persist data to CSV, including correct escaping for commas and quotes.
- Validate dates (including leap years) and amounts to two decimal places.
- Warn and skip malformed CSV rows instead of crashing on the first bad record.
- Build with CMake and run a dependency-free C++ test executable.

## Build and run

Requirements: a C++17 compiler and CMake 3.16 or newer.

```bash
cmake -S . -B build
cmake --build build
./build/pocket-ledger
```

On Windows with Visual Studio's generator, run `build\Debug\pocket-ledger.exe` or `build\Release\pocket-ledger.exe` depending on the selected configuration.

To run tests:

```bash
ctest --test-dir build --output-on-failure
```

The app creates `ledger.csv` in the current directory. Set `POCKET_LEDGER_FILE` to use a different path. To try the sample, copy `sample/ledger.example.csv` to `ledger.csv`.

## Data format

Amounts are stored as integer paise. For example, `125.50` rupees is saved as `12550`. The CSV contains only fictional sample data. Keep your own financial records private; this program is a learning exercise, not accounting software.

## Learning notes

This project practices structs, vectors, functions, input validation, file I/O, CSV parsing, and tests. To explore the build-break-rebuild process, try malformed CSV rows, quoted commas, invalid dates, repeated deletion, and interrupted writes. Add a test before changing behavior.

## License

MIT.
