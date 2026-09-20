# Sommerfeld Field Function Comparison: OpenNEC vs nec2c-1.3

## Function Signatures

### OpenNEC (src/ground.c, line 214)
```c
void sommerfeld_field(context_t *restrict ctx, double t, complex double *restrict e )
```

### nec2c-1.3 (ground.c, line 209)
```c
void sflds( double t, complex double *e )
```

---

## 1. SOURCE POSITION CALCULATIONS

### OpenNEC
```c
xt= ctx->dataj.src_x+ t* ctx->dataj.src_dir_cos_x;
yt= ctx->dataj.src_y+ t* ctx->dataj.src_dir_cos_y;
zt= ctx->dataj.src_z+ t* ctx->dataj.src_dir_cos_z;
```

### nec2c-1.3
```c
xt= dataj.xj+ t* dataj.cabj;
yt= dataj.yj+ t* dataj.sabj;
zt= dataj.zj+ t* dataj.salpj;
```

**Variable Mappings:**
- `ctx->dataj.src_x` ↔ `dataj.xj`
- `ctx->dataj.src_y` ↔ `dataj.yj`
- `ctx->dataj.src_z` ↔ `dataj.zj`
- `ctx->dataj.src_dir_cos_x` ↔ `dataj.cabj` (cosine of angle, bearing)
- `ctx->dataj.src_dir_cos_y` ↔ `dataj.sabj` (sine of angle, bearing)
- `ctx->dataj.src_dir_cos_z` ↔ `dataj.salpj` (sine of angle, elevation)

---

## 2. OBSERVER POSITION & ANGLE CALCULATIONS

### OpenNEC
```c
rhx= ctx->incom.obs_x- xt;
rhy= ctx->incom.obs_y- yt;
rhs= rhx* rhx+ rhy* rhy;
rho= sqrt( rhs);

// ... angle calculation ...

cph= rhx* ctx->incom.dir_cos_x+ rhy* ctx->incom.dir_cos_y;
sph= rhy* ctx->incom.dir_cos_x- rhx* ctx->incom.dir_cos_y;
```

### nec2c-1.3
```c
rhx= incom.xo- xt;
rhy= incom.yo- yt;
rhs= rhx* rhx+ rhy* rhy;
rho= sqrt( rhs);

// ... angle calculation ...

cph= rhx* incom.xsn+ rhy* incom.ysn;
sph= rhy* incom.xsn- rhx* incom.ysn;
```

**Variable Mappings:**
- `ctx->incom.obs_x` ↔ `incom.xo`
- `ctx->incom.obs_y` ↔ `incom.yo`
- `ctx->incom.obs_z` ↔ `incom.zo`
- `ctx->incom.dir_cos_x` ↔ `incom.xsn`
- `ctx->incom.dir_cos_y` ↔ `incom.ysn`
- `ctx->incom.sin_alpha` ↔ `incom.sn` (sine of angle of elevation)

---

## 3. RANGE2 / R2 CALCULATION

Both implementations use **identical calculations**:

### OpenNEC
```c
ctx->gwav.z_img2= ctx->incom.obs_z+ zt;
zphs= ctx->gwav.z_img2* ctx->gwav.z_img2;
r2s= rhs+ zphs;
ctx->gwav.range2= sqrt( r2s);
rk= ctx->gwav.range2* TP;
ctx->gwav.cur_phase2= cmplx( cos( rk),- sin( rk));
```

### nec2c-1.3
```c
gwav.zph= incom.zo+ zt;
zphs= gwav.zph* gwav.zph;
r2s= rhs+ zphs;
gwav.r2= sqrt( r2s);
rk= gwav.r2* TP;
gwav.xx2= cmplx( cos( rk),- sin( rk));
```

**Variable Mappings:**
- `ctx->gwav.z_img2` ↔ `gwav.zph`
- `ctx->gwav.range2` ↔ `gwav.r2`
- `ctx->gwav.cur_phase2` ↔ `gwav.xx2`

**Formula (identical):**
- `z_img2 = obs_z + zt`
- `zphs = z_img2²`
- `r2s = rhs + zphs` (where rhs = rhx² + rhy²)
- `range2 = sqrt(r2s)`
- `cur_phase2 = cos(range2 * TP) - j*sin(range2 * TP)`

---

## 4. INTERPOLATION ANGLE (THETA) COMPUTATION

Both implementations use **identical calculations**:

### OpenNEC
```c
if( rho >= 1.0e-12)
    thet= atan( ctx->gwav.z_img2/ rho);
else
    thet= POT;
```

### nec2c-1.3
```c
if( rho >= 1.0e-12)
    thet= atan( gwav.zph/ rho);
else
    thet= POT;
```

**Formula (identical):**
- `theta = atan(z_img2 / rho)` when `rho >= 1.0e-12`
- `theta = π/2` (POT constant) when `rho < 1.0e-12`

---

## 5. INTERPOLATION FUNCTION CALL

### OpenNEC (line 283)
```c
interpolate_sommerfeld_grid(ctx, ctx->gwav.range2, thet, &erv, &ezv, &erh, &eph );
```

### nec2c-1.3 (line 243)
```c
intrp( gwav.r2, thet, &erv, &ezv, &erh, &eph );
```

**Difference:** The function name differs, but both take the same parameters (range and theta).

---

## 6. POST-INTERPOLATION TERM INITIALIZATION AND SCALING

### OpenNEC (line 284-285)
```c
ctx->gwav.cur_phase2= ctx->gwav.cur_phase2/ ctx->gwav.range2;
sfac= ctx->incom.sin_alpha* cph;
erh= ctx->gwav.cur_phase2*( ctx->dataj.src_dir_cos_z* erv+ sfac* erh);
ezh= ctx->gwav.cur_phase2*( ctx->dataj.src_dir_cos_z* ezv- sfac* erv);
```

### nec2c-1.3 (line 252-255)
```c
gwav.xx2= gwav.xx2/ gwav.r2;
sfac= incom.sn* cph;
erh= gwav.xx2*( dataj.salpj* erv+ sfac* erh);
ezh= gwav.xx2*( dataj.salpj* ezv- sfac* erv);
```

**Identical Calculation Pattern:**
1. Divide the phase factor by range: `cur_phase2 = cur_phase2 / range2` (or `xx2 = xx2 / r2`)
2. Compute scaling factor: `sfac = sin_alpha * cph` (or `sn * cph`)
3. Apply to erh: `cur_phase2 * (src_dir_cos_z * erv + sfac * erh)`
4. Apply to ezh: `cur_phase2 * (src_dir_cos_z * ezv - sfac * erv)`

**Variable Mappings in this section:**
- `ctx->gwav.cur_phase2` ↔ `gwav.xx2` (the divided phase factor)
- `ctx->dataj.src_dir_cos_z` ↔ `dataj.salpj`
- `ctx->incom.sin_alpha` ↔ `incom.sn`

---

## 7. REMAINING FIELD COMPONENT PROCESSING

Both implementations are **identical** after the interpolation:

### OpenNEC (lines 286-299)
```c
eph= ctx->incom.sin_alpha* sph* ctx->gwav.cur_phase2* eph;
e[0]= erh* rhx+ eph* phx;
e[1]= erh* rhy+ eph* phy;
e[2]= ezh;
/* x,y,z fields for sine current */
rk= TP* t;
sfac= sin( rk);
e[3]= e[0]* sfac;
e[4]= e[1]* sfac;
/* x,y,z fields for cosine current */
e[5]= e[2]* sfac;
sfac= cos( rk);
e[6]= e[0]* sfac;
e[7]= e[1]* sfac;
e[8]= e[2]* sfac;
```

### nec2c-1.3 (lines 256-269)
```c
eph= incom.sn* sph* gwav.xx2* eph;
e[0]= erh* rhx+ eph* phx;
e[1]= erh* rhy+ eph* phy;
e[2]= ezh;
/* x,y,z fields for sine current */
rk= TP* t;
sfac= sin( rk);
e[3]= e[0]* sfac;
e[4]= e[1]* sfac;
/* x,y,z fields for cosine current */
e[5]= e[2]* sfac;
sfac= cos( rk);
e[6]= e[0]* sfac;
e[7]= e[1]* sfac;
e[8]= e[2]* sfac;
```

---

## SUMMARY OF KEY DIFFERENCES

| Aspect | OpenNEC | nec2c-1.3 | Status |
|--------|---------|-----------|--------|
| Function name | `sommerfeld_field()` | `sflds()` | Different names |
| Context parameter | `context_t *restrict ctx` | Global structs | Structural difference |
| Range/R2 calculation | Identical formula | Identical formula | **IDENTICAL** |
| Theta calculation | Identical formula | Identical formula | **IDENTICAL** |
| Phase factor scaling | Divided by range | Divided by r2 | **IDENTICAL** |
| Interpolation function | `interpolate_sommerfeld_grid()` | `intrp()` | Different function names |
| Post-interpolation terms | `cur_phase2 * (src_dir_cos_z * erv ± sfac * erh)` | `xx2 * (salpj * erv ± sfac * erh)` | **IDENTICAL LOGIC** |
| Variable naming convention | Context-qualified `ctx->` | Global or local | Different scoping |

---

## VARIABLE NAME MAPPING TABLE

| OpenNEC | nec2c-1.3 | Meaning |
|---------|-----------|---------|
| `ctx->dataj.src_x/y/z` | `dataj.xj/yj/zj` | Source segment position |
| `ctx->dataj.src_dir_cos_x/y` | `dataj.cabj/sabj` | Direction cosines (bearing) |
| `ctx->dataj.src_dir_cos_z` | `dataj.salpj` | Direction cosine (elevation, sine) |
| `ctx->incom.obs_x/y/z` | `incom.xo/yo/zo` | Observer position |
| `ctx->incom.dir_cos_x/y` | `incom.xsn/ysn` | Observer direction (bearing) |
| `ctx->incom.sin_alpha` | `incom.sn` | Observer elevation (sine) |
| `ctx->gwav.z_img2` | `gwav.zph` | Image height (z_img + zt) |
| `ctx->gwav.range2` | `gwav.r2` | Distance to observer |
| `ctx->gwav.cur_phase2` | `gwav.xx2` | Phase factor exp(-jkr) |

---

## CONCLUSION

The mathematical implementations of `sommerfeld_field()` and `sflds()` are **functionally equivalent**:

1. **Range calculation:** Identical
2. **Theta calculation:** Identical  
3. **Phase factor initialization:** Identical
4. **Post-interpolation scaling:** Identical logic (just different variable names)
5. **Final field component calculations:** Identical

The primary differences are:
- **Structural:** OpenNEC uses a context structure, nec2c-1.3 uses global variables
- **Naming:** Variable and function names differ, but represent the same quantities
- **Interpolation function:** Different wrapper functions (`interpolate_sommerfeld_grid` vs `intrp`), but called with the same parameters
