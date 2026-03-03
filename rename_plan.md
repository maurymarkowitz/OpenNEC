# OpenNEC Legacy Name Rename Plan

This document catalogs all remaining Fortran/C-legacy identifiers in OpenNEC — struct type
names, struct field names, and function names — traces each back to its original Fortran
COMMON block or subroutine name and its intermediate nec2c C name, and proposes a modern
replacement. A "necpp C++ name" column shows what the independent necpp C++ port chose for
the same construct, as an additional reference point.

**necpp source**: `/Volumes/Bigger/Users/maury/Developer/necpp-master/nec2cpp/`

**Comment format used in code after renaming:**
- Struct typedef: `/* Formerly: Fortran /FPAT/ → nec2c: fpat_t */`
- Field: trailing inline comment `/* nth — Fortran NTH */`
- Function: leading comment block noting Fortran and nec2c names

---

## 1. Struct Type Renames

necpp merged most small Fortran COMMON blocks into a single `nec_context` class.
Where necpp made a distinct class, it is shown; otherwise "→ `nec_context`" means the
fields were dissolved directly into the monolithic context.

| Current OpenNEC Type | Fortran COMMON | nec2c Type | necpp C++ Class | Proposed Modern Name | Notes |
|---|---|---|---|---|---|
| `geometry_t` | `/DATA/` | `data_t` | `c_geometry` | ✅ keep `geometry_t` | Already renamed |
| `crnt_t` | `/CRNT/` | `crnt_t` | → `nec_context` | `current_t` | Abbreviation of "current" |
| `dataj_t` | `/DATAJ/` | `dataj_t` | → `nec_context` | `segment_t` | "/DATAJ/" = "data for J-th segment" |
| `fpat_t` | `/FPAT/` | `fpat_t` | `nec_radiation_pattern` | `field_pattern_t` | "FPAT" = field pattern |
| `ggrid_t` | `/GGRID/` | `ggrid_t` | `c_ggrid` | `green_grid_t` | Green's function interpolation grid |
| `gnd_t` | `/GND/` | `gnd_t` | `nec_ground` | `ground_params_t` | |
| `gwav_t` | `/GWAV/` | `gwav_t` | `c_ground_wave` | `ground_wave_t` | Norton ground wave parameters |
| `incom_t` | `/INCOM/` | `incom_t` | → `nec_context` | `green_params_t` | Common params for Sommerfeld integration |
| `matpar_t` | `/MATPAR/` | `matpar_t` | → `nec_context` | `matrix_params_t` | |
| `netcx_t` | `/NETCX/` | `netcx_t` | → `nec_context` | `network_context_t` | |
| `plot_t` | `/PLOT/` | `plot_t` | `c_plot_card` | `plot_params_t` | |
| `save_t` | `/SAVE/` | `save_t` | → `nec_context` | `run_params_t` | Holds frequency sweep and ground params |
| `segj_t` | `/SEGJ/` | `segj_t` | → `c_geometry` | `segment_junction_t` | Basis function junction traversal |
| `smat_t` | `/SMAT/` | `smat_t` | → `nec_context` | `symmetry_matrix_t` | Mode transformation matrix |
| `tmi_t` | `/TMI/` | `tmi_t` | → `nec_context` | `wire_e_integration_t` | Used by `intx`/`gf` for wire E-field |
| `tmh_t` | *(local in HFK)* | `tmh_t` | → `nec_context` | `wire_h_integration_t` | Used by `hfk`/`gh` for wire H-field |
| `vsorc_t` | `/VSORC/` | `vsorc_t` | → `nec_context` | `voltage_sources_t` | |
| `yparm_t` | `/YPARM/` | `yparm_t` | → `nec_context` | `coupling_params_t` | Y-parameter coupling |
| `zload_t` | `/ZLOAD/` | `zload_t` | → `nec_context` | `impedance_loading_t` | |

---

## 2. `geometry_t` Field Renames
*(Fortran COMMON `/DATA/`, nec2c `data_t`, necpp `c_geometry`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `n` | `N` | total wire segment count | `n_segments` | `num_segs` |
| `np` | `NP` | segments in symmetry cell | `np` | `num_segs_sym` |
| `m` | `M` | surface patch count | `m` | `num_patches` |
| `mp` | `MP` | patches in symmetry cell | `mp` | `num_patches_sym` |
| `npm` | *(derived)* | N+M | `n_plus_2m` *(n+2m actually)* | `num_segs_and_patches` |
| `np2m` | *(derived)* | N+2M | `n_plus_2m` | `num_segs_2xpatches` |
| `np3m` | *(derived)* | N+3M | `n_plus_3m` | `num_segs_3xpatches` |
| `ipsym` | `IPSYM` | symmetry flag (0/1/2/3/negative) | `m_ipsym` | `symmetry_flag` |
| `*icon1` | `ICON1` | segment end-1 connection index | `icon1` | `*seg_end1_conn` |
| `*icon2` | `ICON2` | segment end-2 connection index | `icon2` | `*seg_end2_conn` |
| `*tag_nums` | `ITAG` | segment tag numbers | `segment_tags` | ✅ keep `*tag_nums` |
| `*card_nums` | *(OpenNEC addition)* | source card line numbers | *(absent)* | ✅ keep `*card_nums` |
| `*x1,*y1,*z1` | *(stored in X/Y/Z pre-CABC)* | seg end-1 coords (meters) | *(transient, not stored)* | `*end1_x, *end1_y, *end1_z` |
| `*x2,*y2,*z2` | `SI,ALP,BET` *(EQUIVALENCEd pre-CABC)* | seg end-2 coords (meters) | `x2, y2, z2` | `*end2_x, *end2_y, *end2_z` |
| `*x,*y,*z` | `X,Y,Z` | seg center coords (wavelengths) | `x, y, z` | `*x_center, *y_center, *z_center` |
| `*si` | `SI` | segment half-length (wavelengths) | `segment_length` | `*half_len` |
| `*bi` | `BI` | segment radius (wavelengths) | `segment_radius` | `*radius` |
| `*cab` | `ALP` *(EQUIVALENCEd as CAB)* | cos(α)·cos(β) direction cosine | `cab` | `*dir_cos_x` |
| `*sab` | `BET` *(EQUIVALENCEd as SAB)* | cos(α)·sin(β) direction cosine | `sab` | `*dir_cos_y` |
| `*salp` | `SALP` *(from /ANGL/)* | sin(α) direction cosine | `salp` | `*dir_cos_z` |
| `wlam` | `WLAM` | wavelength in meters | `_wavelength` *(in nec_context)* | `wavelength` |
| `*px,*py,*pz` | *(patch center)* | patch center coords | `px, py, pz` | `*patch_x_center, *patch_y_center, *patch_z_center` |
| `*t1x,*t1y,*t1z` | `SI,ALP,BET` *(EQUIVALENCEd for patches)* | patch tangent vector T1 | `t1x, t1y, t1z` | `*patch_t1x, *patch_t1y, *patch_t1z` |
| `*t2x,*t2y,*t2z` | `ICON1,ICON2,ITAG` *(EQUIVALENCEd for patches)* | patch tangent vector T2 | `t2x, t2y, t2z` | `*patch_t2x, *patch_t2y, *patch_t2z` |
| `*pbi` | `BI` *(for patches)* | patch area (wavelengths²) | `pbi` | `*patch_area` |
| `*psalp` | `SALP` *(for patches)* | patch normal direction cosine | `psalp` | `*patch_normal_z` |
| `*jco` | `JCO` *(from /SEGJ/)* | connection segment index array | `jco` | `*junction_segs` |
| `jsno` | `JSNO` *(from /SEGJ/)* | number of junction entries | `jsno` | `num_junction_segs` |
| `maxcon` | *(OpenNEC addition)* | allocated connection capacity | `maxcon` | `max_connections` |
| `*ax` | `AX` *(from /SEGJ/)* | constant-current basis coefficients | `ax` | `*coeff_const` |
| `*bx` | `BX` *(from /SEGJ/)* | sine-current basis coefficients | `bx` | `*coeff_sine` |
| `*cx` | `CX` *(from /SEGJ/)* | cosine-current basis coefficients | `cx` | `*coeff_cos` |

---

## 3. `crnt_t` Field Renames
*(Fortran COMMON `/CRNT/`, nec2c `crnt_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `*air` | `AIR` | A-current, real part | `air` | `*a_real` |
| `*aii` | `AII` | A-current, imaginary part | `aii` | `*a_imag` |
| `*bir` | `BIR` | B-current, real part | `bir` | `*b_real` |
| `*bii` | `BII` | B-current, imaginary part | `bii` | `*b_imag` |
| `*cir` | `CIR` | C-current, real part | `cir` | `*c_real` |
| `*cii` | `CII` | C-current, imaginary part | `cii` | `*c_imag` |
| `*cur` | `CUR` | surface current (complex, 3 components per patch) | `current_vector` *(combined)* | `*surface_cur` |

---

## 4. `dataj_t` Field Renames
*(Fortran COMMON `/DATAJ/`, nec2c `dataj_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `iexk` | `IEXK` | extended thin-wire kernel flag | `m_use_exk` | `use_extended_kernel` |
| `ind1` | `IND1` | end-1 kernel indicator (0/1/2) | `ind1` | `end1_kernel_type` |
| `indd1` | `INDD1` | end-1 deferred kernel indicator | `indd1` | `end1_kernel_deferred` |
| `ind2` | `IND2` | end-2 kernel indicator | `ind2` | `end2_kernel_type` |
| `indd2` | `INDD2` | end-2 deferred kernel indicator | `indd2` | `end2_kernel_deferred` |
| `ipgnd` | `IPGND` | ground image loop pass (1 or 2) | *(not in header)* | `ground_image_pass` |
| `s` | `S` | source segment half-length | `m_s` | `seg_half_len` |
| `b` | `B` *(also T2XJ via EQUIVALENCE)* | source segment radius / patch T2X | `m_b` | `seg_radius` |
| `xj,yj,zj` | `XJ,YJ,ZJ` | source segment center coords | `xj, yj, zj` | `src_x, src_y, src_z` |
| `cabj` | `CABJ` *(also T1XJ)* | cos(α)cos(β) for source segment | `cabj` | `src_dir_cos_x` |
| `sabj` | `SABJ` *(also T1YJ)* | cos(α)sin(β) for source segment | `sabj` | `src_dir_cos_y` |
| `salpj` | `SALPJ` *(also T1ZJ)* | sin(α) for source segment | `salpj` | `src_dir_cos_z` |
| `rkh` | `RKH` | k × half-length | `rkh` | `k_half_len` |
| `t1xj,t1yj,t1zj` | *EQUIVALENCEd to CABJ,SABJ,SALPJ* | patch T1 direction (when source is patch) | `t1xj, t1yj, t1zj` | `patch_t1x, patch_t1y, patch_t1z` |
| `t2xj,t2yj,t2zj` | *EQUIVALENCEd to B,IND1,IND2* | patch T2 direction (when source is patch) | `t2xj, t2yj, t2zj` | `patch_t2x, patch_t2y, patch_t2z` |
| `exk,eyk,ezk` | `EXK,EYK,EZK` | E-field contribution (constant current) | `exk, eyk, ezk` | `e_const_x, e_const_y, e_const_z` |
| `exs,eys,ezs` | `EXS,EYS,EZS` | E-field contribution (sine current) | `exs, eys, ezs` | `e_sin_x, e_sin_y, e_sin_z` |
| `exc,eyc,ezc` | `EXC,EYC,EZC` | E-field contribution (cosine current) | `exc, eyc, ezc` | `e_cos_x, e_cos_y, e_cos_z` |

---

## 5. `fpat_t` Field Renames
*(Fortran COMMON `/FPAT/`, nec2c `fpat_t`, necpp `nec_radiation_pattern` + `nec_context`)*

necpp split this struct: per-run parameters stay in `nec_context`; per-result data goes into
`nec_radiation_pattern`. The necpp name column shows where each field landed.

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `near` | `NEAR` | near-field output flag | `m_near` *(context)* | `is_near_field` |
| `nfeh` | `NFEH` | near-field type selector (0=E, 1=H) | `nfeh` *(context)* | `near_field_type` |
| `nrx,nry,nrz` | `NRX,NRY,NRZ` | near-field grid dimensions | `nrx, nry, nrz` *(context)* | `grid_nx, grid_ny, grid_nz` |
| `nth,nph` | `NTH,NPH` | number of theta/phi pattern angles | `n_theta, n_phi` *(pattern)* | `num_theta, num_phi` |
| `ipd` | `IPD` | power/directive gain selector | `m_rp_ipd` *(pattern)* | `gain_type` |
| `iavp` | `IAVP` | average power integration flag | `m_rp_power_average` *(pattern)* | `avg_power_flag` |
| `inor` | `INOR` | normalized gain output flag | `m_rp_normalization` *(pattern)* | `normalize_gain` |
| `iax` | `IAX` | polarization axis selector | `m_rp_output_format` *(pattern)* | `pol_axis` |
| `ixtyp` | `IXTYP` | excitation type | `m_excitation_type` *(context)* | `excitation_type` |
| `thets,phis` | `THETS,PHIS` | starting theta, phi angles (degrees) | `m_theta_start, m_phi_start` *(pattern)* | `theta_start, phi_start` |
| `dth,dph` | `DTH,DPH` | theta, phi step sizes (degrees) | `delta_theta, delta_phi` *(pattern)* | `theta_step, phi_step` |
| `rfld` | `RFLD` | range to field point (near field) | `m_range` *(pattern)* | `range` |
| `gnor` | `GNOR` | normalization gain value | `m_rp_gnor` *(context)* | `norm_gain` |
| `clt,cht` | `CLT,CHT` | cliff edge distance, height | `cliff_edge_distance, cliff_height` *(nec_ground)* | `cliff_dist, cliff_height` |
| `epsr2,sig2` | `EPSR2,SIG2` | second ground medium ε_r and σ | `epsr2, sig2` *(nec_ground)* | `epsr2, sigma2` |
| `xpr6` | `XPR6` | excitation parameter 6 | `xpr6` *(context)* | `exc_param6` |
| `pinr` | `PINR` | input power (watts) | `_pinr` *(pattern)* | `power_in` |
| `pnlr` | `PNLR` | network power loss (watts) | `_pnlr` *(pattern)* | `network_loss` |
| `ploss` | `PLOSS` | ohmic loss (watts) | `structure_power_loss` *(context)* | `ohmic_loss` |
| `xnr,ynr,znr` | `XNR,YNR,ZNR` | near-field grid origin coords | `xnr, ynr, znr` *(context)* | `grid_x0, grid_y0, grid_z0` |
| `dxnr,dynr,dznr` | `DXNR,DYNR,DZNR` | near-field grid spacing | `dxnr, dynr, dznr` *(context)* | `grid_dx, grid_dy, grid_dz` |

---

## 6. `gnd_t` Field Renames
*(Fortran COMMON `/GND/`, nec2c `gnd_t`, necpp `nec_ground`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `ksymp` | `KSYMP` | ground presence flag (1=none, 2=ground) | `ksymp` | `has_ground` |
| `ifar` | `IFAR` | far-field ground interaction type | `ifar` *(context)* | `far_field_type` |
| `iperf` | `IPERF` | perfect ground flag | `iperf` *(private)* | `is_perfect` |
| `nradl` | `NRADL` | number of radial wires in ground screen | `radial_wire_count` | `num_radials` |
| `t2` | `T2` | screen inner radius intermediate | `t2` | `screen_inner_r` |
| `cl` | `CL` | cliff edge distance (wavelengths) | `cliff_edge_distance` *(private, via get_cl)* | `cliff_dist` |
| `ch` | `CH` | cliff height (wavelengths) | `cliff_height` *(private, via get_ch)* | `cliff_height` |
| `scrwl` | `SCRWL` | screen wire length (wavelengths) | `scrwl` | `screen_wire_len` |
| `scrwr` | `SCRWR` | screen wire radius (wavelengths) | `scrwr` | `screen_wire_radius` |
| `zrati` | `ZRATI` | ground impedance ratio η₀/η | `zrati` | `impedance_ratio` |
| `zrati2` | `ZRATI2` | second medium impedance ratio | *(via `get_zrati2()`)* | `impedance_ratio2` |
| `t1` | `T1` | wire screen impedance intermediate | `m_t1` | `screen_impedance` |
| `frati` | `FRATI` | Fresnel reflection boundary parameter | `frati` | `fresnel_ratio` |

---

## 7. `netcx_t` Field Renames
*(Fortran COMMON `/NETCX/`, nec2c `netcx_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `masym` | `MASYM` | matrix asymmetry check flag | `masym` | `check_asymmetry` |
| `neq` | `NEQ` | total equations (matrix size) | `neq` | `num_eq` |
| `npeq` | `NPEQ` | equations per symmetry section | `npeq` | `num_eq_sym` |
| `neq2` | `NEQ2` | NGF new-structure unknowns | `neq2` | `num_eq_ngf` |
| `nonet` | `NONET` | number of two-port network connections | `network_count` | `num_networks` |
| `ntsol` | `NTSOL` | network solution type | `ntsol` | `network_type` |
| `nprint` | `NPRINT` | network data print flag | `nprint` | `print_net_data` |
| `*iseg1` | `ISEG1` | network port-1 segment numbers | `iseg1` | `*net_seg1` |
| `*iseg2` | `ISEG2` | network port-2 segment numbers | `iseg2` | `*net_seg2` |
| `*ntyp` | `NTYP` | network type codes | `ntyp` | `*net_types` |
| `*x11r,*x11i` | `X11R,X11I` | two-port admittance Y11 real/imag | `x11r, x11i` | `*y11_real, *y11_imag` |
| `*x12r,*x12i` | `X12R,X12I` | two-port admittance Y12 real/imag | `x12r, x12i` | `*y12_real, *y12_imag` |
| `*x22r,*x22i` | `X22R,X22I` | two-port admittance Y22 real/imag | `x22r, x22i` | `*y22_real, *y22_imag` |
| `pin` | `PIN` | total input power (watts) | `input_power` | `power_in` |
| `pnls` | `PNLS` | power lost in networks (watts) | `network_power_loss` | `power_net_loss` |
| `asmx` | *(OpenNEC addition)* | maximum relative matrix asymmetry | *(absent)* | `max_asymmetry` |
| `asa` | *(OpenNEC addition)* | RMS relative matrix asymmetry | *(absent)* | `rms_asymmetry` |
| `zped` | `ZPED` | network input impedance | `zped` | `input_impedance` |

---

## 8. `save_t` Field Renames
*(Fortran COMMON `/SAVE/`, nec2c `save_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `*ip` | `IP` | LU factorization pivot indices | `ip` | `*pivot` |
| `nfrq` | *(OpenNEC addition)* | number of frequency steps | `nfrq` | `num_freq` |
| `ifrq` | *(OpenNEC addition)* | frequency step type (0=linear, 1=mult) | `ifrq` | `freq_step_type` |
| `epsr` | `EPSR` | ground relative dielectric constant | `epsr` *(nec_ground private)* | `ground_epsr` |
| `sig` | `SIG` | ground conductivity (S/m) | `sig` *(nec_ground private)* | `ground_sigma` |
| `scrwlt` | `SCRWLT` | screen radial wire length (meters) | `radial_wire_length` *(nec_ground)* | `screen_wire_len` |
| `scrwrt` | `SCRWRT` | screen radial wire radius (meters) | `radial_wire_radius` *(nec_ground)* | `screen_wire_radius` |
| `fmhz` | `FMHZ` | current frequency (MHz) | `freq_mhz` | `freq_mhz` |
| `delfrq` | *(OpenNEC addition)* | frequency step size | `delfrq` | `freq_step` |

---

## 9. `segj_t` Field Renames
*(Fortran COMMON `/SEGJ/`, nec2c `segj_t`, necpp → folded into `c_geometry`)*

These fields are already part of `geometry_t` in OpenNEC. They are listed here separately
for historical reference. See also Section 2 where they appear in the geometry rename table.

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `*jco` | `JCO` | connection segment index array | `jco` *(c_geometry)* | `*junction_segs` |
| `jsno` | `JSNO` | number of junction entries | `jsno` *(c_geometry)* | `num_junction_segs` |
| `maxcon` | *(OpenNEC addition)* | allocated connection capacity | `maxcon` *(c_geometry)* | `max_connections` |
| `*ax` | `AX` | constant-current basis function coefficients | `ax` *(c_geometry)* | `*coeff_const` |
| `*bx` | `BX` | sine-current basis function coefficients | `bx` *(c_geometry)* | `*coeff_sine` |
| `*cx` | `CX` | cosine-current basis function coefficients | `cx` *(c_geometry)* | `*coeff_cos` |

---

## 10. `vsorc_t` Field Renames
*(Fortran COMMON `/VSORC/`, nec2c `vsorc_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `*isant` | `ISANT` | applied-field voltage source segment numbers | `source_segment_array` | `*vsrc_segs` |
| `*ivqd` | `IVQD` | charge-discontinuity source segment numbers | `ivqd` | `*qdsrc_segs` |
| `*iqds` | `IQDS` | charge-discontinuity source indices | `iqds` | `*qdsrc_indices` |
| `nsant` | `NSANT` | number of voltage sources | `voltage_source_count` | `num_vsrcs` |
| `nvqd` | `NVQD` | number of charge-disc sources | `nvqd` | `num_qdsrcs` |
| `nqds` | `NQDS` | number of charge-disc sources used | `nqds` | `num_qdsrcs_used` |
| `*vqd` | `VQD` | charge-disc source voltages | `vqd` | `*qdsrc_voltages` |
| `*vqds` | `VQDS` | charge-disc source voltages (saved copy) | `vqds` | `*qdsrc_voltages_saved` |
| `*vsant` | `VSANT` | applied-field source voltages | `source_voltage_array` | `*vsrc_voltages` |

---

## 11. `yparm_t` Field Renames
*(Fortran COMMON `/YPARM/`, nec2c `yparm_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `ncoup` | `NCOUP` | number of coupling pairs | `ncoup` | `num_pairs` |
| `icoup` | `ICOUP` | coupling computation flag | `icoup` | `coupling_flag` |
| `*nctag` | `NCTAG` | coupling pair tag numbers | `nctag` | `*pair_tags` |
| `*ncseg` | `NCSEG` | coupling pair segment numbers | `ncseg` | `*pair_segs` |
| `*y11a` | `Y11A` | self-admittance Y11 array | `y11a` | `*y11` |
| `*y12a` | `Y12A` | mutual admittance Y12 array | `y12a` | `*y12` |

---

## 12. `zload_t` Field Renames
*(Fortran COMMON `/ZLOAD/` + main program locals, nec2c `zload_t`, necpp → `nec_context`)*

| Current Field | Fortran Origin | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `nload` | `NLOAD` in `/ZLOAD/` | number of loading entries | `nload` | `num_loads` |
| `*zarray` | `ZARRAY` in `/ZLOAD/` | per-segment normalized impedance | `zarray` | `*seg_impedance` |
| `*ldtyp` | `LDTYP` *(local in main)* | loading type codes | `ldtyp` | `*load_types` |
| `*ldtag` | `LDTAG` *(local in main)* | loading tag numbers | `ldtag` | `*load_tags` |
| `*ldtagf` | `LDTAGF` *(local in main)* | loading tag range start | `ldtagf` | `*load_tag_from` |
| `*ldtagt` | `LDTAGT` *(local in main)* | loading tag range end | `ldtagt` | `*load_tag_to` |
| `*zlr` | `ZLR` *(local in main)* | R value (Ω, H, or F depending on type) | `zlr` | `*load_r` |
| `*zli` | `ZLI` *(local in main)* | L value | `zli` | `*load_l` |
| `*zlc` | `ZLC` *(local in main)* | C value | `zlc` | `*load_c` |

---

## 13. `gwav_t` Field Renames
*(Fortran COMMON `/GWAV/`, nec2c `gwav_t`, necpp `c_ground_wave`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `r1` | `R1` | distance to source (image 1) | `r1` | `range1` |
| `r2` | `R2` | distance to source (image 2) | `r2` | `range2` |
| `zmh` | `ZMH` | z − z′, height difference | `zmh` | `z_img1` |
| `zph` | `ZPH` | z + z′, height sum | `zph` | `z_img2` |
| `u` | `U` | ground impedance ratio | `u` | `impedance_ratio` |
| `u2` | `U2` | impedance ratio squared | `u2` | `impedance_ratio_sq` |
| `xx1` | `XX1` | current moment × phase factor (image 1) | `xx1` | `cur_phase1` |
| `xx2` | `XX2` | current moment × phase factor (image 2) | `xx2` | `cur_phase2` |

---

## 14. `tmi_t` Field Renames
*(Fortran COMMON `/TMI/`, nec2c `tmi_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `ij` | `IJX` | kernel type selector (0/±1) | `ija` | `kernel_type` |
| `zpk` | `ZPK` | segment center z-coordinate | `zpk` | `seg_center_z` |
| `rkb2` | `RKB2` | (k · radius)² | `rkb2` | `k_radius_sq` |

---

## 15. `tmh_t` Field Renames
*(Fortran local variables in HFK subroutine, nec2c `tmh_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `zpka` | `ZPKA` | segment center z-coord (H-field version) | `zpka` | `seg_center_z` |
| `rhks` | `RHKS` | k · radius (H-field version) | `rhks` | `k_radius` |

---

## 16. `matpar_t` Field Renames
*(Fortran COMMON `/MATPAR/`, nec2c `matpar_t`, necpp → dissolved into `nec_context`)*

Note: Fortran had many more fields (`NBLOKS`, `NBLSYM`, `NPSYM`, `NLSYM`, `ICASX`, `NBBX`, `NPBX`,
`NLBX`, `NBBL`, `NPBL`, `NLBL`) — nec2c/OpenNEC reduced these to local variables inside
`factr`/`fblock`.

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `icase` | `ICASE` | matrix storage mode (1–5) | `icase` | `storage_case` |
| `npblk` | `NPBLK` | rows per block | `npblk` | `block_rows` |
| `nlast` | `NLAST` | rows in last block | `nlast` | `last_block_rows` |
| `imat` | `IMAT` | complex words of core storage used | *(absent — necpp uses dynamic alloc)* | `core_used` |

---

## 17. `ggrid_t` Field Renames
*(Fortran COMMON `/GGRID/`, nec2c `ggrid_t`, necpp `c_ggrid`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `nxa[3],nya[3]` | `NXA(3),NYA(3)` | grid point counts (3 sub-grids) | `m_nxa[3], m_nya[3]` | `grid_nx[3], grid_ny[3]` |
| `dxa[3],dya[3]` | `DXA(3),DYA(3)` | grid spacing | `m_dxa[3], m_dya[3]` | `grid_dx[3], grid_dy[3]` |
| `xsa[3],ysa[3]` | `XSA(3),YSA(3)` | grid origin coordinates | `m_xsa[3], m_ysa[3]` | `grid_x0[3], grid_y0[3]` |
| `epscf` | `EPSCF` | complex ground dielectric constant for grid | `m_epscf` | `dielectric` |
| `*ar1,*ar2,*ar3` | `AR1(11,10,4), AR2(17,5,4), AR3(9,8,4)` | Sommerfeld interpolation tables | `m_ar1, m_ar2, m_ar3` | `*table1, *table2, *table3` |

---

## 18. `incom_t` Field Renames
*(Fortran COMMON `/INCOM/`, nec2c `incom_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `isnor` | `ISNOR` | use Sommerfeld (1) vs Norton (0) approximation | `isnor` | `use_sommerfeld` |
| `xo,yo,zo` | `XO,YO,ZO` | observation point coordinates | `xo, yo, zo` | `obs_x, obs_y, obs_z` |
| `sn` | `SN` | sin(α) of source segment | `sn` | `sin_alpha` |
| `xsn,ysn` | `XSN,YSN` | horizontal direction cosines of source | `xsn, ysn` | `dir_cos_x, dir_cos_y` |

---

## 19. `smat_t` Field Renames
*(Fortran COMMON `/SMAT/`, nec2c `smat_t`, necpp → dissolved into `nec_context`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `nop` | *(derived: NEQ/NPEQ)* | number of symmetry sections | `nop` | `num_sections` |
| `*ssx` | `SSX(16,16)` | symmetry mode transformation matrix | `symmetry_array` | `*mode_matrix` |

---

## 20. `plot_t` Field Renames
*(Fortran COMMON `/PLOT/`, nec2c `plot_t`, necpp `c_plot_card`)*

| Current Field | Fortran | Meaning | necpp C++ name | Proposed Name |
|---|---|---|---|---|
| `iplp1` | `IPLP1` | plot type selector | *(internal to c_plot_card)* | `plot_type` |
| `iplp2` | `IPLP2` | plot axis/variable selector | *(internal to c_plot_card)* | `plot_axis` |
| `iplp3` | `IPLP3` | plot component selector | *(internal to c_plot_card)* | `plot_component` |
| `iplp4` | `IPLP4` | plot gain/field selector | *(internal to c_plot_card)* | `plot_gain_type` |

---

## 21. Function Renames

### Geometry Functions

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `wire` | `WIRE` | `wire()` | ✅ keep `wire` |
| `arc` | `ARC` | `arc()` | ✅ keep `arc` |
| `helix` | `HELIX` | `helix()` | ✅ keep `helix` |
| `patch` | `PATCH` | `patch()` / `sp_card()` | ✅ keep `patch` |
| `calculate_patch` | `SUBPH` (ENTRY in PATCH) | *(merged into patch)* | ✅ keep `calculate_patch` |
| `connect_segments` | `CONECT` | *(internal to geometry_complete)* | ✅ keep `connect_segments` |
| `calculate_geometry` | `DATAGN` | `geometry_complete()` | ✅ keep `calculate_geometry` |
| `segment_number` | `ISEGNO` | `get_segment_number()` | ✅ keep `segment_number` |
| `reproduce` | `MOVE` | `move()` | ✅ keep `reproduce` |
| `reflect` | `REFLC` | `reflect()` | ✅ keep `reflect` |
| `scale` | *(inline in GS handler)* | `scale()` | ✅ keep `scale` |
| `finish_geometry` | *(OpenNEC addition)* | *(absent)* | ✅ keep `finish_geometry` |

### Matrix / Calculation Functions

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `cmset` | `CMSET` | *(nec_context method)* | `fill_interaction_matrix` |
| `cmss` | `CMSS` | *(nec_context method)* | `fill_patch_patch_matrix` |
| `cmsw` | `CMSW` | *(nec_context method)* | `fill_patch_wire_matrix` |
| `cmws` | `CMWS` | *(nec_context method)* | `fill_wire_patch_matrix` |
| `cmww` | `CMWW` | *(nec_context method)* | `fill_wire_wire_matrix` |
| `cabc` | `CABC` | `get_current_coefficients()` | `compute_current_coefficients` |
| `etmns` | `ETMNS` | *(nec_context method)* | `fill_excitation_vector` |
| `factr` | `FACTR` | `lu_decompose()` *(matrix_algebra.h)* | `factor_matrix` |
| `factrs` | `FACTRS` | `factrs()` *(matrix_algebra.h)* | `factor_matrix_symmetric` |
| `fblock` | `FBLOCK` | *(internal)* | `factor_block_matrix` |
| `solve` | `SOLVE` | `solve()` *(matrix_algebra.h)* | ✅ keep `solve` |
| `solves` | `SOLVES` | `solves()` *(matrix_algebra.h)* | `solve_symmetric` |

### Field Computation Functions

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `efld` | `EFLD` | *(nec_context method)* | `e_field_segment` |
| `eksc` | `EKSC` | *(nec_context method)* | `e_field_thin_wire` |
| `ekscx` | `EKSCX` | *(nec_context method)* | `e_field_extended_wire` |
| `hsfld` | `HSFLD` | *(nec_context method)* | `h_field_segment` |
| `hsflx` | `HSFLX` | *(nec_context method)* | `h_field_segment_components` |
| `hintg` | `HINTG` | *(nec_context method)* | `h_field_patch` |
| `nefld` | `NEFLD` | *(nec_context method)* | `near_e_field` |
| `nhfld` | `NHFLD` | *(nec_context method)* | `near_h_field` |
| `nfpat` | `NFPAT` | *(nec_context method)* | `compute_near_field` |
| `ffld` | `FFLD` | *(nec_context method)* | `far_e_field` |
| `fflds` | `FFLDS` | `fflds()` *(c_geometry)* | `far_e_field_surface` |
| `gfld` | `GFLD` | *(nec_context method)* | `radiated_field_with_ground` |
| `rdpat` | `RDPAT` | *(nec_context → nec_radiation_pattern)* | `compute_radiation_pattern` |
| `qdsrc` | `QDSRC` | *(nec_context method)* | `charge_discontinuity_source` |
| `unere` | `UNERE` | *(nec_context method)* | `e_field_unit_patch_current` |
| `pcint` | `PCINT` | *(nec_context method)* | `integrate_patch_at_junction` |

### Ground / Sommerfeld Functions

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `gwave` | `GWAVE` | *(nec_context method)* | `ground_wave_field` |
| `sflds` | `SFLDS` | *(nec_context method)* | `sommerfeld_field` |
| `rom2` | `ROM2` | *(nec_context method)* | `romberg_integrate_sommerfeld` |
| `intrp` | `INTRP` | `ggrid_interpolate()` *(nec_ground)* | `interpolate_sommerfeld_grid` |
| `somnec` | `SOMNEC` | *(c_ggrid::initialize)* | ✅ keep `somnec` |
| `bessel` | `BESSEL` | `bessel()` *(free fn in c_evlcom.h)* | ✅ keep `bessel` |
| `evlua` | `EVLUA` | `evlua()` *(c_evlcom)* | `evaluate_sommerfeld_integrals` |
| `gshank` | `GSHANK` | `gshank()` *(c_evlcom)* | `shanks_integration` |
| `hankel` | `HANKEL` | `hankel()` *(free fn in c_evlcom.h)* | ✅ keep `hankel` |
| `lambda` | `LAMBDA` | `lambda()` *(c_evlcom)* | `sommerfeld_lambda` |
| `rom1` | `ROM1` | `rom1()` *(c_evlcom)* | `romberg_integrate_1d` |
| `saoa` | `SAOA` | `saoa()` *(c_evlcom)* | `sommerfeld_asymptotic` |
| `fbar` | `FBAR` | *(nec_context method)* | `norton_attenuation_factor` |

### Wire E-field / H-field Integration Helpers

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `gf` | `GF` | *(nec_context method)* | `wire_e_integrand` |
| `gx` | `GX` | *(nec_context method)* | `wire_end_contrib_thin` |
| `gxx` | `GXX` | *(nec_context method)* | `wire_end_contrib_extended` |
| `gh` | `GH` | *(nec_context method)* | `wire_h_integrand` |
| `hfk` | `HFK` | *(nec_context method)* | `h_field_filament` |
| `intx` | `INTX` | *(nec_context method)* | `romberg_integrate_wire_e` |
| `sbf` | `SBF` | *(nec_context method)* | `basis_func_component` |
| `tbf` | `TBF` | `tbf()` *(c_geometry)* | `compute_basis_func` |
| `trio` | `TRIO` | `trio()` *(c_geometry)* | `compute_all_basis_funcs_on_seg` |

### Utility Functions

| Current Name | Fortran | necpp C++ name | Proposed Name |
|---|---|---|---|
| `db10` | `DB10` | *(inline in context)* | ✅ keep `db10` |
| `db20` | `DB20` | *(inline in context)* | ✅ keep `db20` |
| `cang` | `CANG` | *(inline)* | `complex_angle_deg` |
| `test` | `TEST` | `test()` *(matrix_algebra.h)* | `test_romberg_convergence` |
| `couple` | `COUPLE` | *(nec_context method)* | `compute_coupling` |
| `load` | `LOAD` | *(nec_context method)* | `apply_impedance_loading` |
| `network` | `NETWK` | *(nec_context method)* | ✅ keep `network` |
| `zint` | `ZINT` | *(nec_context method)* | `wire_surface_impedance` |
| `zpnorm` | `ZPNORM` | `impedance_norm_factor` *(renamed by necpp)* | ✅ adopt `impedance_norm_factor` |

---

## 22. Fortran EQUIVALENCE Traps — Naming History

These Fortran EQUIVALENCEs explain why some field names look semantically wrong in context.
OpenNEC has already structurally resolved all of them; this table is for documentation only.

| OpenNEC Field (resolved) | Original Fortran Array | Context Where They Aliased |
|---|---|---|
| `dir_cos_x` (`cab`) | `ALP` | Wire direction cosines used same storage |
| `dir_cos_y` (`sab`) | `BET` | Wire direction cosines used same storage |
| `end2_x/y/z` (`x2/y2/z2`) | `SI, ALP, BET` | Only during geometry construction, before `CABC` |
| `patch_t1x/y/z` | `SI, ALP, BET` | Patch tangent T1 reused wire length/angle storage |
| `patch_t2x/y/z` | `ICON1, ICON2, ITAG` | Patch tangent T2 reused connection/tag storage |
| `patch_t1x` in `dataj_t` | `CABJ, SABJ, SALPJ` | When source element is a patch |
| `patch_t2x` in `dataj_t` | `B` (radius) | When source element is a patch |
| `patch_t2y` in `dataj_t` | `IND1` | When source element is a patch |
| `patch_t2z` in `dataj_t` | `IND2` | When source element is a patch |
| `dir_cos_z` (`salp`) | Separate `/ANGL/` block | Merged into geometry struct by nec2c |

---

## 23. necpp Architectural Observations

These observations from reading the necpp source may inform OpenNEC design decisions:

1. **Monolithic context vs. structs**: necpp dissolved nearly all Fortran COMMON blocks directly
   into `nec_context` public fields — the opposite of OpenNEC's struct-per-COMMON approach.
   OpenNEC's approach is cleaner. Do not change OpenNEC's struct decomposition.

2. **`c_geometry` absorbed `/SEGJ/`**: necpp moved `jco`/`jsno`/`ax`/`bx`/`cx` directly into
   the geometry class, the same as OpenNEC's current `geometry_t`. Good validation.

3. **`nec_ground` renamed well**: `radial_wire_count`, `radial_wire_length`,
   `radial_wire_radius`, `cliff_edge_distance`, `cliff_height` — necpp introduced exactly the
   descriptive names proposed above. Strong endorsement.

4. **`nec_radiation_pattern` split `fpat_t`**: necpp moved per-result data (angles, gain
   arrays) into `nec_radiation_pattern` and kept per-run control flags in `nec_context`.
   OpenNEC keeps everything in `fpat_t` per run context; this is fine for a single-run tool.

5. **`impedance_norm_factor` (was `zpnorm`)**: necpp explicitly renamed `zpnorm` to
   `impedance_norm_factor` — adopt that name directly.

6. **`c_evlcom` kept cryptic names**: necpp's Sommerfeld evaluator kept `m_ck2`, `m_ct1`,
   etc. OpenNEC should prefer the descriptive names from the plan above.

7. **Matrix algebra split**: necpp factored `lu_decompose`/`solve` into a separate
   `matrix_algebra.h` free-function module. OpenNEC's `matrix.c/h` already parallels this.

---

## Implementation Notes

- **Phase 1** (struct type renames): global find-replace across all `.c`/`.h` files; trivial.
- **Phase 2** (field renames): do one struct at a time; build and run regression tests after each.
  The geometry `n`, `m`, `x`, `y`, `z` fields appear hundreds of times — use a scoped replace
  (`ctx->geometry.n` → `ctx->geometry.num_segs`) to avoid false matches on bare variable names.
- **Phase 3** (function renames): update declaration in `.h`, definition in `.c`, and all call
  sites; build to confirm.
- **Verification**: `make` must build clean; `test/regression_tests/regression_harness.sh` must
  show zero new diffs versus the pre-rename baseline. These are pure identifier renames with no
  logic changes, so any diff indicates a missed substitution.
