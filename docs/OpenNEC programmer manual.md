OpenNEC Programmer Manual
==========================

Introduction
------------
This manual focuses on how to use the `libopennec.a` library in other programs, and the internal structures and functions that you call from the library in your programs. It also includes guidance on generating and validating NEC decks, controlling simulations, import and export of other formats, and interpreting output.

This manual is aimed at programmers intending to call OpenNEC from their own code. For those looking for instructions on how to use the program from the command line, see the main [README](../README.MD). For instructions on how to build models and decks, see the [OpenNEC modeling manual](OpenNEC%20modeling%20manual.md).

Using OpenNEC as a plug-in engine
---------------------------------
The OpenNEC command-line shell program, `onec`, has been designed to be able to work as a drop-in replacement for the original Fortran NEC-2 executables like `nec2d`, as well as programs that expect the slightly different parameter style found in the `nec2c` and `necpp` versions.

On Unix systems, `onec` matches the nec2c interface, which used the `-i` parameter to define the input filename, and `-o` for the output filename. If no `-o` is provided, it uses the input name and changes the extension to `.out`. If no paramaters are provided, the original nec2c will exit with usage notes. OpenNEC changes this only slightly, allowing you to supply the file through redirection, so you can pipe in the file(s).

OpenNEC also supports `-f`, `--format` to select the output format for the generated `.out` file. Valid values are `nec2c` for the modern Unix-style output and `original` for the legacy Fortran-style layout. The default is `nec2c` on macOS and Linux, and `original` on Windows.

You can use `--line-ending` to select the line ending style for output files. Valid values are `lf` (default on Unix/macOS) or `crlf` (default on Windows).

The Windows version supports the same switches for input and output, but changes the behaviour in the no-parameter case to match nec2d. In that case, it interactively asks for the input and output filenames, and exits if the former is blank. Most programs used redirection in this case, passing in the two filenames from a file. The key difference is that the Unix version expects a deck as the input, whereas the Windows version expects two filenames.

The exact calling proceedure varies among programs, so separate document have been created for each commonly used program. For now, these include:

- [Using OpenNEC with 4nec2](Using&20%OpenNEC&20%with&20%4nec2.md)
- [Using onec with cocoaNEC](Using&20%OpenNEC&20%with&20%cocoaNEC.md)

Using OpenNEC as a Library
--------------------------
Although `onec` provides a compatible shell interface for use with existing programs, it is mostly intended to be used as a statically-linked library within other programs.

### High-Level Architecture

The build system produces two artefacts: `libonec.a`, which contains every module except `src/main.c`, and the `onec` executable, which is formed by linking `src/main.o` against that static library. When you embed OpenNEC in your own program you link against `libonec.a` and call its functions directly — the command-line shell is only a thin driver on top of the same entry points your program will use.

All computation, deck management, and output formatting lives in the library. The `onec` executable contributes nothing that a host application cannot do itself, so there is no loss of capability when moving from the shell to the library.

### Header Files and Public API

The single header `src/opennec.h` is the recommended entry point for any program embedding OpenNEC.  It includes every sub-header in the correct order and exposes the constants, macros, and type definitions that are part of the stable public interface.

```c
#include "opennec.h"
```

The sub-headers it aggregates cover types (`types.h`), memory and logging helpers (`misc.h`), deck lifecycle (`deck.h`), file input (`input.h`), output generation (`output.h`), geometry (`geometry.h`), execution control (`control.h`), and the various physical calculation modules. Consuming programs should include only `opennec.h` rather than individual sub-headers; this ensures that any future reordering of internal dependencies remains transparent.

### Core Data Structures

**`deck_t`** holds the full set of cards read from a `.deck` or `.nec` file together with section bookmarks, `comment_start`, `geometry_start`, `geometry_end`, `deck_end`, and so on. It owns the underlying `card_t` array and all heap memory referenced from it.

**`card_t`** represents one line of the input file. It stores the two-letter card code, the raw string, the parsed numeric fields (`i[1..4]`, `f[1..7]`), any OpenNEC key-value extensions, and metadata such as whether the card is commented out or marked invisible.

**`context_t`** is an opaque handle to all internal simulation state — matrices, segment data, frequency counters, and accumulated results. Because it is forward-declared in `types.h` and only ever accessed through the API, its internal layout is not part of the public contract.

**`errors_list_t`** is a flat array of `error_t` records, each carrying a severity level and a message string. A separate `errors_list_t` is typically passed to each stage (`parse_deck`, `nec_run_simulation`, etc.) so that errors from different stages can be presented separately or merged at the caller's discretion.

### Initialization and Cleanup

`deck_t` holds the individual cards and data about the deck as a whole. It is a plain struct and is normally placed on the stack, not the heap. As such, it does not have an explicit create function, and instead has `init_deck` which ensures the structure is properly zerored out. `destroy_deck()` releases the card array and all heap memory owned by the deck (strings, extension lists, symbol table, etc.). It does not free the `deck_t` struct itself, so stack-allocated decks require no further cleanup after the call. The deck and context are independent: `destroy_deck()` must be called before the deck goes out of scope regardless of when the context is destroyed.

In contrast, `context_t` objects are normally on the heap and have functions to manage their lifecycle directly. `create_context()` allocates and zero-initialises the context. `destroy_context()` frees all memory associated with the simulation state, including any internally allocated segment and matrix buffers.

```c
context_t *ctx = create_context();
if (!ctx) { /* handle allocation failure */ }

deck_t deck;
init_deck(&deck);

/* ... run one or more simulations ... */

destroy_deck(&deck);
destroy_context(ctx);
```

To capture diagnostic messages without writing to `stderr`, register a log callback before doing any other work:

```c
void my_logger(void *user_data, int level, const char *message) {
    /* level is one of ONEC_SEV_INFO, WARNING, ERROR, FATAL */
    fprintf(logfile, "[%d] %s\n", level, message);
}

set_log_callback(ctx, my_logger, user_data);
```

### Performing a Simulation from C

The minimum sequence to load a deck, run the simulation, and write the standard NEC output file is:

```c
#include "opennec.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    context_t *ctx = create_context();

    deck_t deck;
    init_deck(&deck);

    errors_list_t errors;
    memset(&errors, 0, sizeof(errors));

    /* Read from file */
    FILE *in = fopen("example.deck", "r");
    read_deck(ctx, &deck, in);
    fclose(in);

    /* Parse fields, resolve symbols, validate structure */
    parse_deck(ctx, &deck, &errors);

    /* Run the NEC-2 engine */
    run_simulation(ctx, &deck);

    /* Write the traditional .out report */
    FILE *out = fopen("example.out", "w");
    write_nec_output(ctx, &deck, out);
    fclose(out);

    destroy_deck(&deck);
    destroy_context(ctx);
    return 0;
}
```

`parse_deck()` performs structural analysis (locating the geometry and control sections) and evaluates any symbolic expressions defined with `SY` cards. It is a prerequisite for `run_simulation()`; calling the simulation with an unparsed deck produces undefined results.

`write_nec_output()` writes the complete output report in traditional NEC format — structure specification, input card listing, network data, radiation patterns, and timing summary — matching the layout produced by `nec2c`. If you need finer control over the output you can call the component functions (`write_nec_preamble()`, `write_frequency_step_output()`, `write_end_cards()`, `write_footer()`) individually.

For programs that assemble decks programmatically rather than reading from a file, `append_card_from_text()` adds a single card line to a deck:

```c
deck_t deck;
init_deck(&deck);

append_card_from_text(&deck, "CM  Dipole at 14.2 MHz");
append_card_from_text(&deck, "CE");
append_card_from_text(&deck, "GW 1, 21, 0,0,-5.02, 0,0,5.02, 0.001");
append_card_from_text(&deck, "GE 0");
append_card_from_text(&deck, "FR 0, 1, 0, 0, 14.2");
append_card_from_text(&deck, "EX 0, 1, 11, 0, 1, 0");
append_card_from_text(&deck, "RP 0, 37, 73, 1000, 0, 0, 5, 5");
append_card_from_text(&deck, "EN");
```

To estimate whether a deck will run quickly or slowly before committing to the full calculation, call `estimate_time()` after `parse_deck()`. It returns a dimensionless complexity value, "T", that grows roughly as $O(N^3)$ for fill-and-factor and $O(N^2)$ for far-field; on a contemporary desktop machine values below roughly $10^7$ run in under a tenth of a second, and you can use this value to decide whether or not to do so automatically (for smaller values) or wait for the user to ask for the simulation to run (larger values). T values over $10^11$ tend to take several minutes to run.

Bindings and Integration Examples
----------------------------------

The following sections sketch how to call `libonec.a` from languages other than C. In each case the strategy is the same: expose the C API through the language's foreign-function interface, passing pointers to `context_t` and `deck_t` as opaque handles.

### Python (ctypes / cffi)

Python's `ctypes` module can load the dynamic version of the library (build with `make libonec.dylib` on macOS or `make libonec.so` on Linux) and call its functions directly. Declare the function signatures to match the C prototypes, pass `None` for null pointers, and use `ctypes.c_void_p` for the opaque context handle.

```python
import ctypes, os

lib = ctypes.CDLL("./libonec.so")

lib.nec_create_context.restype  = ctypes.c_void_p
lib.nec_destroy_context.argtypes = [ctypes.c_void_p]

ctx = lib.create_context()
# ... call read_deck, parse_deck, nec_run_simulation, write_nec_output ...
lib.destroy_context(ctx)
```

`cffi` is an alternative that allows you to paste the C header text directly, letting it generate the bindings automatically; this is more robust when the API has many struct types.

### Rust

From Rust, declare the foreign functions in an `extern "C"` block and wrap the raw pointer in a non-null `*mut c_void`The Rust `build.rs` script can invoke `make libonec.a` and then instruct `cargo` to link against it with `println!("cargo:rustc-link-lib=static=onec")`.

```rust
use std::ffi::c_void;

extern "C" {
    fn create_context() -> *mut c_void;
    fn destroy_context(ctx: *mut c_void);
    fn run_simulation(ctx: *mut c_void, deck: *mut c_void) -> i32;
}

fn main() {
    unsafe {
        let ctx = create_context();
        // ... populate deck, run simulation ...
        destroy_context(ctx);
    }
}
```

Wrapping the raw pointers in `struct` types backed by `PhantomData` is recommended so that the borrow checker can enforce the create-before-use and destroy-after-use invariants.

### Java (JNI)

Create a thin C shim that implements `Java_com_example_OpenNEC_*` JNI methods, translating between Java `long` handles (storing the `context_t *` as a Java `long`) and the OpenNEC API. Compile the shim as a shared library alongside `libonec.a`.

```java
public class OpenNEC {
    static { System.loadLibrary("onec_jni"); }
    public native long  createContext();
    public native void  destroyContext(long ctx);
    public native int   runSimulation(long ctx, long deck);
}
```

The corresponding C shim stores the pointer in the `jlong` and retrieves it via a cast on each subsequent call. This pattern avoids any heap allocation in the shim itself.

### Swift

Swift can call C functions directly when the library headers are exposed through a bridging header or a module map. Add `libonec.a` to the Xcode target's "Link Binary With Libraries" phase and create a bridging header that includes `opennec.h`:

```c
// OpenNEC-Bridging-Header.h
#include "opennec.h"
```

The C types are then available in Swift as their imported counterparts — `OpaquePointer` for `context_t *` and a typed `UnsafeMutablePointer<deck_t>` for the deck.

```swift
import Foundation

let ctx = create_context()
defer { destroy_context(ctx) }

var deck = deck_t()
init_deck(&deck)

// ... read_deck, parse_deck, nec_run_simulation, write_nec_output ...

destroy_deck(&deck)
```

For a cleaner API, wrap the raw calls in a Swift class that owns the context and deck, using `deinit` to ensure cleanup:

```swift
class NECSimulation {
    private let ctx: OpaquePointer
    private var deck = deck_t()

    init() {
        guard let c = create_context() else { fatalError("nec_create_context failed") }
        ctx = c
    }

    deinit {
        destroy_deck(&deck)
        destroy_context(ctx)
    }
}
```

On macOS, OpenNEC already links against the Accelerate framework, so no additional framework flags are needed in Xcode for BLAS/LAPACK.

### Objective-C

Objective-C has a direct C calling convention, so no bridging layer is needed beyond adding `libonec.a` to the link phase and importing the header:

```objc
#import "opennec.h"
```

The natural pattern is to wrap the context and deck in a class whose `-dealloc` performs cleanup:

```objc
@interface NECSimulation : NSObject {
    context_t *_ctx;
    deck_t         _deck;
}
@end

@implementation NECSimulation

- (instancetype)init {
    self = [super init];
    if (self) {
        _ctx = create_context();
        init_deck(&_deck);
    }
    return self;
}

- (void)runWithInputFile:(NSString *)path outputFile:(NSString *)outPath {
    FILE *in  = fopen(path.fileSystemRepresentation, "r");
    FILE *out = fopen(outPath.fileSystemRepresentation, "w");

    errors_list_t errors;
    memset(&errors, 0, sizeof(errors));

    read_deck(_ctx, &_deck, in);    fclose(in);
    parse_deck(_ctx, &_deck, &errors);
    run_simulation(_ctx, &_deck);
    write_nec_output(_ctx, &_deck, out); fclose(out);
}

- (void)dealloc {
    destroy_deck(&_deck);
    destroy_context(_ctx);
}

@end
```

Because Objective-C automatic reference counting (ARC) does not manage C memory, it is important that `-dealloc` always calls both `destroy_deck()` and `destroy_context()` even if the simulation never ran — `destroy_deck()` is safe to call on a zero-initialised deck.

### Other Languages

The same opaque-handle pattern applies to any language with a C FFI. Julia uses `ccall()` with `Ptr{Cvoid}` for the context; Go uses `import "C"` with cgo, noting that the `deck_t` struct fields are accessible directly if desired. In all cases, the host language manages the lifetime of the context handle while OpenNEC manages the memory it allocates internally.

Deck File Format
----------------

A NEC deck is a plain-text file containing one card per line. Each card consists of a two-letter card code followed by numeric fields or other parameters. The deck is divided into logical sections: comments, optional symbols (SY cards, not part of NEC-2), geometry definition, control cards, and terminators. For complete technical details on each card, refer to the [official NEC-2 PROGRAM INPUT documentation](https://www.nec2.org/part_3/control.html).

### Comments

OpenNEC supports multiple comment styles to annotate decks:

**Traditional NEC Comment Cards (CM/CE)**

The original NEC format uses dedicated comment cards:
- `CM` — Comment card; text following the card code is a free-form comment.
- `CE` — Comment End; marks the end of the comment section. CE cards can contain comment text, but most commonly do not so they provide whitespace.
These must appear at the start of the file, before any geometry or control cards, and at least the CE must be present.

Although NEC-2 strictly requires comments only at the top of the file, most modern implementations including OpenNEC, allow `CM` comments to be inserted anywhere in the deck. Most implementations also allow comment lines to be indicated with `!`, `'`, or `#` anywhere in the deck:

```
GE 0
! This is a full-line comment between sections
FR 0 1 0 0 14.2
```
`#` is used only by nec2c, and other engines use it to indicate AWG wire measurements. For this reason, `#` is allowed as a comment marker only for whole-line comments, not...


**Inline comments in modern NEC engine and OpenNEC**

OpenNEC supports end-of-line comments on any card using two markers:
- `!` — End-of-line comment marker (most widely used in modern software).
- `'` — Alternative comment marker (single quote).

Note that `#` cannot be used end-of-line, a `#` anywhere but the start of the line is treated as the AWG symbol.

End-of-line comments allow you to document data directly on the card, for example:

```
GW 1 21 0 0 -5.02 0 0 5.02 0.001     ! Center element (14.2 MHz dipole)
GE 0                                 ! End of geometry
FR 0 1 0 0 14.2                      ! Frequency 14.2 MHz
EX 0 1 11 0 1 0                      ! Excite at segment 11
```

When a deck is converted from OpenNEC format to standard NEC-2 format, all end-of-line and full-line comments using these markers must be removed, leaving only CM/CE blocks at the top.

### Card Organization and Reference

The following table lists all supported cards, organized by function. For detailed syntax and parameter descriptions, refer to the [NEC-2 documentation](https://www.nec2.org/part_3/toc.html). Cards marked **[NEC-4]** are part of the NEC-4 standard; those marked **[OpenNEC]** are extensions specific to OpenNEC.

#### Comment & Symbol Cards

| Card | Name | Status | Notes |
|------|------|--------|-------|
| CM   | Comment | Standard NEC-2 | Appears at start of file; marks content as documentation |
| CE   | Comment End | Standard NEC-2 | **Required**; marks end of comment block |
| SY   | Symbol (Variable) | **[OpenNEC/4nec2]** | Define variables for use in formulas elsewhere in the deck |

#### Geometry Definition Cards

| Card | Name | Status | Notes |
|------|------|--------|-------|
| GW   | Wire | Standard NEC-2 | Straight wire segment; core geometry element |
| GA   | Wire Arc | Standard NEC-2 | Circular arc approximation for curved wires |
| SP   | Surface Patch | Standard NEC-2 | Triangular surface patch; used for thin sheet modeling |
| SM   | Multiple Patches | Standard NEC-2 | Generate rectangular grid of surface patches |
| GH   | Helix/Spiral | Standard NEC-2 | Helical or spiral wire structure |
| GR   | Generate Cylindrical | Standard NEC-2 | Repeat structure in cylindrical pattern |
| GX   | Reflection | Standard NEC-2 | Reflect geometry across coordinate planes (X, Y, Z, or combinations) |
| GM   | Coordinate Transformation | Standard NEC-2 | Rotate and scale existing geometry |
| GS   | Scale Structure | Standard NEC-2 | Scale all geometry by a factor |
| GF   | Read NGF File | Standard NEC-2 | Load Numerical Green's Function data from file |
| WG   | Write NGF File | Standard NEC-2 | Write Numerical Green's Function data to file for reuse |
| GE   | Geometry End | Standard NEC-2 | **Required**; marks end of geometry section |

#### Program Control Cards

| Card | Name | Status | Notes |
|------|------|--------|-------|
| FR   | Frequency | Standard NEC-2 | Set frequency (MHz) and frequency stepping mode |
| EX   | Excitation | Standard NEC-2 | Define antenna feed point (voltage or current source) |
| LD   | Loading | Standard NEC-2 | Add series impedance (resistance, inductance, capacitance) to segments |
| TL   | Transmission Line | Standard NEC-2 | Connect two segments with a transmission line (specified impedance, length, velocity factor) |
| NT   | Networks | Standard NEC-2 | Connect segments with a network (impedance matrix representation) |
| GN   | Ground Parameters | Standard NEC-2 | Set ground parameters (Sommerfeld/Norton method, relative permittivity, conductivity) |
| GD   | Additional Ground Parameters | Standard NEC-2 | Extend GN with high-accuracy ground parameters |
| RP   | Radiation Pattern | Standard NEC-2 | Request far-field radiation pattern calculation (theta/phi range and step) |
| NE   | Near Fields | Standard NEC-2 | Request near-field data on a rectangular grid |
| NH   | Near Fields (Spherical) | Standard NEC-2 | Request near-field data on a spherical surface |
| PT   | Print Control (Current) | Standard NEC-2 | Control which current data are printed to output |
| PQ   | Print Control (Charge) | Standard NEC-2 | Control which charge data are printed to output |
| CP   | Maximum Coupling | Standard NEC-2 | Request coupling coefficient calculation and printing |
| EK   | Extended Thin-Wire Kernel | Standard NEC-2 | Enable extended thin-wire kernel for higher accuracy (use for small wire radii) |
| KH   | Interaction Approximation Range | Standard NEC-2 | Set range limits for interactions (distance-based separation approximations) |
| NX   | Next Structure | Standard NEC-2 | Finish current batch; used to reset frequency loop for a new structure or configuration |
| XQ   | Execute | Standard NEC-2 | Send simulation batch for execution; used when automatic batch-end is disabled |
| EN   | End of Run | Standard NEC-2 | **Required**; marks end of input and terminates the program |
| XT   | Exit Immediately | **[OpenNEC]** | Terminate input and skip remaining cards (for partial-deck testing) |

#### NEC-4 Extensions

The following cards are part of the NEC-4 standard and are recognized by OpenNEC:

| Card | Name | Status | Notes |
|------|------|--------|-------|
| IS   | Insulation (Series Impedance) | **[NEC-4]** | Alternative to LD for specifying distributed series resistance |

#### nec2c Extensions

The following cards are extensions introduced in nec2c and are recognized by OpenNEC:

| Card | Name | Status | Notes |
|------|------|--------|-------|
| XT   | Exizt | **[nec2c]** | Stop further processing |

The `XT` card terminates deck processing and skips all remaining cards. It is useful for partial-deck testing without having to comment out or delete cards below the test point:

```
CM  Yagi test deck
CE
GW  1  11  -1  0  -2  -1  0  2  0.001  ! reflector
GW  2  11   0  0  -2   0  0  2  0.001  ! driver
GE  0
FR  0  1  0  0  14.2
EX  0  1  6  0  1  0
XT  ! Stop here for quick testing
RP  0  37  73  1000  0  0  5  5        ! Skipped
EN  ! This is also skipped
```

Remaining cards after `XT` are not parsed, so you cannot "resume" after an `XT` card. Use `XT` only for iterative development; always remove or comment out the `XT` before the final run.

#### Minimal Deck Structure

A valid OpenNEC deck must contain, at minimum:

```
CM Example dipole
CE
GW 1 21 0 0 -5.02 0 0 5.02 0.001
GE 0
FR 0 1 0 0 14.2
EX 0 1 11 0 1 0
RP 0 37 73 1000 0 0 5 5
EN
```

The `CM`/`CE` block is technically optional (the parser will insert an empty one if absent), but is recommended for documentation. Every deck must have at least one `GW`, `GA`, `SP`, or `SM` card; a `GE` card to end geometry; at least one control card like `FR` or `RP`; and an `EN` card to terminate.

#### Field Format and Conventions

Fields on NEC cards follow a "close to free format" convention:
- Fields are separated by spaces, tabs, commas, or any combination thereof.
- Numeric fields can use fixed-point (e.g., `14.2`) or scientific notation (e.g., `1.42E+01`).

For complete syntax details, including field positions, defaults, and modal behavior, see the specific card documentation at [https://www.nec2.org/part_3/control.html](https://www.nec2.org/part_3/control.html) and related card pages.

Symbols and Formulas
--------------------

OpenNEC extends the NEC deck format with support for symbolic variables and mathematical formulas, making it easier to parameterize antenna designs and perform unit conversions. This feature is built on the `SY` (SYmbol) card, originally introduced in 4nec2, that allows you to define reusable variables and reference them throughout your deck.

### SY Cards: Defining Variables

An `SY` card defines one or more variables that can be referenced in subsequent cards. Each variable consists of a name and a formula.

**Syntax:**

```
SY  var1=formula1, var2=formula2, var3=formula3
```

**Rules:**
- Variable names must start with a letter and can contain letters, digits, and underscores: `L1`, `wire_radius`, `F_MHz`, `r_1`, etc.
- Variable names are case-insensitive; `L1` and `l1` refer to the same variable.
- Each variable is defined as a formula: a mathematical expression using constants, other variables, operators, and functions.
- Each SY card can define one or more variables. When multiple variables are defined, they must be comma-separated.
- SY cards should appear after the comment/geometry section but before the control cards, though they may be placed anywhere as long as they precede cards that reference them.
- SY cards are evaluated as they are found. This means you can define the variable `L1` to be 1 at the top of a deck, and then define it again to be 2 later in the deck.

**Example:**

```
CM Yagi antenna with parameterized dimensions
CE
!
SY  freq=14.2, lambda=300/freq, boom_len=lambda*0.45
SY  elem_spacing=lambda*0.2, wire_rad=0.005
!
GW  1 11  0  0  -5  0  0  5  wire_rad
GW  2 11  elem_spacing  0  -boom_len/2  elem_spacing  0  boom_len/2  wire_rad
GW  3 11  -elem_spacing  0  -boom_len/2  -elem_spacing  0  boom_len/2  wire_rad
GE  0
!
FR  0 1 0 0 freq
EX  0 1 6 0 1 0
RP  0 37 73 1000 0 0 5 5
EN
```

In this example, once you define `freq=14.2`, subsequent references to `freq` in formulas (like `lambda=300/freq`) are automatically substituted and evaluated.

### Operators and Functions

Formulas support standard mathematical operators and functions:

**Operators** (evaluated left-to-right, standard precedence):
- `+` — Addition
- `-` — Subtraction  
- `*` — Multiplication
- `/` — Division
- `^` — Exponentiation (e.g., `2^3 = 8`)

**Functions** (case-insensitive; argument in parentheses):
- `sin(x)`, `cos(x)`, `tan(x)` — Trigonometric functions; argument in **degrees**
- `atn(x)` — Arc tangent; returns result in **degrees**
- `sqr(x)`, `sqrt(x)` — Square root
- `exp(x)` — Exponential (e^x)
- `log(x)` — Natural logarithm (base e)
- `log10(x)` — Base-10 logarithm
- `abs(x)` — Absolute value
- `sgn(x)` — Sign function; returns -1, 0, or +1
- `int(x)` — Round to nearest integer
- `mod(x, y)` — Remainder after division (e.g., `mod(10, 3) = 1`)

**Example:**

```
SY  wave_ang = 45
SY  height = 100 * sin(wave_ang)           ! 70.7 meters
SY  radius_mm = 2.5
SY  radius_in = radius_mm / 25.4          ! Convert to inches (0.098 in)
SY  power_out = 10 ^ 3                    ! 1000 watts
```

### Unit Suffixes

OpenNEC recognizes unit suffixes that allow you to specify dimensions in different measurement systems.These suffixes are converted to OpenNEC's internal SI units (meters, Hz) for calculation.

Internally, unit suffixes are simply pre-defined symbols, for instance, `in` is 0.0254, and so `15 in` would normally expand to `15 0.0254`, an invalid formula. OpenNEC includes logic to look for these instances and re-write them into proper formulas prior to calculation, so this is internally converted to `15*0.0254`.

There is an exception: some cards will use the unit alone, with no constant, simply `in` not `1 in`. Additional logic looks for this edge case and converts this to `1*0.0254`.

**SI Prefix System:**

OpenNEC supports standard SI (metric) prefixes that can be applied to any unit. These prefixes represent powers of 10 and are especially useful for component values and impedances:

| Prefix | Factor | Example |
|--------|--------|---------|
| G (giga) | 10^9 | `2 GHz` = 2,000,000,000 Hz |
| M (mega) | 10^6 | `50 MOhm` = 50,000,000 Ohms |
| k (kilo) | 10^3 | `1 kHz` = 1,000 Hz |
| (none) | 10^0 | `50 Ohm` = 50 Ohms |
| m (milli) | 10^-3 | `100 mH` = 0.1 Henry (inductor) |
| µ (micro) | 10^-6 | `10 µF` = 0.00001 Farads |
| n (nano) | 10^-9 | `100 nH` = 0.0000001 Henries |
| p (pico) | 10^-12 | `10 pF` = 0.00000000001 Farads |

**Frequency Suffixes:**
- `Hz` — Hertz (base unit, e.g., `14200000 Hz`)
- `kHz` — Kilohertz (e.g., `14200 kHz` = 14.2 MHz)
- `MHz` — Megahertz (e.g., `14.2 MHz`)
- `GHz` — Gigahertz (e.g., `2.4 GHz`)

**Length Suffixes** (wire dimensions, coordinates):
- `ft` — Feet (e.g., `5.02 ft` = 1.53 meters)
- `in` — Inches (e.g., `0.197 in` = 5 mm)
- `mm` — Millimeters (e.g., `5 mm`)
- `cm` — Centimeters (e.g., `50 cm`)
- `m` — Meters (explicit, e.g., `1.53 m`)

**Impedance & Resistance Suffixes:**
- `Ohm` — Ohms (base unit, e.g., `50 Ohm`)
- `kOhm` — Kilohms (e.g., `10 kOhm` = 10,000 Ohms)
- `MOhm` — Megaohms (e.g., `1 MOhm` = 1,000,000 Ohms)

**Inductance Suffixes:**
- `H` — Henries (base unit, rarely used; use mH or µH instead)
- `mH` — Millihenries (e.g., `100 mH`)
- `µH` — Microhenries (e.g., `10 µH`)
- `nH` — Nanohenries (e.g., `1 nH`)

**Capacitance Suffixes:**
- `F` — Farads (base unit, rarely used; use µF, nF, or pF instead)
- `µF` — Microfarads (e.g., `100 µF`)
- `nF` — Nanofarads (e.g., `10 nF`)
- `pF` — Picofarads (e.g., `1000 pF`)

**Wire Gauge:**
- `awg` — American Wire Gauge

**Examples:**

```
SY  boom_length = 15 ft
SY  wire_diameter = 1.27 mm
SY  element_spacing = 6.5 in
SY  feed_impedance = 50 Ohm
SY  loading_L = 100 µH
SY  tuning_C = 10 pF
SY  freq_MHz = 14.2
SY  freq_Hz = freq_MHz MHz

GW  1 21  0  0  -boom_length/2  0  0  boom_length/2  wire_diameter
LD  0  1  1  6  feed_impedance  loading_L  tuning_C
FR  0  1  0  0  freq_MHz
```

**Important Note on NEC-2 Compatibility:**

When storing a deck for maximum compatibility with standard NEC-2 software, use long-form suffixes (`ft`, `in`, `MHz`, `kOhm`, `mH`, `µF`, `awg`) in saved files; these will be written as-is. Do not use the short symbols `'` (feet), `"` (inches), or `#` (AWG) in saved deck files, as these conflict with comment markers in various NEC implementations. However, you may use these symbols when *reading* a deck. Similarly, avoid Unicode characters like `µ` in files intended for exchange; use `u` (Latin u) as a fallback if necessary.

### Using Variables in Cards

Once defined, a variable can be referenced by name in any subsequent card field. The reference is syntactically indistinguishable from a numeric value — the parser simply substitutes the variable name with its computed value at parse time.

**Example:**

```
SY  freq_MHz = 14.2, wave_len = 300/freq_MHz

FR  0  1  0  0  freq_MHz
GW  1  21  0  0  -wave_len/4  0  0  wave_len/4  0.001
EX  0  1  11  0  1  0
RP  0  37  73  1000  0  0  5  5
```

When the deck is parsed:
1. `freq_MHz` is computed as `14.2`
2. `wave_len` is computed as `300 / 14.2 = 21.127 meters`
3. The FR card becomes: `FR  0  1  0  0  14.2`
4. The GW card becomes: `GW  1  21  0  0  -10.563  0  0  10.563  0.001`

### Inline Formulas and Out-of-Band Formulas

In addition to SY cards, OpenNEC allows formulas to appear *directly* in card fields:

```
GW  1  21  0  0  -300/(2*14.2)  0  0  300/(2*14.2)  0.001
```

However, inline formulas are **not** portable to standard NEC-2. To maintain compatibility, you can use **out-of-band formulas** — formulas stored in trailing comments using the OpenNEC extension syntax:

```
GW  1  21  0  0  10.563  0  0  10.563  0.001  F3=-300/(2*14.2) F4=300/(2*14.2)
```

When this deck is saved, the formula is stored in the comment section, and the numeric value remains in the field, making it fully NEC-2 compatible. When the deck is later loaded, the formula is restored from the comment.

### Common Antenna Design Patterns

**Parameterized Dipole:**

```
SY  freq=14.2, halfwave=150/freq

GW  1  21  0  0  -halfwave  0  0  halfwave  0.001
GE  0
FR  0  1  0  0  freq
EX  0  1  11  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Yagi Array with Variable Spacing:**

```
SY  freq=14.2, lambda=300/freq, boom_len=1.5*lambda
SY  spacing=0.15*lambda, reflector_len=0.52*lambda, driver_len=0.48*lambda, director_len=0.44*lambda

GW  1  11  -spacing  0  -reflector_len/2  -spacing  0  reflector_len/2  0.001  ! Reflector
GW  2  11  0  0  -driver_len/2  0  0  driver_len/2  0.001              ! Driver
GW  3  11  spacing  0  -director_len/2  spacing  0  director_len/2  0.001  ! Director
GE  0

FR  0  1  0  0  freq
EX  0  1  6  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Converting to NEC-2 Format**

To convert a deck with SY cards to pure NEC-2 format:

1. All SY cards are evaluated and removed.
2. All variable references are replaced with their numeric values.
3. Inline comments and formulas are removed.
4. The result is a standard NEC-2 deck compatible with any NEC parser.

This conversion preserves all calculation results while ensuring portability. Use the `onec` tool with output formatting options to save this converted format automatically.

OpenNEC Extensions
------------------

OpenNEC extends the standard NEC-2 format with features designed to support GUI applications, deck development workflows, and improved compatibility with other modern antenna software. These extensions are carefully designed to be invisible to standard NEC-2 parsers, allowing decks with extensions to remain compatible with legacy software.

For complete technical details on the extension system, see [OpenNEC file format § OpenNEC extensions](../doc/OpenNEC%20file%20format.md#opennec-extensions).

### Key-Value Metadata Extensions

OpenNEC allows arbitrary key-value pairs to be attached to any card using inline comment markers (`!` or `'`). These pairs are parsed separately from the card data and made available to GUI applications without affecting NEC calculations.

**Syntax:**

```
GW  1  21  0  0  -5.02  0  0  5.02  0.001  ! name=center, group=main_array, material=copper
GA  1  2  45  0,0,0  0,0,4  0.002         ! name=arc_detail, shape=circle, invisible:true
```

**Parsing Rule:** 
- Keys and values are separated by `=` or `:` (formulas typically use `=`; metadata uses `:`).
- Multiple pairs are separated by spaces, commas, or semicolons.
- Strings containing spaces must be quoted: `name='corner reflector'` or `name="16 ft boom"`.
- Unquoted text without `=` or `:` is concatenated into a comment string.

**API Access:**

From C, key-value pairs are stored in a linked list on each `card_t`:

```c
#include "opennec.h"

card_t *card = &deck->cards[i];
key_value_t *kv = card->extensns;

while (kv != NULL) {
    printf("%s = %s\n", kv->key, kv->value);
    kv = kv->next;
}
```

For Python (using `ctypes`), iterate over extensns similarly to access application-specific metadata.

### Standard Extensions for Display and Development

**Name and Group** — Organizational Metadata

- `name` — Assign a human-readable label to a single element (e.g., `name=reflector`, `name=boom`)
- `group` — Collect multiple related cards under a logical grouping (e.g., `group="upper array"`, `group="feed network"`)

These are typically used in GUI applications to populate tree views, disclosure widgets, or property panels:

```
CM  Yagi antenna
CE
!
GW  1  11  -1  0  -2  -1  0  2  0.001  ! name=reflector, group=elements, material=aluminum
GW  2  11   0  0  -2   0  0  2  0.001  ! name=driver, group=elements, material=copper
GW  3  11   1  0  -1.8 1  0  1.8  0.001  ! name=director, group=elements, material=aluminum
LD  0  2  1  6  50             ! name=feed_impedance, group=loading
```

**Ignore** — Skip During Development

The `ignore` extension allows a card to be skipped during simulation without physical removal or comment markers, making it easier to toggle elements on and off:

```
GW  1  21  0  0  -5  0  0  5  0.002      ! name=main, material=copper
GW  2  21  2  0  -5  2  0  5  0.003      ! name=experimental, ignore:true
LD  0  1  1  6  50                        ! feed impedance (active)
LD  0  2  1  6  75                        ! alternative feed (ignored)
EX  0  1  11  0  1  0
```

When parsed, cards with `ignore:true` (or `ignore=yes`, case-insensitive) have their `card->ignore` flag set. The geometry is still calculated and can be displayed in a GUI, allowing visual preview of disabled elements.

**Invisible** — Visual Display Control

The `invisible` extension hides geometry from on-screen display without removing it from calculations. This is useful for auxiliary geometry far from the main structure:

```
GW  1  11  0  0  -5  0  0  5  0.001      ! name=dipole
GW  2  5   0  10  -10  0  10  10  0.002  ! name=test_point, invisible:true
NE  1  3  10  1  0  0  0.1               ! request near-field at test point
```

**Shape** — Geometric Representation for Display

The `shape` extension allows GUI applications to render geometry more accurately than the standard wire/patch interpretation:

```
GW  1  5   0  0  -3  0  0  3   0.0025   ! name=element, shape=circle
GW  2  11  1.5 0 -4  1.5 0  4   0.02    ! name=boom, shape=square, material=aluminum
GA  1  2  45  0,0,0  0,0,2  0.005       ! name=arc_detail, shape=circle
```

Recommended shapes: `circle` (default), `square`, `rectangular`, `triangular`, and custom application-specific shapes.

**Material** — Physical Properties and Display

The `material` extension documents what the element is made from, supporting GUI color schemes and material-aware features:

```
GW  1  21  0  0  -5.02  0  0  5.02  0.001  ! material=copper
GW  2  21  1  0  -5     1  0  5     0.002  ! material=aluminum
TL  1  11  2  11  75  0.5  1              ! material:copper, comment='transmission line'
```

**Standard Materials:**
- Conducting: `copper`, `aluminum`, `silver`, `brass`, `phosphor bronze`, `steel`
- Alloys: `6061-T6`, `6063-T832`
- Other values are application-defined (e.g., `titanium`, `stainless-steel`, `carbon-fiber`)

**Comment** — Inline Documentation Boundary

The `comment` extension marks the boundary between structured key-value pairs and free-form text. This must be the last key-value pair on a line:

```
GW  1  21  0  0  -5  0  0  5  0.001  ! name=center, material=copper, comment='Main radiating element at 14.2 MHz'
```

### Out-of-Band Formulas

When a deck must remain compatible with standard NEC-2 parsers, formulas can be stored "out-of-band" in extension comments rather than inline. This preserves portability while allowing formula-driven design:

```
GW  1  21  0  0  -5.35  0  0  5.35  0.001  ! F3=-150/freq, F4=150/freq
```

The `F1` through `F7` keys correspond to numeric fields on geometry cards. During parsing, the formula is evaluated and placed into the field; during writing, the numeric value is stored in the field and the formula is re-added as an extension. This ensures:
- Other NEC software reads a pure numeric deck
- OpenNEC preserves the formula for future re-evaluation
- Changes to upstream SY variables automatically propagate when the deck is re-parsed

For complete details on formula extraction and substitution, see [Symbols and Formulas § Inline Formulas and Out-of-Band Formulas](#symbols-and-formulas).

### Extension Compatibility and Conversion

**When Writing for NEC-2 Compatibility:**
- Comment markers (`!`, `'`) and their trailing content are always acceptable to modern NEC software; use them freely
- Key-value pairs (with `=` or `:`) are parsed as part of the comment and ignored by NEC-2 parsers
- SY cards and out-of-band formulas (if present) should be removed during conversion
- Use the `onec` tool's export options to automatically clean decks for maximum portability

**When Importing from Other Formats:**
- Some NEC variants (like 4nec2) add tag numbers 9800–9899 to mark invisible geometry; OpenNEC automatically converts these to `invisible:true` annotations
- Material and shape information from MMANA-GAL, Yagi Optimizer, or cocoaNEC files may be preserved as metadata during import/export
- Extensions are preserved without modification through round-trip conversions (import → parse → export)

**Versioning and Future Extensions:**
OpenNEC extensions are versioned along with the software release. New extensions are documented in [VERSIONS](../doc/VERSIONS) with their introduction date. Applications using OpenNEC should gracefully ignore unknown extensions (treating them as comments) to ensure forward compatibility with future versions of the software.

Output Formats
--------------

OpenNEC produces output in multiple formats designed to support post-processing, visualization, compatibility with other tools, and preservation of design changes made during simulation. This section covers the primary output formats and their uses.

### Text Output (.out) Format

The standard NEC-format text output file (typically `.out`) is the primary simulation report produced by `write_nec_output()`. It contains a complete record of the simulation, including geometry, loading, currents, and radiation patterns.

**Structure:**

The `.out` file is divided into logical sections, each with tabular data and descriptive headers:

1. **File Header & Title** — Execution date, time, and problem identification
2. **Structure Specification** — Summary of geometry complexity
   - Total number of segments and patches
   - Frequency and wavelength
   - Ground parameters
3. **Segment List** — Complete table of wire segment data
   - Tag, segment number, coordinates, radius, conductor properties
4. **Patch Table** — Surface patch geometry (if present)
   - Tag, number of patches, area, normal vectors
5. **Input Cards** — Echo of the entire control section
6. **Per-Frequency Output** — Repeated for each frequency step:
   - Frequency and wavelength data
   - Network data (transmission lines, networks)
   - Loading impedances (LD card results)
   - Antenna input impedance
   - Current distribution (if PT card used)
   - Charge distribution (if PQ card used)
   - Power budget (incident, radiated, dissipated power)
7. **Radiation Patterns** — If RP card(s) present
   - Directivity, gain, polarization
   - E-field magnitudes and phases
   - Axial ratio (for elliptical polarization)
   - Theta/phi coordinates and field components
8. **Near-Field Data** — If NE/NH card(s) present
   - E-field and H-field components at grid points
   - Magnitude and phase at each point
9. **Timing Summary** — Execution time and resource usage

**Example Output Section (Radiation Pattern):**

```
---------- RADIATION PATTERNS -----------

     ANGLES        MAJOR(DB)  MINOR(DB)   TOTAL      TILT    AXIAL    MAG      PHASE      MAG      PHASE
 THETA      PHI     AXIS      AXIS        DB     DEGREES   RATIO  VOLTS/M   DEGREES   VOLTS/M   DEGREES
 DEGREES   DEGREES      
   0.00    0.00    0.00     -13.50     0.00      0.00     1.00  1.00E+00    +0.00   1.00E+00    +0.00
   5.00    0.00   -0.15     -13.45     0.00     +1.20     1.01  1.01E+00    -2.50   9.90E-01   +172.50
  10.00    0.00   -0.29     -13.31     0.00     +2.40     1.03  1.02E+00    -5.00   9.80E-01   +185.00
  ...
```

**API Access from C:**

```c
#include "opennec.h"

context_t *ctx = create_context();
deck_t deck;
init_deck(&deck);

/* ... load and parse deck ... */

FILE *output_fp = fopen("results.out", "w");
write_nec_output(ctx, &deck, output_fp);
fclose(output_fp);
```

### Field Data Output

**Radiation Pattern Data Structure**

Radiation pattern results are accumulated in `ctx->rpat` and written via the `.out` file or extracted programmatically:

```c
/* Access pattern results directly */
if (ctx->rpat.num_points > 0) {
    for (int i = 0; i < ctx->rpat.num_points; i++) {
        rpat_point_t *pt = &ctx->rpat.points[i];
        printf("Theta: %.2f°, Phi: %.2f°, Gain: %.2f dB\n",
               pt->theta, pt->phi, pt->gtot);
    }
}
```

**Pattern Output Modes** (controlled by RP card):

- **Azimuth Pattern** — Fixed elevation (zenith) angle, sweep azimuth (0°–360°)
- **Elevation Pattern** — Fixed azimuth, sweep elevation (0°–180°)
- **3D Pattern** — Full sphere: user-specified theta/phi ranges and steps (e.g., 0°–180° theta, 0°–360° phi)

Each point includes:
- Theta and phi angles (degrees)
- Gain (dB relative to isotropic or dipole)
- Directivity (dB)
- Axial ratio (elliptical polarization measure)
- E-field magnitude and phase (volts/meter, degrees)
- Theta and phi component magnitudes

**Near-Field Data Structure**

Near-field results (from NE/NH cards) are stored in `ctx->nfr`:

```c
/* Access near-field grid points */
if (ctx->nfr.num_points > 0) {
    for (int i = 0; i < ctx->nfr.num_points; i++) {
        near_field_point_t *pt = &ctx->nfr.points[i];
        printf("Position: (%.3f, %.3f, %.3f) m\n", pt->xob, pt->yob, pt->zob);
        printf("E-field: |%.3e| ∠ %.2f° V/m\n",
               cabs(pt->ex), carg(pt->ex) * 180 / M_PI);
    }
}
```

**Grid Types:**

- **Rectangular (NE)** — Uniform Cartesian grid with user-specified step sizes
- **Spherical (NH)** — Concentric spheres centered on antenna at origin

### Plot Files

When PT (print control) output is requested for plotting, OpenNEC can generate a simplified plot file for use with external visualization tools. The plot file contains only the essential gain/field data without the verbose tabular headers of the `.out` format:

```
# Theta   Phi    Gain_dB
  0.00    0.00   +5.23
  5.00    0.00   +4.87
 10.00    0.00   +4.41
 ...
```

To access plot data programmatically, use the radiation pattern structures directly rather than parsing the `.out` file.

### Numerical Green's Function (NGF) Files

OpenNEC can read and write Numerical Green's Function files (`.wgf`, `.ngf`) in the original NEC-2 binary format. These files store the unfactored CM (interaction) matrix and geometry, allowing fast reuse for frequency-sweep simulations without rebuilding the matrix:

```c
/* Write NGF file after geometry is processed */
FILE *ngf = fopen("antenna.wgf", "wb");
write_ngf_file(ctx, &deck, ngf);
fclose(ngf);

/* Later, load from NGF for fast frequency sweeps */
FILE *ngf_in = fopen("antenna.wgf", "rb");
read_ngf_file(ctx, &deck, ngf_in);
close(ngf_in);
```

**Binary Format Details:**

The NGF format is Fortran unformatted sequential, matching the NEC-2 WG (Write Green's function) card output:
- Header: version, date, frequency, wavelength
- Segment and patch data
- Unfactored CM matrix (complex double precision)
- Compatible with 4nec2, Cebik collection, and other tools


## Import/Export

OpenNEC supports exporting decks in multiple formats, allowing you to save design changes, convert between software families, or preserve your work for archival purposes.

**OpenNEC Format (.onec, .deck)**

The OpenNEC format preserves all symbols, formulas, and comments from the original deck, making it ideal for iterative design and GUI-based editing:

```c
write_deck_onec(ctx, &deck, output_fp);
```

Features:
- Retains all SY (symbol) variable definitions
- Preserves out-of-band formulas (F1, F3, F4, etc.)
- Keeps inline comments (`!`, `'`) and key-value extensions
- Evaluates formulas and replaces numeric values with results
- Enables round-trip editing: load → modify in GUI → save → re-parse

**NEC-2 Format (.nec)**

Pure NEC-2 format with maximum compatibility across tools:

```c
write_deck_nec2(&deck, output_fp);
```

Transformations:
- Removes all SY (symbol) cards
- Replaces all variable references with numeric values
- Removes inline comments and extensions
- Converts out-of-band formulas to inline values
- Strips custom OpenNEC metadata

Use NEC-2 format when exchanging decks with legacy software or ensuring portability.

**cocoaNEC Format (.nc)**

Exports to cocoaNEC scripting language, suitable for use with cocoaNEC GUI or archival:

```c
write_deck_nc(&deck, output_fp);
```

Output structure:
```nc
// Generated by OpenNEC
model ( "Yagi" )
{
	real freq, lambda ;
	element reflector, driver, director ;

	freq = 14.2 ;
	lambda = 300 / freq ;

	reflector = wire( -lambda*0.15, 0, -lambda*0.26, -lambda*0.15, 0, lambda*0.26, 0.001, 11 ) ;
	driver = wire( 0, 0, -lambda*0.24, 0, 0, lambda*0.24, 0.001, 11 ) ;
	director = wire( lambda*0.15, 0, -lambda*0.22, lambda*0.15, 0, lambda*0.22, 0.001, 11 ) ;

	voltageFeed( driver, 1.0, 0.0 ) ;

	setFrequency( freq ) ;
	azimuthPlotForElevationAngle( 0 ) ;
}
```

**MMANA-GAL Format (.maa, .mma)**

Exports to MMANA-GAL format for use with MMANA-GAL antenna modeling software:

```c
write_deck_maa(&deck, output_fp);
```

Material and geometry information is preserved through OpenNEC metadata extensions.

**Yagi Optimizer Format (.yo, .ant, .yag)**

Exports to Yagi Optimizer format, preserving element dimensions and material properties:

```c
write_deck_yo(&deck, output_fp);
```

Ideal for iterative Yagi optimization workflows.

**Convert NEC-2 deck to OpenNEC with formulas:**

```c
deck_t deck;
init_deck(&deck);
read_deck(ctx, &deck, input_fp);
parse_deck(ctx, &deck, &errors);

/* Deck is now in memory with all formulas evaluated; 
   save it back in OpenNEC format to preserve current state */
FILE *onec_out = fopen("antenna.onec", "w");
write_deck_onec(ctx, &deck, onec_out);
fclose(onec_out);
```

**Export to standard NEC-2 for legacy software:**

```c
FILE *nec2_out = fopen("antenna_compat.nec", "w");
write_deck_nec2(&deck, nec2_out);
fclose(nec2_out);
/* Result is fully compatible with any NEC-2 parser */
```

**Save NGF for frequency-sweep workflow:**

```c
/* Run initial simulation */
run_simulation(ctx, &deck);

/* Save Green's function to avoid rebuilding matrix */
FILE *wgf_file = fopen("antenna.wgf", "wb");
write_ngf_file(ctx, &deck, wgf_file);
fclose(wgf_file);

/* Later, load from NGF and sweep frequency */
deck_t deck2;
init_deck(&deck2);
read_deck(ctx, &deck2, input_fp);

FILE *wgf_in = fopen("antenna.wgf", "rb");
read_ngf_file(ctx, &deck2, wgf_in);
fclose(wgf_in);

/* Frequency sweep now reuses matrix; much faster */
```

### Supported File Formats Summary

| Format | Read | Write | Use Case |
|--------|------|-------|----------|
| `.nec` / `.deck` | ✓ | ✓ (NEC-2) | Standard NEC format; maximum compatibility |
| `.onec` | ✓ | ✓ | OpenNEC format; preserves formulas and comments |
| `.out` | — | ✓ | NEC simulation report; human-readable results |
| `.ngf` / `.wgf` | ✓ | ✓ | Numerical Green's function binary; fast frequency sweeps |
| `.nc` | ✓ | ✓ | cocoaNEC scripting language |
| `.maa` / `.mma` | ✓ | ✓ | MMANA-GAL antenna modeling |
| `.yo` / `.ant` / `.yag` | ✓ | ✓ | Yagi Optimizer |

For detailed information on specific file formats, see the format documentation in [doc/](../doc/).

Deck and card validation
------------------------

OpenNEC provides two tiers of validation: **deck-level validation**, which sweeps the entire deck looking for structural and cross-card problems, and **card-level validation**, which checks the fields of a single card independently of the rest of the deck. These tiers serve different use cases; deck validation is suited to a batch or pre-run check; card validation is designed for interactive GUI feedback as the user edits individual cards.

### Deck-Level Validation

Four functions perform deck-level validation checks. They are typically called after `parse_deck()` has been called on a successfully loaded deck.

| Function | What it checks |
|---|---|
| `test_deck_structure()` | Structural completeness: presence of required cards (CE, GE, EN), cards in the wrong section, SM without a following SC, etc. |
| `test_duplicate_tags()` | Geometry tag numbers: each tag should appear on exactly one geometry card. |
| `test_card_inputs()` | Individual card field values: segment counts, coordinate ranges, referenced tags, wire radius rules, and other cross-card rules that require knowing the full geometry. |
| `test_field_separators()` | Internal consistency of field separator style (spaces, tabs, commas) across cards in the deck. |

All four functions share the same signature and append findings to a caller-supplied `errors_list_t`:

```c
void test_deck_structure  (const context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_duplicate_tags  (const context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_card_inputs     (const context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_field_separators(const context_t *ctx, const deck_t *deck, errors_list_t *errors);
```

Each call appends to (not replaces) the list, so you can accumulate findings from all four passes in a single `errors_list_t`, or use separate lists if you want to present the stages independently.

Each entry in the error list carries a severity drawn from the `error_level` enum:

| Value | Constant | Meaning |
|---|---|---|
| 0 | `NONE` | No problem |
| 1 | `WARNING` | Suspicious; simulation will likely proceed |
| 2 | `PROBLEM` | Likely to cause incorrect results |
| 3 | `FATAL` | Deck cannot be processed |

**Example: validate a deck after loading and route findings to the log**

```c
#include "opennec.h"
#include <string.h>
#include <stdio.h>

/* Log callback defined elsewhere; receives severity + message */
static void my_logger(void *ud, int level, const char *msg)
{
    (void)ud;
    static const char *labels[] = { "INFO", "WARNING", "ERROR", "FATAL" };
    fprintf(stderr, "[%s] %s\n", labels[level < 4 ? level : 3], msg);
}

int load_and_validate(const char *path)
{
    context_t *ctx = create_context();
    set_log_callback(ctx, my_logger, NULL);

    deck_t deck;
    init_deck(&deck);

    errors_list_t parse_errors, test_errors;
    memset(&parse_errors, 0, sizeof(parse_errors));
    memset(&test_errors,  0, sizeof(test_errors));

    FILE *fp = fopen(path, "r");
    read_deck(ctx, &deck, fp);
    fclose(fp);

    parse_deck(ctx, &deck, &parse_errors);

    /* Run all four deck-level checks into a single list */
    test_deck_structure  (ctx, &deck, &test_errors);
    test_duplicate_tags  (ctx, &deck, &test_errors);
    test_card_inputs     (ctx, &deck, &test_errors);
    test_field_separators(ctx, &deck, &test_errors);

    /* Route every finding through the log callback */
    for (int i = 0; i < test_errors.num_errors; i++) {
        int sev = test_errors.errors[i].severity; /* maps to ONEC_SEV_* directly */
        report(ctx, sev, "Validation [%d]: %s", i + 1, test_errors.errors[i].message);
    }

    int fatal_count = 0;
    for (int i = 0; i < test_errors.num_errors; i++)
        if (test_errors.errors[i].severity == FATAL)
            fatal_count++;

    destroy_deck(&deck);
    destroy_context(ctx);
    return fatal_count == 0 ? 0 : -1;
}
```

The `severity` field on each `error_t` uses the same numeric values as `ONEC_SEV_INFO` / `ONEC_SEV_WARNING` / `ONEC_SEV_ERROR` / `ONEC_SEV_FATAL`, so it can be passed directly to `report()`.

Parse errors (returned in `parse_errors`) and structural test errors (returned in `test_errors`) accumulate independently. If you want to present them together, you can iterate both lists in sequence, or call `transfer_errors()` to merge one list into the other:

```c
transfer_errors(&parse_errors, &test_errors); /* appends parse_errors into test_errors */
```

### Card-Level Validation

Card-level validation checks one field at a time on a single `card_t`. It requires no `context_t` or `deck_t`, making it suitable for interactive feedback in GUI applications — it can be called on every keystroke as a user edits a field.

Two functions are provided:

```c
/* Validate a single named field; returns result by value */
field_validation_t validate_card_field(const card_t *card, const char *field_name);

/* Validate all 11 fields of a card in one call */
void validate_card_all_fields(const card_t *card, field_validation_t results[11]);
```

`validate_card_field` accepts `"I1"` through `"I4"` and `"F1"` through `"F7"` as field names and returns a `field_validation_t`:

```c
typedef struct {
    error_level severity;        /* NONE=OK, WARNING, PROBLEM, FATAL */
    char message[MAX_ERROR_LEN]; /* Human-readable description; empty when NONE */
} field_validation_t;
```

`validate_card_all_fields` fills an 11-element array in the following order:

```
results[0..3]  → I1..I4   (index = fieldN - 1)
results[4..10] → F1..F7   (index = fieldN + 3)
```

A severity of `NONE` means either the field is valid or no validation rule is defined for that field on that card type; the GUI treats both cases identically (no indicator shown).

**Example: validate all fields on a card after the user edits it**

This example sketches a GUI callback that fires whenever any field on a card changes. It calls `validate_card_all_fields`, maps each result to a colour, and updates the UI. After field-level feedback, it triggers a full deck re-check to catch cross-card issues (such as a referenced tag that no longer exists).

```c
#include "opennec.h"

/* Application-defined display helper */
extern void set_field_indicator(int field_idx, error_level severity, const char *msg);
extern void show_deck_errors(const errors_list_t *errors);

void on_card_edited(context_t *ctx, deck_t *deck, int card_index)
{
    card_t *card = &deck->cards[card_index];

    /* --- Per-field validation (no context or deck needed) --- */
    field_validation_t results[11];
    validate_card_all_fields(card, results);

    /* results[0..3] = I1..I4; results[4..10] = F1..F7 */
    for (int n = 0; n < 4; n++)
        set_field_indicator(n, results[n].severity, results[n].message);
    for (int n = 0; n < 7; n++)
        set_field_indicator(4 + n, results[4 + n].severity, results[4 + n].message);

    /* --- Incremental deck-level re-check --- */
    /* Mark the card dirty so re-parse knows to re-evaluate its formulas */
    card->edited = true;

    errors_list_t deck_errors;
    memset(&deck_errors, 0, sizeof(deck_errors));

    parse_deck(ctx, deck, &deck_errors);
    test_deck_structure(ctx, deck, &deck_errors);
    test_duplicate_tags(ctx, deck, &deck_errors);
    test_card_inputs   (ctx, deck, &deck_errors);

    show_deck_errors(&deck_errors);
}
```

**Example: validate a single field only**

When the user leaves a specific field (rather than a full card save), you can call `validate_card_field` directly for minimum overhead:

```c
void on_field_focus_lost(card_t *card, const char *field_name)
{
    field_validation_t result = validate_card_field(card, field_name);

    if (result.severity == NONE) {
        /* Clear any previous indicator for this field */
        set_field_indicator_by_name(field_name, NONE, "");
    } else {
        set_field_indicator_by_name(field_name, result.severity, result.message);

        if (result.severity == FATAL)
            show_alert("This value will prevent the deck from running: %s", result.message);
    }
}
```

### Validation Severity Summary

| Severity | Deck validation (`error_level`) | Card validation (`error_level`) | Recommended action |
|---|---|---|---|
| `NONE` = 0 | No problem found | Field is valid or no rule defined | No indicator |
| `WARNING` = 1 | Suspicious but will likely run | Value is unusual but legal | Yellow indicator; advisory |
| `PROBLEM` = 2 | Likely to produce wrong results | Value will probably cause failure | Orange indicator; block run unless overridden |
| `FATAL` = 3 | Deck cannot be processed | Value will definitely fail | Red indicator; block simulation |

Testing
-------

The OpenNEC repository contains several layers of tests, each aimed at a different concern. All tests assume that `onec` has been built in the repository root (`make`).

### Test Suites Overview

| Suite | Location | What it checks |
|---|---|---|
| Error tests | `test/error_tests/` | Engine error messages for deliberately broken decks |
| Formula tests | `test/formula_tests/` | SY symbol evaluation and unit-suffix expansion |
| Validation tests | `test/validation_tests/` | Deck-level structural warnings (via `onec -t -n`) |
| Round-trip tests | `test/roundtrip_test.c` | Read → parse → write fidelity across any collection of decks |
| Regression harness | `test/regression_tests/regression_harness.sh` | Numerical agreement across BLAS backends; timing |

### Running the Test Suite

**Error tests** — verify that specific broken decks produce the expected engine error message:

```sh
cd test/error_tests
bash run_error_tests.sh
```

Each `.deck` file in `test/error_tests/` is designed to trigger a particular error path (wrong load type, geometry below ground, missing GE card, etc.). The script runs `onec` on each file, searches its stderr output for a known regex, and reports `PASS` or `FAIL`. Files with no configured regex are reported as `SKIP`.

**Formula tests** — verify that symbol evaluation and unit conversion produce numerically identical output to the baseline:

```sh
cd test/formula_tests
bash run_formula_tests.sh
```

The script runs `examples/example5.nec` as the baseline and then runs each `*.deck` in `test/formula_tests/` (parametric variants of example5 written with SY cards, inline formulas, and unit suffixes). After normalising the `.out` files (stripping comments, timing lines, and signed-zero differences), each result is diffed against the baseline. A mismatch means a formula evaluated to a different value than the direct numeric baseline.

**Validation tests** — for each deck in `test/validation_tests/`, run `onec -t -n` (test mode, no simulation) and display the structural diagnostics:

```sh
cd test/validation_tests
bash run_tests.sh
```

The `-t` flag enables all four deck-level validation passes (`test_deck_structure`, `test_duplicate_tags`, `test_card_inputs`, `test_field_separators`). The `-n` flag suppresses the actual simulation so the check runs in near-zero time even for large decks. This is useful for interactively checking a deck before committing to a run:

```sh
./onec -t -n myantenna.deck
```

**Round-trip tests** — read a deck, parse it, write it back as `.onec` format, and check for expected content:

```sh
make roundtrip_test    # builds test/roundtrip_test binary
find test -type f \( -iname '*.nec' -o -name '*.deck' \) ! -path '*cebik*' \
  | sort | xargs ./roundtrip_test
```

The `roundtrip_test` binary writes each input file back alongside the original with a `.onec` extension (e.g. `examples/example5.nec` → `examples/example5.onec`). Examine the output with `diff` to confirm that field values, comments, formulas, and extensions round-trip cleanly. You should expect only cosmetic differences — normalised field spacing and canonical comment markers.

**Regression harness** — compare numerical output across BLAS backends and record timing:

```sh
bash test/regression_tests/regression_harness.sh
```

The harness automatically detects which backends are available (Accelerate on macOS, OpenBLAS if installed via Homebrew or pkg-config, MKL if `MKL_ROOT` is set), builds `onec` against each one in turn, runs the deck set, and diffs every pair of `.out` files. Timing is written to `test/regression_tests/timing.csv` (columns: deck, backend, real, user, sys). A human-readable summary is written to `test/regression_tests/report.txt`. A clean run shows only `OK:` lines; any `DIFF:` line indicates a numerical mismatch between backends that needs investigation.

To run the harness against a specific set of decks rather than the default collection, pass the paths as arguments:

```sh
bash test/regression_tests/regression_harness.sh examples/example5.nec test/simple_yagi.nec
```

### The `onec -t` Flag

Any deck can be validated without running a simulation using the `-t` and `-n` flags together:

```sh
./onec -t -n myantenna.deck
```

`-t` runs the four structural test passes and prints a diagnostic summary to stderr. `-n` (no-run) skips the simulation so the command returns immediately. The exit code is still 0 unless a fatal parse error occurred; check stderr for the diagnostic lines.

This is the recommended first step before submitting a long simulation: catch structural problems quickly, fix them, then run.

### Adding New Tests

**Error test** — create a `.deck` in `test/error_tests/` that deliberately triggers one specific error condition, then add a `case` entry for it in `run_error_tests.sh` with the expected regex. Keep the deck as small as possible (only enough cards to trigger the error); the minimal deck shown in the error README files is a good model.

**Formula test** — copy `examples/example5.nec` and rewrite the numeric fields using SY cards, unit suffixes, or inline formulas. The simulation result must be numerically identical to the original. Add the new deck to `test/formula_tests/` with a descriptive name; the `run_formula_tests.sh` script picks it up automatically.

**Validation test** — add a `.deck` to `test/validation_tests/` that contains a specific structural problem (or a known-good variant of one). The `run_tests.sh` script in that directory runs all `.deck` files automatically. Pair each problem deck with a corresponding `_ok` variant if you want to confirm that the correct form produces no diagnostic.

**Round-trip test** — no new test file is needed; every `.deck` or `.nec` file in the test tree is automatically covered the next time `./roundtrip_test` is run. If you add a new feature that affects how cards are serialised, run the round-trip test against decks that exercise that feature and compare the `.onec` output manually.

**Regression baseline** — after intentionally changing output format or numerical precision, re-run the regression harness and commit the updated `.diff` files in `test/regression_tests/` as the new accepted baseline.

Troubleshooting
---------------

### Deck Parsing Problems

**"The deck has no cards." / "A deck has to have at least five cards..."**

The file is empty or contains only comments. A valid deck must have at minimum: one comment block (`CM`/`CE`), at least one geometry card (`GW` etc.), a `GE` card, a frequency card (`FR`), an excitation card (`EX`), and an end card (`EN`).

**"GE on line N: no geometry cards were seen before it."**

The deck contains a `GE` card but no preceding geometry (`GW`, `GA`, `GH`, `SP`, `SM`, `GF`, etc.). Either the geometry cards are missing, or a card code was misspelled so the parser did not recognize them as geometry.

**"<CARD> on line N: appears before the GE; ... should follow geometry."**

Control cards like `FR`, `EX`, `LD`, `TL`, `GN`, `GD`, `RP`, or `EK` were found before the `GE` card that ends the geometry section. Move all control cards after `GE`.

**"Error evaluating formula '<expr>' on card N: <reason>"**

A formula in an SY card or inline formula field could not be evaluated. Common causes:
- Referencing a variable before it is defined (SY cards are evaluated top-to-bottom)
- Typo in a variable name
- Division by zero (e.g., `lambda = 300/freq` where `freq = 0`)
- Unsupported function name (check the list in the [Operators and Functions](#operators-and-functions) section)

To debug, run `./onec -t -n myantenna.deck` — it will print exactly which card and field triggered the evaluation error.

**Unexpected parse of field values / wrong numbers in output**

If numeric fields appear to be read as zero or shifted, the field separator may be ambiguous. NEC format traditionally uses fixed-width columns; OpenNEC also accepts comma, tab, and space separators. If two values are adjacent without a separator (e.g., `14.2-5.0`), OpenNEC may read this as a single value. Use commas or spaces between fields: `14.2, -5.0`.

---

### Geometry Errors

**"GW on line N (tag T): crosses the ground plane (z=0)"**

A wire passes through z=0 when a ground plane is enabled. All wire endpoints must be at z > 0 when using `GN 1`, `GN 2`, or `GN 3`. Wires below the ground plane are non-physical and cause incorrect results.

**"Wire on line N (tag T) has L/R <value> < 2; 4nec2 treats this as an error."**

The wire segment is nearly as thick as it is long (length/radius ratio < 2). NEC's thin-wire approximation breaks down for fat wires. Solutions:
- Increase segment count to make individual segments longer relative to the radius
- Use a more physically appropriate radius

**"GW on line N (tag T): has radius R which gives L/R ≤ 8"**

A wire segment's length-to-radius ratio is below 8. NEC's thin-wire approximation becomes less accurate below this ratio. Increase the number of segments (subdivision) or reduce the wire radius. This is a warning, not a fatal error; the simulation will still run.

**"Connected wires (lines X and Y, tags T1 and T2) have a radius ratio >10:1"**

Two wires that share an endpoint have very different radii. NEC's junction model expects similar radii at connection points. A ratio above 10:1 may produce inaccurate current distribution. Use a tapering geometry (GC card or intermediate GW wires) to transition between radii.

**"GW on line N (tag T): radius ≥ 0.5 × len with extended kernel"**

When the extended thin-wire kernel (`EK` card) is enabled, the wire radius must be less than half the segment length. Either increase the number of segments, reduce the radius, or disable the extended kernel if it is not needed.

**Segment below ground / wire below ground**

Any segment endpoint at z < 0 (below the ground plane) is illegal when ground is enabled:

```
error: segment 5 is below ground plane
```

Check all `GW` card end coordinates when using `GN 1`, `GN 2`, or `GN 3`. Flip the sign of negative z-coordinates or move the antenna above the ground plane.

---

### Control Card Errors

**"EX on line N: references invalid tag T, segment S"**

The excitation card references a wire tag and segment number that does not exist in the geometry. Common causes:
- Tag number in `EX` does not match any `GW` tag
- Segment index in `EX` exceeds the number of segments defined for that wire
- Geometry was modified (segments added/removed) but `EX` was not updated

**"LD on line N: references an invalid start/end segment"**

The loading card references a segment index out of range for the specified wire tag. Check that the segment numbers in `LD` are within [1, Nseg] for that tag's wire.

**"TL on line N: Z0 = 0 in F1, which is invalid."**

The transmission line characteristic impedance is zero, which is physically impossible. Set F1 to the appropriate impedance (e.g., `50` for 50 Ω coaxial cable).

**"TL on line N: connects the same tag and segment on both ends"**

A transmission line that connects a segment to itself is a no-op and usually indicates a copy-paste error. Correct the endpoint tags and segment indices.

**"GN on line N: radial wire ground screen cannot be used with Sommerfeld ground option."**

`GN 2` with `nradl > 0` (radial wire screen) is incompatible with the Sommerfeld numerical integration mode. Use `GN 2` without a radial screen, or use a different ground model.

**"FR on line N: I2 step count is > 1 but F2 (frequency step) is zero."**

A frequency sweep was requested (step count > 1) but the step size is zero, so all steps would be at the same frequency. Either set I2 to 1 (single frequency) or set F2 to a non-zero frequency step.

---

### Numerical Issues

**No output / simulation produces no results**

If `onec` exits cleanly but the output file is empty or contains no radiation pattern or impedance data, check:
1. Is there a `RP`, `NE`, or `NH` card? Without one of these, no data output is generated.
2. Is there an `EX` card with a non-zero amplitude? A zero-amplitude excitation produces zero currents.
3. Is the `GE` card present and in the correct position (after all geometry, before control cards)?

**"No convergence in shanks_integration()"**

The Sommerfeld integral did not converge during ground field computation. This can happen when:
- The wire is very close to or touching the ground plane (z ≈ 0); add clearance (≥ 0.001λ)
- Ground conductivity is extremely high (σ > 1000 S/m); for sea water use σ = 5.0
- The geometry has segments spanning very different scales

**Unrealistically high impedance or gain**

Very high or very low impedance values at the feed point often indicate a modeling issue rather than a computational error:
- **Feed point not at centre of element** — if tag/segment reference in `EX` lands on an end segment rather than the middle segment, drive-point impedance will be incorrect
- **Missing ground plane** — an antenna modeled over perfect ground may show unexpected low impedance if half-wave geometry depends on the ground image
- **Too few segments** — for a half-wave dipole, use at least 11 segments; for Yagi elements, 11–21 each

**NaN / Inf in output**

Not-a-Number or Infinity values in the output indicate a divide-by-zero or arithmetic overflow in the calculation. Typical causes:
- `LD` type 4 (impedance) with L=0 or C=0 at a frequency where the reactance goes to zero or infinity (series resonance in a loading coil/capacitor)
- A `TL` card with length = 0
- A wire segment with computed zero length (both endpoints identical)

Run `./onec -t -n myantenna.deck` first to catch structural errors before the simulation starts. Add `-v` for verbose parsing output to trace which card is being processed when the error occurs.

---

### Formula and Unit Suffix Problems

**Unit suffix is ignored / wrong value used**

Suffixes must be attached directly to the number with no space in SY card context: `14.2MHz` or as a separate argument `14.2 MHz`. In deck field context (on non-SY cards), values with suffixes must be part of a formula field (inline or out-of-band). Plain numeric fields on `GW`, `FR`, etc. are always treated as SI units (metres, Hz).

**Variable defined but not substituted**

SY variables are case-insensitive but must be referenced by exact name (allowing for case variation). Ensure the variable is defined *before* the card that uses it. Variables cannot be forward-referenced — SY evaluation is strictly top-to-bottom.

**"Error evaluating formula: unknown variable 'X'"**

The formula references a name that has not been defined in any preceding SY card. Check for typos and ensure the SY card defining the variable appears above the first card that references it.

---

### Import / Export Issues

**"'file.nec' uses the cocoaNEC format which is not yet supported for import."**

CocoaNEC's `.nec` files are XML, not NEC-2 card format. Export from cocoaNEC as a card deck (File → Export Card Deck) to get a file `onec` can process.

**"onec: '-o' cannot be used with multiple input files"**

When processing multiple input files, use a directory as the output target or let `onec` write alongside each input file. The `-o` flag only works with a single input file.

**Round-trip differences in field spacing**

When a deck is read and immediately written back, leading zeros, trailing zeros, scientific notation, and field spacing may change. This is cosmetic only and does not affect numerical results. To suppress the differences, normalise both files before diffing:

```sh
./onec input.nec -w /tmp/roundtripped.nec
diff <(cat input.nec | grep -v '^CM') <(cat /tmp/roundtripped.nec | grep -v '^CM')
```

---

### Getting More Diagnostic Information

Run `onec` with the validation flags before committing to a full simulation:

```sh
# Structural check only (no simulation):
./onec -t -n myantenna.deck

# Verbose parsing + structural check:
./onec -t -n -v myantenna.deck 2>&1 | less

# Redirect errors to a file:
./onec myantenna.deck -e errors.txt
```

The `-t` flag enables all four deck-level validation passes. The `-n` flag skips the actual simulation. The `-e` flag writes all diagnostic messages to a file instead of stderr, which is useful when the terminal truncates long output.

Appendices
----------

## Glossary of Terms

**BLAS** — Basic Linear Algebra Subroutines; a standard library for numerical linear algebra. OpenNEC supports three BLAS backends: Accelerate (macOS), OpenBLAS (Homebrew/pkg-config), and Intel MKL. The choice of BLAS backend typically does not affect numerical results but can significantly affect performance.

**Card** — A single line in a NEC deck file. Each card begins with a two-letter mnemonic (e.g., `GW`, `FR`, `EX`) followed by numeric fields or parameters. The term originates from the era when NEC input was punched onto 80-column computer cards.

**Complexity** — A dimensionless estimate of the computational work required to simulate a deck, computed by the `estimate_time()` function, and denoted as `T` in the original NEC documentation. Complexity grows roughly as O(N³) for matrix fill-and-factor and O(N²) for far-field calculations. Values below roughly 10⁷ run in under 0.1 seconds on contemporary hardware; values above 10¹¹ may take several minutes.

**Deck** — A plain-text input file containing one or more NEC cards describing an antenna and its simulation parameters. Decks typically use file extensions `.nec`, `.deck`, or `.onec` (OpenNEC format). A complete deck must include geometry, frequency, excitation, and control cards, terminated by an `EN` card.

**Green's Function** — The fundamental solution to the wave equation for electromagnetic fields in the presence of a ground plane or free space. NEC computes Green's functions via Sommerfeld integrals (for real ground) or method-of-images (for perfect ground). Numerical Green's Function (NGF) tables can be cached in files (GF/WG cards) for reuse.

**Image Method** — A technique for computing radiation in the presence of a perfect conducting ground. Uses the principle of images: the ground is approximated by a mirror image current, eliminating the need for numerical integration. Very fast but only valid for perfectly conducting surfaces.

**Kernel** — The mathematical function used to compute interactions between current-carrying segments. NEC-2 uses the thin-wire kernel by default; the `EK` card enables an extended kernel for more accurate computation of fields near small-radius wires.

**Matrix Solver** — The numerical engine that solves the system of linear equations derived from the Method of Moments. Available methods include Gaussian elimination (LU decomposition) and iterative solvers. Performance depends on matrix size (number of segments) and BLAS backend.

**Method of Moments** (MOM) — The fundamental numerical technique used by NEC. The antenna geometry is discretized into segments, and the integral equations for electromagnetic fields are converted into a matrix equation: **Z** · **I** = **V**, where **Z** is the impedance matrix, **I** is segment current, and **V** is excitation voltage. Solving for **I** gives the current distribution, from which radiation patterns and impedance are computed.

**NEC** — Numerical Electromagnetics Code; the foundational antenna simulation software originally developed at Lawrence Livermore National Laboratory in the 1970s–1980s. Multiple implementations exist: NEC-2 (original Fortran release), NEC-4 (extensions), nec2c (C port), and OpenNEC (modern C re-implementation).

**Numerical Integration** — Computational method for evaluating definite integrals with high accuracy. The Sommerfeld-Norton ground model uses numerical integration (Romberg method) to compute ground field contributions. More accurate than image method but slower.

**Segment** — A straight-line subdivision of a wire, used to discretize geometry for the Method of Moments. Each segment carries a piecewise-sinusoidal current distribution. More segments improve accuracy but increase computation time. Segment length should be roughly λ/10 to λ/20 for good results.

**Sommerfeld-Norton Method** — The standard numerical technique for computing ground effects for a lossy (real) earth. Integrates contributions from an infinite lossy half-space, accounting for ground conductivity and permittivity. More accurate than perfect ground but slower. Used when `GN 2` or `GN 3` is specified.

**SY Card** — Symbol card; defines a variable or constant for use in subsequent cards. Example: `SY freq=14.2, lambda=300/freq`. Variables are evaluated at parse time and substituted into all referenced fields. Extends the base NEC-2 format (originates in 4nec2 and nec2c variants).

**Tag** — An integer identifier (1–9999 in standard NEC) assigned to each segment or patch to facilitate referencing in cards like `EX`, `LD`, `RP`, etc. Tags are user-assigned; multiple segments can share the same tag for convenience.

**Unit Suffixes** — Shorthand notation for SI units and common engineering units. Length: `ft`, `in`, `mm`, `cm`, `m`. Frequency: `Hz`, `kHz`, `MHz`, `GHz`. Impedance: `Ohm`, `kOhm`, `MOhm`. Inductance: `H`, `mH`, `µH`, `nH`. Capacitance: `F`, `µF`, `nF`, `pF`. Wire gauge: `awg` (American Wire Gauge). Example: `14.2 MHz` instead of `14200000 Hz`.

### References and further reading

- NEC-2 official documentation (Part I and III available):
  - Part I: https://www.nec2.org/part_1/toc.html
  - Part III: https://www.nec2.org/part_3/toc.html
- ARRL antenna design books (Archive.org):
  - https://archive.org/details/arrl_antennabook
  - https://archive.org/details/arrl_antenna_compendium
- Other NEC tools and resources:
  - 4nec2 info: https://www.qsl.net/4nec2/
  - nec2c/necpp reference material: various GitHub and archive repositories
  - A. Cebik NEC insights: https://www.cebik.com/nec.html

These references provide useful historical context and extended guidance for NEC-based antenna modeling.