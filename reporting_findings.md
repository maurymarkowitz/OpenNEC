# reporting.c Compilation Errors - Source Analysis

## 1. ground_params_t Structure Definition

**Location:** [src/internals.h](src/internals.h#L265)

```c
/* common  /gnd/ */
/* Formerly: Fortran /GND/ → nec2c: gnd_t */
typedef struct
{
	int
		has_ground,         /* ksymp — KSYMP: ground presence flag (1=none, 2=ground) */
		far_field_type,     /* ifar — IFAR: far-field ground interaction type */
		is_perfect,         /* iperf — IPERF: perfect ground flag */
		num_radials;        /* nradl — NRADL: number of radial wires in screen */

	double
		screen_inner_r,     /* t2 — T2: screen inner radius intermediate */
		cliff_dist,         /* cl — CL: cliff edge distance (wavelengths) */
		cliff_height,       /* ch — CH: cliff height (wavelengths) */
		screen_wire_len,    /* scrwl — SCRWL: screen wire length */
		screen_wire_radius; /* scrwr — SCRWR: screen wire radius */

	complex double
		impedance_ratio,    /* zrati — ZRATI: ground impedance ratio */
		impedance_ratio2,   /* zrati2 — ZRATI2: second medium impedance ratio */
		screen_impedance,   /* t1 — T1: wire screen impedance intermediate */
		fresnel_ratio;      /* frati — FRATI: Fresnel reflection boundary param */

} ground_params_t;
```

**Key Fields:**
- **Flags**: `has_ground` (ksymp), `is_perfect` (iperf), `num_radials` (nradl), `far_field_type` (ifar)
- **Ground Parameters**: `impedance_ratio` (zrati), `impedance_ratio2` (zrati2)
- **Screen Parameters**: `screen_wire_len`, `screen_wire_radius`, `screen_impedance`
- **Cliff Parameters**: `cliff_dist` (cl), `cliff_height` (ch)

**Accessed in reporting.c** (lines 460-476):
```c
ctx->gnd.iperf = iperf;
ctx->gnd.nradl = card->i[2];
ctx->gnd.ksymp = 2;
ctx->gnd.epsr = card->f[1];
ctx->gnd.sig = card->f[2];
ctx->gnd.scrwlt = card->f[3];
ctx->gnd.scrwrt = card->f[4];
ctx->gnd.epsr2 = card->f[3];
ctx->gnd.sig2 = card->f[4];
ctx->gnd.clt = card->f[5];
ctx->gnd.cht = card->f[6];
```

---

## 2. Function Definitions and Usage

### reset_loading_buffers()

**Signature:**
```c
static void reset_loading_buffers(context_t *ctx)
```

**Defined in:** [src/control.c](src/control.c#L700)
**Used in:** [src/control.c](src/control.c#L561), [src/reporting.c](src/reporting.c#L402)

**Implementation** (control.c lines 700-718):
```c
static void reset_loading_buffers(context_t *ctx)
{
    if (ctx->zload.num_loads > 0) {
        mem_free(ctx, (void **)&ctx->zload.load_types);
        mem_free(ctx, (void **)&ctx->zload.load_tags);
        mem_free(ctx, (void **)&ctx->zload.load_tag_from);
        mem_free(ctx, (void **)&ctx->zload.load_tag_to);
        mem_free(ctx, (void **)&ctx->zload.ldcard_num);
        mem_free(ctx, (void **)&ctx->zload.load_r);
        mem_free(ctx, (void **)&ctx->zload.load_l);
        mem_free(ctx, (void **)&ctx->zload.load_c);
        mem_free(ctx, (void **)&ctx->zload.load_freq);
        ctx->zload.num_loads = 0;
    }
    if (ctx->loading_outputs.entries != NULL) {
        mem_free(ctx, (void **)&ctx->loading_outputs.entries);
        ctx->loading_outputs.count = 0;
        ctx->loading_outputs.capacity = 0;
    }
}
```

---

### reset_vsorc_buffers()

**Signature:**
```c
static void reset_vsorc_buffers(context_t *ctx)
```

**Defined in:** [src/control.c](src/control.c#L766)
**Used in:** [src/control.c](src/control.c#L564), [src/reporting.c](src/reporting.c#L495)

**Implementation** (control.c lines 766-781):
```c
static void reset_vsorc_buffers(context_t *ctx)
{
    if (ctx->vsorc.num_vsrcs > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.vsrc_segs);
        mem_free(ctx, (void **)&ctx->vsorc.vsrc_voltages);
        ctx->vsorc.num_vsrcs = 0;
    }
    if (ctx->vsorc.num_qdsrcs > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_segs);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_indices);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_voltages);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_voltages_saved);
        ctx->vsorc.num_qdsrcs = 0;
        ctx->vsorc.num_qdsrcs_used = 0;
    }
}
```

---

## 3. Missing Functions (Compilation Errors)

### add_loading()

**Called in:** [src/reporting.c](src/reporting.c#L434)
**Status:** ❌ **NOT DEFINED** - This function is missing from the codebase

**Call site** (reporting.c lines 433-434):
```c
/* Add loading to context - use existing add_loading or queue system */
int result = add_loading(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc);
```

**Expected signature:**
```c
int add_loading(context_t *ctx, int ldtyp, int ldtag, int ldtagf, int ldtagt, 
                double zlr, double zli, double zlc);
```

**Related function:** `add_loading_output()` exists in [src/calculations.c](src/calculations.c#L23) but serves a different purpose (output reporting, not buffer management).

---

### add_voltage_source()

**Called in:** [src/reporting.c](src/reporting.c#L520)
**Status:** ❌ **NOT DEFINED** - This function is missing from the codebase

**Call site** (reporting.c lines 519-520):
```c
if (extype == 0) {
    return add_voltage_source(ctx, tag, seg, voltage);
```

**Expected signature:**
```c
int add_voltage_source(context_t *ctx, int tag, int seg, complex double voltage);
```

---

### add_current_source()

**Called in:** [src/reporting.c](src/reporting.c#L521)
**Status:** ⚠️ **PARTIALLY DEFINED** - `inject_current_source()` exists but has a different signature

**Call site** (reporting.c line 521):
```c
} else {
    return add_current_source(ctx, tag, seg, voltage);
}
```

**Actual implementation:** [src/control.c](src/control.c#L835) has `inject_current_source()`:
```c
static int inject_current_source(context_t *ctx, int card_idx,
                                  int tag, int seg_idx,
                                  complex double I_desired)
```

**Difference:** The existing function takes `card_idx` as a parameter, while reporting.c is calling with just `(ctx, tag, seg, voltage)`.

---

## Summary of Issues

| Function | Status | File | Line | Issue |
|----------|--------|------|------|-------|
| `reset_loading_buffers()` | ✓ Defined | control.c | 700 | Works - declared static |
| `reset_vsorc_buffers()` | ✓ Defined | control.c | 766 | Works - declared static |
| `add_loading()` | ✗ Missing | — | — | **Not implemented** |
| `add_voltage_source()` | ✗ Missing | — | — | **Not implemented** |
| `add_current_source()` | ✗ Mismatch | control.c | 835 | Named `inject_current_source()` with different signature |

---

## Context Structure Fields Used in reporting.c

From GN card processing, reporting.c tries to set these fields on `ctx->gnd`:
- `ctx->gnd.iperf` ✓ (exists)
- `ctx->gnd.nradl` ✓ (exists as `num_radials`)
- `ctx->gnd.ksymp` ✓ (exists as `has_ground`)
- `ctx->gnd.epsr` ✗ **NOT FOUND** in ground_params_t
- `ctx->gnd.sig` ✗ **NOT FOUND** in ground_params_t
- `ctx->gnd.scrwlt` ✗ **NOT FOUND** - should be `screen_wire_len`
- `ctx->gnd.scrwrt` ✗ **NOT FOUND** - should be `screen_wire_radius`
- `ctx->gnd.epsr2` ✗ **NOT FOUND**
- `ctx->gnd.sig2` ✗ **NOT FOUND**
- `ctx->gnd.clt` ✗ **NOT FOUND** - should be `cliff_dist`
- `ctx->gnd.cht` ✗ **NOT FOUND** - should be `cliff_height`

**These field name mismatches are the root cause of the compilation errors!**
