MMANA‑GAL .maa File Format Survey
===================================

This document was produced by scanning all 935 `.maa`/`.mma` files found in the
`AntennaFiles-OLD-master` collection and comparing the structure of each one
against the grammar described in `MMA file format.md`.

Summary of findings
--------------------

| Metric | Count |
|--------|-------|
| Total files scanned | 935 |
| **Variant A** – combined counts line (`nw nl ns`), wires follow immediately | 729 |
| **Variant B** – per-section `***Header***` with inline count line | 205 |
| Misnamed NEC-format file (`.maa` extension, actual NEC deck) | 1 |
| Files containing `***Source***` sections | 220 |
| Files containing `***Load***` sections | 220 |
| Files containing `***Segmentation***` sections | 220 |
| Files containing `###Comment###` blocks | 17 |

Key findings
------------

### Headers are effectively required

934 of 935 files contain `***Wires***` and other `***…***` section headers.
The single headerless file (`40m 2E Wire Beam.maa`) is in fact a valid NEC deck
saved with the wrong extension — it contains CM/CE/SY/GW/GE/EX/FR/EN cards and
is not a MMANA file at all.

The `MMA file format.md` grammar says headers are optional ("purely cosmetic").
**The data shows headers should be treated as expected** and the grammar should
be revised accordingly.

### Two distinct file variants

**Variant A** (729 files, ≈ 78 %):

    <title>
    <frequency>
    <nw> <nl> <ns>        ← combined counts for wires, loads, sources
    <wire line 1>
    …
    ***Source***
    <source count>
    <source line(s)>
    ***Load***
    <load count>
    <load line(s)>
    ***Segmentation***
    …

The `***Wires***` header may or may not appear before the wire block; the
counts line is self-contained.

**Variant B** (205 files, ≈ 22 %):

    <title>
    *                     ← single asterisk separator (sometimes blank)
    <frequency>
    ***Wires***
    <nw>                  ← wire count alone on its own line
    <wire line 1>
    …
    ***Source***
    <ns> , <0>            ← source count (wire, offset)
    <source spec lines>
    ***Load***
    <nl> , <0>
    <load spec lines>
    ***Segmentation***
    …

The `***Wires***` header is **required** in Variant B to locate the wire count. The source and load counts use a two-value `wire, offset` form rather than a single integer.

### Comment blocks are rare

Only 17 files use `###Comment###` markers.  The marker may be followed by the comment text on the same line or on the immediately following line.

Per-file analysis
-----------------

Columns:
- **File** – path relative to collection root
- **Variant** – A, B, or NEC
- **Wires** – number of GW-type wire lines detected
- **Has comments** – whether `###Comment###` appears
- **Notes**

| File | Variant | Wires | Comments | Notes |
|------|---------|-------|----------|-------|
| `144CQlomba.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `15-10-DualbandCubicalQuad.maa` | A | 17 |  |  |
| `15m Loop near ground.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `15m Vertical Yagi near ground.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `15mStackedYagi.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `160m_grounded_tower_Igor.maa` | A | 74 |  |  |
| `160m_grounded_tower_Igor_inner_gamma.maa` | A | 78 |  |  |
| `160m_grounded_tower_Igor_match_net.maa` | A | 78 |  |  |
| `160m_wire_yagi_Igor.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Droopy dipole Combined.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Droopy dipole-static discharge.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Droopy dipole.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Loop Dielectric Test.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Loop.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 Vee Dipole.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 broadband GM Dipole.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `1E40 dipole stack.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `20_15_10_Diamond_Loop_New_split_feed.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `20_17_15_10_Diamond_Loop_New_split_feed.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `20_17_15_10_SQ_LOOP.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `20_17_15_SQ_LOOP.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `2E15 Vertcal Yagi salt water.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `40CQ CW.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `40CQ Frame.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `40CQ SSB.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `40CQ.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `40HL CW.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `40HL Frame.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `40HL SSB.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `40Loop.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m 2E Wire Beam.maa` | NEC | 1 |  | Not MMANA format – NEC deck with wrong extension |
| `40m HWF.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m Loop near ground.maa` | B | 1 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m Vertical 50 Ohm 2 Radial.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m Vertical 50 Ohm.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m Vertical near ground.maa` | B | 1 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m band Loop.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m cube quad.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m_OWA_DIPOLE.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m_vert_compromised1.maa` | B | 27 |  | Per-section headers; wire count inside `***Wires***` block |
| `40m_vert_compromised2.maa` | B | 27 |  | Per-section headers; wire count inside `***Wires***` block |
| `80m_wire_yagi.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `Active Antenna Array - Loop.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `Active Antenna Array - Monopole.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `BIG Antenna Array - Monopole.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `Broadband 80m.5.maa` | A | 8 | Yes |  |
| `Broadband 80m.6.maa` | A | 8 | Yes |  |
| `Broadband 80m.7.maa` | A | 6 | Yes |  |
| `Broadband 80m.maa` | A | 6 | Yes |  |
| `CQ-20-15-10.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `CQ-20-17-15-10.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Compromised 40M 2E Loop 2.maa` | B | 11 |  | Per-section headers; wire count inside `***Wires***` block |
| `Compromised 40M 2E Loop.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `Crazy 15m band Array.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `DV3.5_4MHz.2.maa` | A | 31 | Yes |  |
| `DV3.5_4MHz.maa` | A | 3 | Yes |  |
| `Destructiv Test.maa` | B | 28 |  | Per-section headers; wire count inside `***Wires***` block |
| `Dip80m+UR0GT-match.2.maa` | A | 10 | Yes |  |
| `Dip80m+UR0GT-match.maa` | A | 8 | Yes |  |
| `G5RV.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `InPhase Array.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `K6STI Receive Loop.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `Moxon40.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_1E_40Loop.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_1E_40Loop_Cardioid_EU.maa` | B | 44 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_1E_40Loop_EU.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_1E_40Loop_JA_USA.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_2E_Compromised_40Loop.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_40CQ.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `TOWER_JTF_installed dipole.maa` | B | 52 |  | Per-section headers; wire count inside `***Wires***` block |
| `UR0GT_DEWD.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `UR0GT_DEWD_TEST.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `broadband_112_165mhz.maa` | A | 6 | Yes |  |
| `dipole-wide7.maa` | A | 6 | Yes |  |
| `open wire installed.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/15M_GP_beam.maa` | A | 6 |  |  |
| `ANT/15M_MOXON.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/15M_Vert_Moxon.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/15M_half_square.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/20M_Omega_Dipole.maa` | A | 10 |  |  |
| `ANT/20M_Omega_Dipole_1.maa` | A | 10 | Yes |  |
| `ANT/20M_Omega_Dipole_2.maa` | A | 10 | Yes |  |
| `ANT/2CQ15.MAA` | A | 9 |  |  |
| `ANT/40M_15M_Low_Loss_Dipole.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_15M_Low_Loss_Dipole_75ohm.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Cap_Hat_Dipole.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Cap_Hat_Dipole_Beta_Match.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Cap_Hat_Dipole_Omega.maa` | B | 13 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Cap_Hat_Openwire.maa` | B | 15 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Cap_Hat_Pasang.maa` | B | 17 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Full_Sized_Vee_Dipole.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Inverted_L_FCP.maa` | B | 7 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_L_Match_End_Loaded_Dipole.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Linear_Loaded_Dipole.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Omega_Dipole.maa` | A | 10 |  |  |
| `ANT/40M_Omega_Dipole_2.maa` | A | 10 | Yes |  |
| `ANT/40M_Pennant_EA3VY.maa` | A | 5 | Yes |  |
| `ANT/40M_Shortened_Vertical_Elevated_Ground.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Shortened_Vertical_OnGround.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_TriWire_FlatTop.maa` | B | 11 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_TriWire_InvVee.maa` | B | 15 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Tubing_Cap_Hat_Omega.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Tubing_Cap_Hat_Omega_2.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Tubing_Omega_Dipole_2.maa` | A | 10 | Yes |  |
| `ANT/40M_Tunable_Cap_Hat_Dipole.maa` | B | 11 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Wire_Beam_2E.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_Wire_Beam_2E_Top_load.maa` | A | 6 |  |  |
| `ANT/40M_Wire_Cap_Hat_Dipole_Omega.maa` | B | 13 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_half_square.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_phased_vert.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_phased_vert_elevated.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_top_load_backup.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40M_top_load_backup_elevated.maa` | B | 11 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/40m_GP_tower_JTF.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/C-Pole Wire.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/D4080S.maa` | B | 15 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/D40S.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/D80C.maa` | B | 27 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/D80CC.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/D80S.maa` | B | 18 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/K6STI_YD1SDL.maa` | A | 11 |  |  |
| `ANT/Opti_Omega.maa` | B | 13 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/RA9MB-40M.maa` | A | 5 | Yes |  |
| `ANT/Shortened_Dipole_40_PVC.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Shortened_Vee_Dipole_40.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_.maa` | B | 34 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_40M_Cap_Hat_Dipole.maa` | B | 37 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_40M_Full_Sized_PVC.maa` | B | 41 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_40M_TriWire_InvVee.maa` | B | 47 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_40M_TriWire_InvVee_and_Dipole.maa` | B | 52 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_Shortened_Vee_Dipole.maa` | B | 42 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_JTF_blank.maa` | B | 32 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/TOWER_SATU_STICK.maa` | B | 154 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Vee_Doublet.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Stacks/2x6el10.maa` | A | 7 |  |  |
| `ANT/Stacks/4X12.MAA` | A | 18 |  |  |
| `ANT/Stacks/4xQQ.maa` | A | 9 |  |  |
| `ANT/Stacks/hornYagi-2.maa` | A | 17 |  |  |
| `ANT/Stacks/hornYagi-3.maa` | A | 13 |  |  |
| `ANT/Stacks/syack3el20.maa` | A | 5 |  |  |
| `ANT/Feeders/0,25lambda_trans.maa` | A | 5 |  |  |
| `ANT/Feeders/600 to 250 Ohm trans.maa` | A | 13 |  |  |
| `ANT/Feeders/600 to 300 Ohm stub trans.maa` | A | 9 |  |  |
| `ANT/Feeders/650-300 Ohn Delta trans.maa` | A | 5 |  |  |
| `ANT/Feeders/FEEDER.MAA` | A | 5 |  |  |
| `ANT/Match/0,25lambda_trans.maa` | A | 5 |  |  |
| `ANT/Match/4EL20CM.maa` | A | 12 |  |  |
| `ANT/Match/4EL20HM.MAA` | A | 12 |  |  |
| `ANT/Match/4el6m-T-match-1.maa` | A | 9 |  |  |
| `ANT/Match/4el6m-T-match-C-1.maa` | A | 12 |  |  |
| `ANT/Match/4el6m-T-match-C.maa` | A | 12 |  |  |
| `ANT/Match/4el6m-T-match.maa` | A | 10 |  |  |
| `ANT/Match/600 to 250 Ohm trans.maa` | A | 13 |  |  |
| `ANT/Match/600 to 300 Ohm stub trans.maa` | A | 9 |  |  |
| `ANT/Match/650-300 Ohn Delta trans.maa` | A | 5 |  |  |
| `ANT/Match/Asym non res Omega GP.maa` | A | 9 |  |  |
| `ANT/Match/Asym res Omega GP.maa` | A | 9 |  |  |
| `ANT/Match/D-match.maa` | A | 7 |  |  |
| `ANT/Match/Delta-Gamma.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Match/Dipole+L match.maa` | A | 2 |  |  |
| `ANT/Match/GP with wide C+LC match.maa` | A | 9 |  |  |
| `ANT/Match/GP with wide L+LC match.maa` | A | 4 |  |  |
| `ANT/Match/GP+C match.maa` | A | 9 |  |  |
| `ANT/Match/GP-polosa-1.maa` | A | 4 |  |  |
| `ANT/Match/GP-polosa.maa` | A | 4 |  |  |
| `ANT/Match/GP14omega6,3m.maa` | A | 13 |  |  |
| `ANT/Match/Gamma offset dipole-1.maa` | A | 7 |  |  |
| `ANT/Match/Gamma offset dipole.maa` | A | 7 |  |  |
| `ANT/Match/Gamma offset non-resonance dipole.maa` | A | 7 |  |  |
| `ANT/Match/Gamma offset non-resonance short dipole.maa` | A | 7 |  |  |
| `ANT/Match/Gamma-10m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-13m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-2,3m-wide.maa` | A | 15 |  |  |
| `ANT/Match/Gamma-3,2m.maa` | A | 4 |  |  |
| `ANT/Match/Gamma-3,5m-wide.maa` | A | 16 |  |  |
| `ANT/Match/Gamma-3,5m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-4m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-5m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-6 m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-7m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-8m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma-9m.maa` | A | 5 |  |  |
| `ANT/Match/Gamma.maa` | A | 7 |  |  |
| `ANT/Match/MW-Broadcasting.maa` | A | 386 |  |  |
| `ANT/Match/Non res GP with asym wire gamma 1.maa` | A | 7 |  |  |
| `ANT/Match/Non res asym omega long dipole.maa` | A | 9 |  |  |
| `ANT/Match/Non res omega long dipole.maa` | A | 10 |  |  |
| `ANT/Match/Non res omega short dipole.maa` | A | 10 |  |  |
| `ANT/Match/Non-res GP with asym gamma-1.maa` | A | 7 |  |  |
| `ANT/Match/Non-res GP with asym gamma.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 10m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 13m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 3m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 4m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 5m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 6m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 7m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 8m.maa` | A | 7 |  |  |
| `ANT/Match/Omega GP 9m.maa` | A | 7 |  |  |
| `ANT/Match/Omega asym dipole.maa` | A | 9 |  |  |
| `ANT/Match/Omega dipole-1.maa` | A | 10 |  |  |
| `ANT/Match/Omega dipole-2.maa` | A | 10 |  |  |
| `ANT/Match/Omega dipole-3.maa` | A | 10 |  |  |
| `ANT/Match/Omega dipole-4.maa` | A | 10 |  |  |
| `ANT/Match/Omega dipole-5.maa` | A | 10 |  |  |
| `ANT/Match/Omega dipole.maa` | A | 10 |  |  |
| `ANT/Match/RA9MB-3.5-3.8.maa` | A | 5 |  |  |
| `ANT/Match/RA9MB.maa` | A | 5 |  |  |
| `ANT/Match/Res GP with asym gamma.maa` | A | 7 |  |  |
| `ANT/Match/Res GP with asym wire gamma 1.maa` | A | 7 |  |  |
| `ANT/Match/Short-Gamma-dipole.maa` | A | 5 |  |  |
| `ANT/Match/T-match Yagi.maa` | A | 9 |  |  |
| `ANT/Match/T-match.maa` | A | 7 |  |  |
| `ANT/Match/Vert20L.maa` | A | 4 |  |  |
| `ANT/Match/Vert20s.maa` | A | 2 |  |  |
| `ANT/Match/WB gamma-dipole.maa` | A | 7 |  |  |
| `ANT/Match/Wide band GP match.maa` | A | 7 |  |  |
| `ANT/Match/Wide quad LC match.maa` | A | 9 |  |  |
| `ANT/Match/Wideband_match Sh GP-1.maa` | A | 7 |  |  |
| `ANT/Match/Wideband_match.maa` | A | 10 |  |  |
| `ANT/Match/Wideband_match1.maa` | A | 10 |  |  |
| `ANT/Match/Widebannd_match Sh GP.maa` | A | 10 |  |  |
| `ANT/Match/dipole-wide.maa` | A | 7 |  |  |
| `ANT/Match/dipole-wide1.maa` | A | 7 |  |  |
| `ANT/Match/dipole-wide2.maa` | A | 7 |  |  |
| `ANT/Match/wide Omega GP 5m.maa` | A | 7 |  |  |
| `ANT/Phased/2 el activ VP2E 40m.maa` | A | 5 |  |  |
| `ANT/Phased/2_el_pahs_ VP2E 40m.maa` | A | 5 |  |  |
| `ANT/Phased/40m 2square.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Phased/40m _2square0.125.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Phased/HB9CV.MAA` | A | 3 |  |  |
| `ANT/Phased/HB9CVH.MAA` | A | 16 |  |  |
| `ANT/Phased/HB9CVW.MAA` | A | 16 |  |  |
| `ANT/Phased/Switch_4_dir_phas_Inv V.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/0,25L_sloper.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/0.25 lamda sloper_2.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/0.25lambda sloper.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/0.25lambda sloper_1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/0.5 dipole+short radial.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Dipole/0.5 lamda dipole witn feed line -1.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/0.5 lamda dipole witn feed line.maa` | A | 8 |  |  |
| `ANT/HF simple/Dipole/0.5llamda dipole 0.25trans.maa` | A | 10 |  |  |
| `ANT/HF simple/Dipole/3.5and3.8 dipole -1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/80_75dipol.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/80_75dipol_par.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Dipole/BW dipole.maa` | A | 12 |  |  |
| `ANT/HF simple/Dipole/DP20.MAA` | A | 2 |  |  |
| `ANT/HF simple/Dipole/FD20.MAA` | A | 5 |  |  |
| `ANT/HF simple/Dipole/FD20_1.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/FD20_2.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/FD20_3.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/FD20_4.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/Fat_dipole.maa` | A | 20 |  |  |
| `ANT/HF simple/Dipole/GND via C.maa` | A | 34 |  |  |
| `ANT/HF simple/Dipole/Inv V lambda.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/Inv Web.maa` | A | 88 |  |  |
| `ANT/HF simple/Dipole/InvV40.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/J-ant28-horiz.maa` | A | 8 |  |  |
| `ANT/HF simple/Dipole/LW.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/LW14.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/LW14_1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/LW14_1R.maa` | A | 12 |  |  |
| `ANT/HF simple/Dipole/LW14_2bad.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/LW14_2bad_R.maa` | A | 9 |  |  |
| `ANT/HF simple/Dipole/LWV10_0.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/LWV10_1.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/LWV10_GND.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/NadenDip.maa` | A | 42 |  |  |
| `ANT/HF simple/Dipole/Piramida.maa` | A | 8 |  |  |
| `ANT/HF simple/Dipole/Sloper 15 deg.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/Sloper 30 deg.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/Sloper 45 deg.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/Sloper 60 deg.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/Sloper 75 deg.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/Sloper1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/VP2E 40m -1 0.25feed.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/VP2E 40m -1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/VP2E 40m.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Vbeam10.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Vbeam10_0.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Vbeam10_1.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Vbeam10_2.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/WIDEBAND DIPOLE.maa` | A | 12 |  |  |
| `ANT/HF simple/Dipole/Wave asimm dip.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Wave sloper dipole 40.maa` | A | 3 |  |  |
| `ANT/HF simple/Dipole/Wave_dip.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/Web.maa` | A | 88 |  |  |
| `ANT/HF simple/Dipole/Windom-2.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/Windom-3.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/Windom-4.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/Windom.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/dipol160.maa` | B | 1 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Dipole/dipole-wide.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/dipole-wide1.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/dipole-wide2.maa` | A | 6 |  |  |
| `ANT/HF simple/Dipole/end feer dipole.maa` | A | 5 |  |  |
| `ANT/HF simple/Dipole/fat dipole2-1.maa` | A | 64 |  |  |
| `ANT/HF simple/Dipole/fat dipole2.maa` | A | 38 |  |  |
| `ANT/HF simple/Dipole/fat dipole3.maa` | A | 26 |  |  |
| `ANT/HF simple/Dipole/fat dipole4.maa` | A | 26 |  |  |
| `ANT/HF simple/Dipole/sloper test.maa` | A | 4 |  |  |
| `ANT/HF simple/Dipole/wave dipole.maa` | A | 2 |  |  |
| `ANT/HF simple/Dipole/wideInvV80.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/0,25GP with ATU.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/0.25GP with slope radials.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/0.5 L  GP + feed line.maa` | A | 8 |  |  |
| `ANT/HF simple/Vertical/AsimmGP28..maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/AsimmGP28_200.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/AssymGP.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/CB5_8 lambda.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/Conus.maa` | A | 26 |  |  |
| `ANT/HF simple/Vertical/Discone.maa` | A | 66 |  |  |
| `ANT/HF simple/Vertical/FQuad.maa` | A | 23 |  |  |
| `ANT/HF simple/Vertical/FWB14_50.maa` | A | 42 |  |  |
| `ANT/HF simple/Vertical/Folded GP.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/Folded GP_50 Ohm.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/Folded_GP1.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Folded_GP1_50.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/Folded_GP1_75.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/Folded_GP_Long.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/GP with offset.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/GP40 1 mh.MAA` | A | 6 |  |  |
| `ANT/HF simple/Vertical/GP40.MAA` | A | 6 |  |  |
| `ANT/HF simple/Vertical/GP40S.MAA` | A | 6 |  |  |
| `ANT/HF simple/Vertical/Gamma 0.5 lamda GP.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/H-antenna-1.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/H-antenna-2.maa` | A | 8 |  |  |
| `ANT/HF simple/Vertical/H-antenna.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/Hy Gain GP-1.maa` | A | 34 |  |  |
| `ANT/HF simple/Vertical/Hy Gain GP-2.maa` | A | 34 |  |  |
| `ANT/HF simple/Vertical/Hy Gain GP.maa` | A | 34 |  |  |
| `ANT/HF simple/Vertical/Inv GP+SR.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Inv GP-1.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Inv GP.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Inv L.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/Inverted L.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/Inverted L1.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/J-ant.maa` | A | 8 |  |  |
| `ANT/HF simple/Vertical/Sh Folded GP.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/Slope GP-TAU.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Sloper GP.maa` | A | 2 |  |  |
| `ANT/HF simple/Vertical/Sloper long GP.maa` | A | 2 |  |  |
| `ANT/HF simple/Vertical/TAU -0.25-1.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/TAU -0.25.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/UW4HW+radials.maa` | A | 26 |  |  |
| `ANT/HF simple/Vertical/UW4HW+res radials.maa` | A | 24 |  |  |
| `ANT/HF simple/Vertical/UW4HW.maa` | A | 14 |  |  |
| `ANT/HF simple/Vertical/UW4HW_m.maa` | A | 22 |  |  |
| `ANT/HF simple/Vertical/VERT20.MAA` | A | 2 |  |  |
| `ANT/HF simple/Vertical/VERT20M.MAA` | A | 4 |  |  |
| `ANT/HF simple/Vertical/VERT53.MAA` | A | 5 |  |  |
| `ANT/HF simple/Vertical/VERT58.MAA` | A | 5 |  |  |
| `ANT/HF simple/Vertical/Vert0.4L+LC.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/Vert0.6L+ LC.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/Vert20L.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Vert20s+rad.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Vert20s.maa` | A | 2 |  |  |
| `ANT/HF simple/Vertical/Vert20s_75+rad.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/Vert20s_75.maa` | A | 2 |  |  |
| `ANT/HF simple/Vertical/WB14_28.maa` | A | 14 |  |  |
| `ANT/HF simple/Vertical/WB14_28_m.maa` | A | 22 |  |  |
| `ANT/HF simple/Vertical/WB14_50.maa` | A | 40 |  |  |
| `ANT/HF simple/Vertical/fold. unipole  shield.maa` | B | 50 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Vertical/mod J-1.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/mod J-2.maa` | A | 10 |  |  |
| `ANT/HF simple/Vertical/mod J-3.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/mod J-4.maa` | A | 13 |  |  |
| `ANT/HF simple/Vertical/mod J.maa` | A | 9 |  |  |
| `ANT/HF simple/Vertical/offcenter GP.maa` | A | 8 |  |  |
| `ANT/HF simple/Vertical/offcenter GP1.maa` | A | 7 |  |  |
| `ANT/HF simple/Vertical/GND tower/160m vert gnd tower.maa` | A | 13 |  |  |
| `ANT/HF simple/Vertical/GND tower/AWP+mr.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/GND tower/AWP.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/GND tower/GND Tower GP80 with  Yagi on top.maa` | A | 15 |  |  |
| `ANT/HF simple/Vertical/GND tower/GND tower shunt feed.maa` | A | 4 |  |  |
| `ANT/HF simple/Vertical/GND tower/GP-windom-1.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/GP-windom-2.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/GP-windom-3.maa` | A | 7 |  |  |
| `ANT/HF simple/Vertical/GND tower/GP-windom-4.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/GP-windom.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/Inv feed GP with Yagi on top.maa` | A | 20 |  |  |
| `ANT/HF simple/Vertical/GND tower/Inverted feed GP.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/GND tower/Inverted feed GP_1.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/GND tower/Inverted feed GP_2.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/GND tower/Inverted feed GP_3.maa` | A | 16 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP  160 with HF Yagi-m.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP  80 with HF Yagi.maa` | A | 12 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP  with HF Yagi gamma.maa` | A | 25 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP-1.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP-2.maa` | A | 5 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower GP.maa` | A | 3 |  |  |
| `ANT/HF simple/Vertical/GND tower/Tower160.maa` | A | 17 |  |  |
| `ANT/HF simple/Vertical/GND tower/inverted  GP.maa` | A | 6 |  |  |
| `ANT/HF simple/Vertical/GND tower/inverted  long GP.maa` | A | 6 |  |  |
| `ANT/HF simple/Loop/2t delta.maa` | A | 8 |  |  |
| `ANT/HF simple/Loop/Hentenna1.maa` | A | 8 |  |  |
| `ANT/HF simple/Loop/JJ2IXF.maa` | A | 8 |  |  |
| `ANT/HF simple/Loop/LOOP20-200.maa` | A | 5 |  |  |
| `ANT/HF simple/Loop/LOOP20-50 Ohm -1.maa` | A | 6 |  |  |
| `ANT/HF simple/Loop/LOOP20-50 Ohm.maa` | A | 5 |  |  |
| `ANT/HF simple/Loop/LOOP20.MAA` | A | 5 |  |  |
| `ANT/HF simple/Loop/LOOP20C.MAA` | A | 13 |  |  |
| `ANT/HF simple/Loop/LOOP20Tr V.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Loop/LOOP20V.maa` | A | 5 |  |  |
| `ANT/HF simple/Loop/LOOP20d.maa` | A | 4 |  |  |
| `ANT/HF simple/Loop/LOOP20r.maa` | A | 5 |  |  |
| `ANT/HF simple/Loop/LOOP80 Asymm.maa` | A | 7 |  |  |
| `ANT/HF simple/Loop/Q+Q L_4.maa` | B | 7 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Loop/Q+Q.maa` | B | 7 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Loop/Q+Q_sh+tune.maa` | A | 19 |  |  |
| `ANT/HF simple/Loop/Q+Q_short.maa` | B | 7 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF simple/Loop/ZZ1.maa` | A | 10 |  |  |
| `ANT/HF simple/Loop/ZZ1_hen_DL2KQ.maa` | A | 12 |  |  |
| `ANT/HF simple/Loop/ZZ2.maa` | A | 8 |  |  |
| `ANT/HF simple/Loop/ZZ2_hen_DL2KQ.maa` | A | 10 |  |  |
| `ANT/HF simple/Loop/delta-opt.maa` | A | 5 |  |  |
| `ANT/HF simple/Loop/delta-opt1.maa` | A | 5 |  |  |
| `ANT/HF beams/2CQ20.MAA` | A | 9 |  |  |
| `ANT/HF beams/2DELTA20.MAA` | A | 9 |  |  |
| `ANT/HF beams/2EL20.MAA` | A | 3 |  |  |
| `ANT/HF beams/2ELVP15.MAA` | A | 23 |  |  |
| `ANT/HF beams/2ELVP20.MAA` | A | 23 |  |  |
| `ANT/HF beams/2el.delta.20m.maa` | A | 8 |  |  |
| `ANT/HF beams/3EL20.MAA` | A | 5 |  |  |
| `ANT/HF beams/3el GP80 CW.maa` | A | 6 |  |  |
| `ANT/HF beams/3el_GP80_CW.maa` | A | 6 |  |  |
| `ANT/HF beams/4EL20.MAA` | A | 6 |  |  |
| `ANT/HF beams/4EL20HM.MAA` | A | 12 |  |  |
| `ANT/HF beams/5EL20.MAA` | A | 7 |  |  |
| `ANT/HF beams/6EL10.MAA` | A | 7 |  |  |
| `ANT/HF beams/DUAL2EL.MAA` | A | 7 |  |  |
| `ANT/HF beams/Db54m.maa` | A | 10 |  |  |
| `ANT/HF beams/G4ZU 14.maa` | A | 15 |  |  |
| `ANT/HF beams/HB9CV.MAA` | A | 3 |  |  |
| `ANT/HF beams/HB9CVH.MAA` | A | 16 |  |  |
| `ANT/HF beams/HB9CVW.MAA` | A | 16 |  |  |
| `ANT/HF beams/JungleJob.maa` | A | 4 |  |  |
| `ANT/HF beams/LPDA.MAA` | A | 64 |  |  |
| `ANT/HF beams/LPDA15.MAA` | A | 36 |  |  |
| `ANT/HF beams/M2CQ.MAA` | A | 25 |  |  |
| `ANT/HF beams/M2CQW.MAA` | A | 41 |  |  |
| `ANT/HF beams/QuadsUA4IF.maa` | A | 45 |  |  |
| `ANT/HF beams/Rhomb10.maa` | A | 5 |  |  |
| `ANT/HF beams/Switch 4 dir  active Inv V.maa` | A | 5 |  |  |
| `ANT/HF beams/Switch_4_dir_pahs_InvV.maa` | A | 5 |  |  |
| `ANT/HF beams/TribandQQ.maa` | A | 25 |  |  |
| `ANT/HF beams/V-Yagi.maa` | A | 10 |  |  |
| `ANT/HF beams/XQ20.MAA` | A | 22 |  |  |
| `ANT/HF beams/YagiQuad.maa` | A | 6 |  |  |
| `ANT/HF beams/ZL20.MAA` | A | 18 |  |  |
| `ANT/HF beams/dx415tt.maa` | A | 12 |  |  |
| `ANT/HF beams/w2eey.maa` | A | 16 |  |  |
| `ANT/Short/Magnetic loops/C- short-1.maa` | A | 8 |  |  |
| `ANT/Short/Magnetic loops/MAGLOOP.MAA` | A | 5 |  |  |
| `ANT/Short/Magnetic loops/MAGLOOP2.MAA` | A | 17 |  |  |
| `ANT/Short/Magnetic loops/MAGLOOPC.MAA` | A | 10 |  |  |
| `ANT/Short/Magnetic loops/MAGLOOPM.MAA` | A | 9 |  |  |
| `ANT/Short/Magnetic loops/MAGLOOPT.MAA` | A | 10 |  |  |
| `ANT/Short/Magnetic loops/Magn loop 12 m.maa` | A | 10 |  |  |
| `ANT/Short/Magnetic loops/Multiturnloop-2.maa` | A | 22 |  |  |
| `ANT/Short/Magnetic loops/Multiturnloop-WB-RX.maa` | A | 22 |  |  |
| `ANT/Short/Magnetic loops/Multiturnloop.maa` | A | 18 |  |  |
| `ANT/Short/Magnetic loops/Quad-C.maa` | A | 5 |  |  |
| `ANT/Short/Magnetic loops/hairpin monopol.maa` | A | 6 |  |  |
| `ANT/Short/Magnetic loops/hairpin monopol2.maa` | A | 6 |  |  |
| `ANT/Short/Magnetic loops/hairpin monopol3.maa` | A | 6 |  |  |
| `ANT/Short/Magnetic loops/mag_atg.maa` | A | 10 |  |  |
| `ANT/Short/Complex/160m vert gnd tower.maa` | A | 11 |  |  |
| `ANT/Short/Complex/AWP.maa` | A | 4 |  |  |
| `ANT/Short/Complex/CF-dipole.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/DDRR-DL2KQ.maa` | A | 25 |  |  |
| `ANT/Short/Complex/DDRR-W6UYH.maa` | A | 23 |  |  |
| `ANT/Short/Complex/DDRR-history next.maa` | A | 8 |  |  |
| `ANT/Short/Complex/DDRR-history.maa` | A | 4 |  |  |
| `ANT/Short/Complex/DDRR.maa` | A | 15 |  |  |
| `ANT/Short/Complex/DDRR1.maa` | A | 15 |  |  |
| `ANT/Short/Complex/DDRR2.maa` | A | 15 |  |  |
| `ANT/Short/Complex/DDRR3.maa` | A | 26 |  |  |
| `ANT/Short/Complex/DDRR4.maa` | A | 14 |  |  |
| `ANT/Short/Complex/DDRR5.maa` | A | 7 |  |  |
| `ANT/Short/Complex/DDRR6.maa` | A | 7 |  |  |
| `ANT/Short/Complex/Fold GP short monopole.maa` | A | 4 |  |  |
| `ANT/Short/Complex/Fold GP short monopole2.maa` | A | 4 |  |  |
| `ANT/Short/Complex/Fold GP short monopole3.maa` | A | 4 |  |  |
| `ANT/Short/Complex/Helix80+C.maa` | A | 22 |  |  |
| `ANT/Short/Complex/LC contur.maa` | A | 67 |  |  |
| `ANT/Short/Complex/Short 160m  grounded GP- best.maa` | A | 10 |  |  |
| `ANT/Short/Complex/Short 160m  grounded GP-best-1.maa` | A | 10 |  |  |
| `ANT/Short/Complex/Short 75- lin load+top.maa` | A | 10 |  |  |
| `ANT/Short/Complex/Short 75m  grounded GP- best.maa` | A | 9 |  |  |
| `ANT/Short/Complex/Short 75m  grounded GP-b.maa` | A | 6 |  |  |
| `ANT/Short/Complex/Short 75m  grounded GP.maa` | A | 6 |  |  |
| `ANT/Short/Complex/Short Quad 9.maa` | A | 13 |  |  |
| `ANT/Short/Complex/ShortGP75 with Top complex load.maa` | A | 6 |  |  |
| `ANT/Short/Complex/ShortGP75 with Top complex load_1.maa` | A | 10 |  |  |
| `ANT/Short/Complex/ShortGP75 with Top complex load_2.maa` | A | 18 |  |  |
| `ANT/Short/Complex/ShortGP75 with Top complex load_3.maa` | A | 18 |  |  |
| `ANT/Short/Complex/ShortGP75 with Top complex load_4.maa` | A | 4 |  |  |
| `ANT/Short/Complex/ShortGP75 with complex load.maa` | A | 6 |  |  |
| `ANT/Short/Complex/Top C+coax gamma+L.maa` | A | 23 |  |  |
| `ANT/Short/Complex/mini verlical.maa` | A | 33 |  |  |
| `ANT/Short/Complex/short fold GP-1.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold GP-2.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold GP-3.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold GP-4.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold GP-5.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold GP.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short fold dipole.maa` | B | 6 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Complex/short vetr75.maa` | A | 5 |  |  |
| `ANT/Short/Fractal/0.5 lamda  GP witn feed line.maa` | A | 8 |  |  |
| `ANT/Short/Fractal/0.5 lamda  deform  GP witn feed line -1.maa` | A | 10 |  |  |
| `ANT/Short/Fractal/0.5 lamda  deform  GP witn feed line.maa` | A | 10 |  |  |
| `ANT/Short/Fractal/80VVERT.MAA` | A | 12 |  |  |
| `ANT/Short/Fractal/B-dipol.maa` | A | 7 |  |  |
| `ANT/Short/Fractal/Coil vertical.maa` | A | 16 |  |  |
| `ANT/Short/Fractal/FoldedGP75_3.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/Fractal_14_1.maa` | A | 101 |  |  |
| `ANT/Short/Fractal/Fractal_2.maa` | A | 58 |  |  |
| `ANT/Short/Fractal/Lineload75.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/O-dipole.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/Piramida 2.maa` | A | 10 |  |  |
| `ANT/Short/Fractal/RA9OS20.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/Short Quad-2.maa` | A | 7 |  |  |
| `ANT/Short/Fractal/Short inv L offcenter.maa` | A | 4 |  |  |
| `ANT/Short/Fractal/Short inv L offcenter_1.maa` | A | 4 |  |  |
| `ANT/Short/Fractal/Short loop3.maa` | A | 13 |  |  |
| `ANT/Short/Fractal/ShortGP75-0.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/ShortGP75-2.maa` | A | 6 |  |  |
| `ANT/Short/Fractal/ShortGP75.maa` | A | 8 |  |  |
| `ANT/Short/Fractal/ShortGP75_1.maa` | A | 7 |  |  |
| `ANT/Short/Fractal/Short_folded_GP.maa` | A | 8 |  |  |
| `ANT/Short/Fractal/U-dipole.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Fractal/U-folded dipole-h.maa` | A | 13 |  |  |
| `ANT/Short/Fractal/U-folded dipole-v.maa` | A | 13 |  |  |
| `ANT/Short/Fractal/U-folded dipole-v1.maa` | A | 11 |  |  |
| `ANT/Short/Fractal/X-loop.maa` | A | 17 |  |  |
| `ANT/Short/Fractal/Z-dipol.maa` | A | 9 |  |  |
| `ANT/Short/Fractal/short loop GP-80.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Fractal/short_dip160.maa` | A | 16 |  |  |
| `ANT/Short/Curved/Curved dipole.maa` | A | 4 |  |  |
| `ANT/Short/Curved/Curved dipole1.maa` | A | 4 |  |  |
| `ANT/Short/Curved/Curved dipole2.maa` | A | 10 |  |  |
| `ANT/Short/Curved/Curved dipole3.maa` | A | 8 |  |  |
| `ANT/Short/Curved/Curved dipole4.maa` | A | 8 |  |  |
| `ANT/Short/Curved/Curved dipole5.maa` | A | 12 |  |  |
| `ANT/Short/Curved/Curved dipole6.maa` | A | 14 |  |  |
| `ANT/Short/Curved/Curved loop.maa` | A | 9 |  |  |
| `ANT/Short/Curved/Curved loop1.maa` | A | 13 |  |  |
| `ANT/Short/Curved/Folded GP75_4.maa` | A | 5 |  |  |
| `ANT/Short/Curved/Folded triangle GP-160.maa` | A | 4 |  |  |
| `ANT/Short/Curved/Folded triangle GP.maa` | A | 6 |  |  |
| `ANT/Short/Curved/FoldedGP75.maa` | A | 7 |  |  |
| `ANT/Short/Curved/FoldedGP75_1.maa` | A | 5 |  |  |
| `ANT/Short/Curved/FoldedGP75_2.maa` | A | 9 |  |  |
| `ANT/Short/Curved/FoldedGP75_3.maa` | A | 9 |  |  |
| `ANT/Short/Curved/Fractal dipole5.maa` | A | 12 |  |  |
| `ANT/Short/Curved/Fractal_0.maa` | A | 40 |  |  |
| `ANT/Short/Curved/Fractal_1.maa` | A | 62 |  |  |
| `ANT/Short/Curved/Fractal_3.maa` | A | 25 |  |  |
| `ANT/Short/Curved/Fractal_4.maa` | A | 15 |  |  |
| `ANT/Short/Curved/Inverted L.maa` | A | 3 |  |  |
| `ANT/Short/Curved/Trihat-quad-1.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Curved/Trihat-quad.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/Curved/V dipole.maa` | A | 3 |  |  |
| `ANT/Short/Curved/V dipole1.maa` | A | 4 |  |  |
| `ANT/Short/Curved/V dipole2.maa` | A | 4 |  |  |
| `ANT/Short/Curved/shortQuad.maa` | A | 12 |  |  |
| `ANT/Short/L/1sh+L.maa` | A | 4 |  |  |
| `ANT/Short/L/Auto with GP-7.maa` | A | 88 |  |  |
| `ANT/Short/L/Auto with GP.maa` | A | 90 |  |  |
| `ANT/Short/L/DL7PE-2.maa` | A | 3 |  |  |
| `ANT/Short/L/DL7PE.-1.maa` | A | 115 |  |  |
| `ANT/Short/L/DL7PE..maa` | A | 115 |  |  |
| `ANT/Short/L/DP160LD.MAA` | A | 4 |  |  |
| `ANT/Short/L/Gp40short rad.maa` | A | 7 |  |  |
| `ANT/Short/L/HELIX80.MAA` | A | 18 |  |  |
| `ANT/Short/L/Helix145.maa` | A | 50 |  |  |
| `ANT/Short/L/Helix20.maa` | A | 50 |  |  |
| `ANT/Short/L/Helix435.maa` | A | 20 |  |  |
| `ANT/Short/L/MCQM.MAA` | A | 12 |  |  |
| `ANT/Short/L/SLOPER.MAA` | A | 3 |  |  |
| `ANT/Short/L/Short  L dip 160 -1.maa` | A | 9 |  |  |
| `ANT/Short/L/Short Delta-Gamma-2.maa` | A | 9 |  |  |
| `ANT/Short/L/Short Delta-Gamma.maa` | A | 9 |  |  |
| `ANT/Short/L/Short Quad with stub.maa` | A | 17 |  |  |
| `ANT/Short/L/ShortGP75 with L.maa` | A | 2 |  |  |
| `ANT/Short/L/ShortGP75 with L_1.maa` | A | 2 |  |  |
| `ANT/Short/L/Strange GP.maa` | A | 6 |  |  |
| `ANT/Short/L/Strange dipole.maa` | A | 6 |  |  |
| `ANT/Short/L/VDP40.MAA` | A | 4 |  |  |
| `ANT/Short/L/VDP40B.MAA` | A | 2 |  |  |
| `ANT/Short/L/VDP40d.maa` | A | 4 |  |  |
| `ANT/Short/L/Vert+L145&435.maa` | A | 21 |  |  |
| `ANT/Short/L/Vert80CW.maa` | A | 6 |  |  |
| `ANT/Short/L/short L quad.maa` | B | 4 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Short/L/w0kph.maa` | A | 214 |  |  |
| `ANT/VHF/144omni.maa` | A | 14 |  |  |
| `ANT/VHF/6SKYDOOR.MAA` | A | 10 |  |  |
| `ANT/VHF/Big star.maa` | A | 18 |  |  |
| `ANT/VHF/CR2.MAA` | A | 63 |  |  |
| `ANT/VHF/Discon55-200.maa` | A | 18 |  |  |
| `ANT/VHF/GP144-435.maa` | A | 2 |  |  |
| `ANT/VHF/HENTENNA.MAA` | A | 8 |  |  |
| `ANT/VHF/Helix435.maa` | A | 20 |  |  |
| `ANT/VHF/Isotrop.maa` | A | 9 |  |  |
| `ANT/VHF/J-ant144.maa` | A | 8 |  |  |
| `ANT/VHF/Split J-ant V-H145.maa` | A | 13 |  |  |
| `ANT/VHF/TWIND.MAA` | A | 8 |  |  |
| `ANT/VHF/TWINLOOP.MAA` | A | 8 |  |  |
| `ANT/VHF/collinear 3x5_8lamda.maa` | A | 16 |  |  |
| `ANT/HF multibands/Parallel/14-21 parallel Quad.maa` | A | 14 |  |  |
| `ANT/HF multibands/Parallel/14-21-28 gamma Quad.maa` | B | 29 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Parallel/14-21-28 parallel Quad.maa` | A | 21 |  |  |
| `ANT/HF multibands/Parallel/DBLDP.MAA` | A | 6 |  |  |
| `ANT/HF multibands/Parallel/Dbldp40&10.maa` | A | 6 |  |  |
| `ANT/HF multibands/Parallel/Dipole20_10.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Parallel/InvV160-80.maa` | A | 6 |  |  |
| `ANT/HF multibands/Parallel/InvV80-40-20-15.maa` | A | 8 |  |  |
| `ANT/HF multibands/Parallel/InvV80-40.maa` | A | 6 |  |  |
| `ANT/HF multibands/Parallel/WARC dipole.maa` | A | 12 |  |  |
| `ANT/HF multibands/Parallel/dipole80_40.maa` | A | 8 |  |  |
| `ANT/HF multibands/Complex/10-18-24GP.maa` | A | 13 |  |  |
| `ANT/HF multibands/Complex/10-18-24SHdip.maa` | A | 8 |  |  |
| `ANT/HF multibands/Complex/10-18-24SHdipole.maa` | A | 10 |  |  |
| `ANT/HF multibands/Complex/10_14_18_21_24_28GP.maa` | A | 12 |  |  |
| `ANT/HF multibands/Complex/14&21 Dipol.maa` | A | 7 |  |  |
| `ANT/HF multibands/Complex/14&21 GP.maa` | A | 5 |  |  |
| `ANT/HF multibands/Complex/14-21-28.maa` | A | 8 |  |  |
| `ANT/HF multibands/Complex/14-21-28SHdip.maa` | A | 7 |  |  |
| `ANT/HF multibands/Complex/14-28 radials.maa` | A | 17 |  |  |
| `ANT/HF multibands/Complex/14_18_21_24_28GP.maa` | A | 11 |  |  |
| `ANT/HF multibands/Complex/14_21_28GP.maa` | A | 13 |  |  |
| `ANT/HF multibands/Complex/3.5-7-14-21-28_pohod_DL2KQ.maa` | A | 9 |  |  |
| `ANT/HF multibands/Complex/3.5-7-14-28offsetdip-1.maa` | A | 3 |  |  |
| `ANT/HF multibands/Complex/3.5_7 vert+inv  L.maa` | A | 7 |  |  |
| `ANT/HF multibands/Complex/7-14-21-28GPL.maa` | A | 13 |  |  |
| `ANT/HF multibands/Complex/7-14.maa` | A | 7 |  |  |
| `ANT/HF multibands/Complex/7_10_14_18_21_24_28GP.maa` | A | 15 |  |  |
| `ANT/HF multibands/Complex/CTSVR.maa` | A | 51 |  |  |
| `ANT/HF multibands/Complex/InvV80-40-20-15-10.maa` | A | 10 |  |  |
| `ANT/HF multibands/Complex/InvV80-40-20-15.maa` | A | 8 |  |  |
| `ANT/HF multibands/Complex/UT1MA.maa` | A | 12 |  |  |
| `ANT/HF multibands/Complex/UW4HW-m+10.maa` | A | 23 |  |  |
| `ANT/HF multibands/Complex/UW4HW-m+7.maa` | A | 23 |  |  |
| `ANT/HF multibands/Complex/WARC tpap dip.maa` | A | 4 |  |  |
| `ANT/HF multibands/LC in antenna/14-21-28 LC quad.maa` | A | 9 |  |  |
| `ANT/HF multibands/LC in antenna/160_40InvV.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/5b Delta CW.maa` | A | 5 |  |  |
| `ANT/HF multibands/LC in antenna/5b Delta CW_symm.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/6b Delta.maa` | A | 5 |  |  |
| `ANT/HF multibands/LC in antenna/6b Delta_symm.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/7-10 GP.maa` | A | 3 |  |  |
| `ANT/HF multibands/LC in antenna/7-3,5 GP.maa` | A | 4 |  |  |
| `ANT/HF multibands/LC in antenna/7-3,5 GP1.maa` | A | 3 |  |  |
| `ANT/HF multibands/LC in antenna/8b Delta.maa` | A | 5 |  |  |
| `ANT/HF multibands/LC in antenna/8b Delta_symm.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/Dipole_14-21-28.maa` | A | 12 |  |  |
| `ANT/HF multibands/LC in antenna/Dipole_14-21.maa` | A | 4 |  |  |
| `ANT/HF multibands/LC in antenna/Dipole_3,5-7.maa` | A | 4 |  |  |
| `ANT/HF multibands/LC in antenna/Dipole_7-10.maa` | A | 4 |  |  |
| `ANT/HF multibands/LC in antenna/GP14-21-28.maa` | A | 7 |  |  |
| `ANT/HF multibands/LC in antenna/GP_3.5-7-14pohod.maa` | A | 3 |  |  |
| `ANT/HF multibands/LC in antenna/Inv V 1,8-3,5 with L.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/MCQM.MAA` | A | 12 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/10-14GND GP.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/14-18-21-24-28GND GP.maa` | A | 7 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/14-21-28  GND GP-1.maa` | A | 15 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/14-21-28  GND GP-2.maa` | A | 15 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/14-21-28  GND GP.maa` | A | 15 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/14-21GND GP.maa` | A | 8 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/160_80N4PC vertical.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/18-24GND GP.maa` | A | 6 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/7-10GND GP.maa` | A | 7 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/Vert-loop 20-40.maa` | A | 8 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/Vert-loop 80-40-tower-s.maa` | A | 9 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/Vert-loop 80-40-tower.maa` | A | 8 |  |  |
| `ANT/HF multibands/LC in antenna/GND GP/Vert-loop 80-40.maa` | A | 8 |  |  |
| `ANT/HF multibands/Trap/10_18_24 trap vertical.maa` | A | 8 |  |  |
| `ANT/HF multibands/Trap/14_21GP.maa` | A | 3 |  |  |
| `ANT/HF multibands/Trap/14_21_28 trap vertical.maa` | A | 8 |  |  |
| `ANT/HF multibands/Trap/14_21_28trapGP+C.maa` | A | 10 |  |  |
| `ANT/HF multibands/Trap/14_21_28trapGP.maa` | A | 10 |  |  |
| `ANT/HF multibands/Trap/28_21GP.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Trap/28_21_14v3.maa` | A | 8 |  |  |
| `ANT/HF multibands/Trap/7_10_14_21_trapGP+C.maa` | A | 5 |  |  |
| `ANT/HF multibands/Trap/7_10_14_trapGP+C.maa` | A | 4 |  |  |
| `ANT/HF multibands/Trap/80_40 Inverted L.maa` | A | 4 |  |  |
| `ANT/HF multibands/Trap/Delta 10-14.maa` | A | 5 |  |  |
| `ANT/HF multibands/Trap/GP10-14s.maa` | A | 7 |  |  |
| `ANT/HF multibands/Trap/GP14-18s.maa` | A | 7 |  |  |
| `ANT/HF multibands/Trap/K2GUm_7_14_28.maa` | A | 4 |  |  |
| `ANT/HF multibands/Trap/MULTDPH.MAA` | A | 8 |  |  |
| `ANT/HF multibands/Trap/MULTDPHW.MAA` | A | 14 |  |  |
| `ANT/HF multibands/Trap/Quad18-21s.maa` | A | 10 |  |  |
| `ANT/HF multibands/Trap/Trap dipole 1,8_3,5_7.maa` | A | 6 |  |  |
| `ANT/HF multibands/Trap/Trap dipole 3,7_7_10.maa` | A | 6 |  |  |
| `ANT/HF multibands/Trap/W3DZZm.maa` | A | 4 |  |  |
| `ANT/HF multibands/Trap/W3DZZm_invV.maa` | A | 5 |  |  |
| `ANT/HF multibands/Trap/WARC_Dip.maa` | A | 6 |  |  |
| `ANT/HF multibands/Only size/10_24 GP with 0.25L trap.maa` | A | 4 |  |  |
| `ANT/HF multibands/Only size/10_28 GP with 0.25L trap.maa` | A | 5 |  |  |
| `ANT/HF multibands/Only size/14_7Dipole.maa` | A | 3 |  |  |
| `ANT/HF multibands/Only size/3.5-7-14-18-24-28offsetdip-2.maa` | A | 3 |  |  |
| `ANT/HF multibands/Only size/7-10-14-18-21-24-28 Inv V.maa` | A | 15 |  |  |
| `ANT/HF multibands/Only size/7-14-21-28offsetdip.maa` | A | 3 |  |  |
| `ANT/HF multibands/Only size/7_21_28 GP with 0.25L trap.maa` | A | 5 |  |  |
| `ANT/HF multibands/Only size/Asimm dipole 3,5_7_14_24_28.maa` | A | 3 |  |  |
| `ANT/HF multibands/Only size/Assym dipole 7-14-28.maa` | A | 3 |  |  |
| `ANT/HF multibands/Only size/Dipole 80-20-10m.maa` | A | 28 |  |  |
| `ANT/HF multibands/Only size/J-ant-2b.maa` | A | 22 |  |  |
| `ANT/HF multibands/Only size/Kaktus.maa` | B | 23 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Only size/SHOEBOX.MAA` | A | 5 |  |  |
| `ANT/HF multibands/Only size/VA2ERY 10.maa` | A | 6 |  |  |
| `ANT/HF multibands/Only size/VA2ERY 15.maa` | A | 6 |  |  |
| `ANT/HF multibands/Only size/VA2ERY 20.maa` | A | 6 |  |  |
| `ANT/HF multibands/Only size/VA2ERY 40.maa` | A | 6 |  |  |
| `ANT/HF multibands/Only size/Windom-7_14.maa` | A | 4 |  |  |
| `ANT/HF multibands/Only size/dipole80-20.maa` | A | 10 |  |  |
| `ANT/HF multibands/Only size/long GP10.maa` | A | 11 |  |  |
| `ANT/HF multibands/OpenSleeve/1.8_3.5_3.8 MHz Sleeve.maa` | A | 4 |  |  |
| `ANT/HF multibands/OpenSleeve/14-18-21-24-28 Sleeve vertical -7 m.maa` | A | 16 |  |  |
| `ANT/HF multibands/OpenSleeve/14-18-21-24-28 Sleeve vertical.maa` | A | 15 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleeve 14-21-28.maa` | A | 6 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleve 14-18-21-24-28.maa` | A | 11 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleve 14-18-21-24-28_wire.maa` | A | 11 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleve 14-18-21-24-28_wire_tune.maa` | A | 15 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleve14C-21-28.maa` | A | 7 |  |  |
| `ANT/HF multibands/OpenSleeve/GP Sleve14C-21-28_up.maa` | A | 13 |  |  |
| `ANT/HF multibands/OpenSleeve/Sleve 7-24.maa` | A | 6 |  |  |
| `ANT/HF multibands/OpenSleeve/Sleve14-21-28.maa` | A | 4 |  |  |
| `ANT/HF multibands/OpenSleeve/Sleve14-21.maa` | A | 3 |  |  |
| `ANT/HF multibands/OpenSleeve/Sleve14-21_B.maa` | A | 3 |  |  |
| `ANT/HF multibands/OpenSleeve/Sleve14-21_C.maa` | A | 3 |  |  |
| `ANT/HF multibands/OpenSleeve/WARC open sleeve dip.maa` | A | 4 |  |  |
| `ANT/HF multibands/Ant+tuner/14-21-28 LCdipole.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/14-21LC quad.maa` | A | 5 |  |  |
| `ANT/HF multibands/Ant+tuner/14-21LCdipole.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/18-24LCdipole.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/7-14LC1.maa` | A | 5 |  |  |
| `ANT/HF multibands/Ant+tuner/7_14_LC.maa` | A | 4 |  |  |
| `ANT/HF multibands/Ant+tuner/7_18 vertical.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/DL2KQ15.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/DL2KQ20.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/DL2KQ40.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/DL2KQ80.maa` | A | 9 |  |  |
| `ANT/HF multibands/Ant+tuner/Delta 80-40 .maa` | A | 4 |  |  |
| `ANT/HF multibands/Ant+tuner/Delta160allbands.maa` | A | 7 |  |  |
| `ANT/HF multibands/Ant+tuner/Dipole.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/Dipole1.maa` | A | 2 |  |  |
| `ANT/HF multibands/Ant+tuner/EU1TTvert160.maa` | A | 12 |  |  |
| `ANT/HF multibands/Ant+tuner/EU1TTvert40.maa` | A | 12 |  |  |
| `ANT/HF multibands/Ant+tuner/EU1TTvert80.maa` | A | 12 |  |  |
| `ANT/HF multibands/Ant+tuner/Fat_dipole.maa` | A | 12 |  |  |
| `ANT/HF multibands/Ant+tuner/G5RV.maa` | A | 6 |  |  |
| `ANT/HF multibands/Ant+tuner/N4UFP17_12.maa` | A | 6 |  |  |
| `ANT/HF multibands/Ant+tuner/N4UFP30_17.maa` | A | 6 |  |  |
| `ANT/HF multibands/Ant+tuner/Short gp80-40.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/VertUA3SFH.maa` | A | 3 |  |  |
| `ANT/HF multibands/Ant+tuner/dip4010.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Ant+tuner/dip8010.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Ant+tuner/eu1tt160.maa` | A | 9 |  |  |
| `ANT/HF multibands/Ant+tuner/eu1tt30.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/eu1tt40.maa` | A | 9 |  |  |
| `ANT/HF multibands/Ant+tuner/eu1tt80.maa` | A | 8 |  |  |
| `ANT/HF multibands/Ant+tuner/n7rk_160.maa` | A | 11 |  |  |
| `ANT/HF multibands/Ant+tuner/n7rk_80.maa` | A | 11 |  |  |
| `ANT/HF multibands/Ant+tuner/ua1dz.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Ant+tuner/warc.maa` | B | 1 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/HF multibands/Ant+tuner/zs6bkw.maa` | A | 6 |  |  |
| `ANT/Receive/BEVERAGE.MAA` | A | 4 |  |  |
| `ANT/Receive/Beverage  0.1m.maa` | A | 4 |  |  |
| `ANT/Receive/Beverage-1+GP.maa` | A | 7 |  |  |
| `ANT/Receive/Beverage-1+Inv V.maa` | A | 8 |  |  |
| `ANT/Receive/Beverage-1.maa` | A | 6 |  |  |
| `ANT/Receive/Bidirectional Flag1.maa` | A | 11 |  |  |
| `ANT/Receive/Bidirectional Flag2.maa` | A | 11 |  |  |
| `ANT/Receive/Bidirectional pennant.maa` | A | 9 |  |  |
| `ANT/Receive/Cage_vertical_40.maa` | B | 39 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Receive/EWE.MAA` | A | 4 |  |  |
| `ANT/Receive/Flag K6SE+GP.maa` | A | 6 |  |  |
| `ANT/Receive/Flag K6SE.maa` | A | 5 |  |  |
| `ANT/Receive/Helix Beverage.maa` | A | 474 |  |  |
| `ANT/Receive/JF1DMQ.maa` | A | 5 |  |  |
| `ANT/Receive/K9AY.maa` | A | 5 |  |  |
| `ANT/Receive/L-Bev.maa` | A | 13 |  |  |
| `ANT/Receive/Pennant EA3VY.maa` | A | 5 | Yes |  |
| `ANT/Receive/TBev_dl2kq.maa` | A | 4 |  |  |
| `ANT/Receive/beverage180-NEC2.maa` | A | 6 |  |  |
| `ANT/Receive/beverage180.maa` | A | 4 |  |  |
| `ANT/Receive/bidir_beverage1.maa` | A | 7 |  |  |
| `ANT/Receive/bidir_beverage2.maa` | A | 7 |  |  |
| `ANT/Radiation of feeder/0.25lambda  symm balun.maa` | A | 8 |  |  |
| `ANT/Radiation of feeder/0.25lambda balun asym.maa` | A | 7 |  |  |
| `ANT/Radiation of feeder/0.25lambda balun.maa` | A | 7 |  |  |
| `ANT/Radiation of feeder/Asymm dipole - simm line.maa` | A | 6 |  |  |
| `ANT/Radiation of feeder/GP-1.maa` | A | 7 |  |  |
| `ANT/Radiation of feeder/GP-2.maa` | A | 11 |  |  |
| `ANT/Radiation of feeder/Opt_drossel.maa` | A | 4 |  |  |
| `ANT/Radiation of feeder/Parasitic currents.maa` | A | 14 |  |  |
| `ANT/Radiation of feeder/Symm line+ asymm TRX.maa` | A | 7 |  |  |
| `ANT/Radiation of feeder/asym  test -  LC trap.maa` | A | 4 |  |  |
| `ANT/Radiation of feeder/asymmetry test.maa` | A | 4 |  |  |
| `ANT/Radiation of feeder/end fire.maa` | A | 5 |  |  |
| `ANT/Radiation of feeder/symmetry test -  LC trap.maa` | A | 4 |  |  |
| `ANT/Radiation of feeder/symmetry test -1 RW3FO.maa` | A | 4 |  |  |
| `ANT/Radiation of feeder/symmetry test RW3FO.maa` | A | 4 |  |  |
| `ANT/VHF beams/12CQ430.MAA` | A | 49 |  |  |
| `ANT/VHF beams/12EL23CM.MAA` | A | 97 |  |  |
| `ANT/VHF beams/12EL430.MAA` | A | 16 |  |  |
| `ANT/VHF beams/144-5Yagi.maa` | A | 6 |  |  |
| `ANT/VHF beams/15EL23CM.MAA` | A | 16 |  |  |
| `ANT/VHF beams/3DQ6.MAA` | A | 13 |  |  |
| `ANT/VHF beams/3HENT.MAA` | A | 22 |  |  |
| `ANT/VHF beams/3el Quads.maa` | A | 13 |  |  |
| `ANT/VHF beams/4DELTA6.MAA` | A | 13 |  |  |
| `ANT/VHF beams/5CQ2.MAA` | A | 21 |  |  |
| `ANT/VHF beams/5ELTWIN.MAA` | A | 36 |  |  |
| `ANT/VHF beams/5ELTWIND.MAA` | A | 36 |  |  |
| `ANT/VHF beams/6EL6MW.MAA` | A | 7 |  |  |
| `ANT/VHF beams/6el Yagi.maa` | A | 7 |  |  |
| `ANT/VHF beams/7EL6M.MAA` | A | 8 |  |  |
| `ANT/VHF beams/8EL2MW.MAA` | A | 9 |  |  |
| `ANT/VHF beams/8EL6M.MAA` | A | 9 |  |  |
| `ANT/VHF beams/8EL6MW.MAA` | A | 9 |  |  |
| `ANT/VHF beams/Crosspol144.maa` | A | 10 |  |  |
| `ANT/VHF beams/J-Yagi 144.maa` | A | 12 |  |  |
| `ANT/VHF beams/SQ6M.MAA` | A | 30 |  |  |
| `ANT/VHF beams/TV Log Yagi VHF-UHF.maa` | A | 36 |  |  |
| `ANT/VHF beams/ZZ-NEW.MAA` | A | 14 |  |  |
| `ANT/VHF beams/dk7zb-2.maa` | A | 5 |  |  |
| `ANT/VHF beams/f9ft-2-uu4jcr.maa` | A | 17 |  |  |
| `ANT/VHF beams/f9ft-21-432.maa` | A | 25 |  |  |
| `ANT/VHF beams/hornYagi-2.maa` | A | 17 |  |  |
| `ANT/VHF beams/hornYagi-3.maa` | A | 13 |  |  |
| `ANT/VHF beams/hornYagi.maa` | A | 15 |  |  |
| `ANT/VHF beams/mirror.maa` | B | 313 |  | Per-section headers; wire count inside `***Wires***` block |
| `ANT/Aperiodic/ABW-R1.maa` | A | 23 |  |  |
| `ANT/Aperiodic/ABW1.maa` | A | 8 |  |  |
| `ANT/Aperiodic/ABW40.maa` | A | 6 |  |  |
| `ANT/Aperiodic/ABW80_10.maa` | A | 20 |  |  |
| `ANT/Aperiodic/Ap_quad.maa` | A | 7 |  |  |
| `ANT/Aperiodic/Apiram.maa` | A | 8 |  |  |
| `ANT/Aperiodic/D2T.maa` | A | 15 |  |  |
| `ANT/Aperiodic/Fractal line.maa` | A | 17 |  |  |
| `ANT/Aperiodic/Lu4-full.maa` | A | 150 |  |  |
| `ANT/Aperiodic/Lu4.maa` | A | 25 |  |  |
| `ANT/Aperiodic/PYRAM.maa` | A | 48 |  |  |
| `ANT/Aperiodic/RHOMBIC-2tower.maa` | A | 7 |  |  |
| `ANT/Aperiodic/RHOMBIC.MAA` | A | 7 |  |  |
| `ANT/Aperiodic/RHOMBIC_Braude.maa` | A | 27 |  |  |
| `ANT/Aperiodic/Rhombic1.maa` | A | 11 |  |  |
| `ANT/Aperiodic/Rhombic2.maa` | A | 11 |  |  |
| `ANT/Aperiodic/Semirhomb.maa` | A | 5 |  |  |
| `ANT/Aperiodic/T2FD.maa` | A | 5 |  |  |
| `ANT/Aperiodic/Travwave.maa` | A | 4 |  |  |
| `ANT/Aperiodic/WA2WVL.maa` | A | 4 |  |  |
| `ANT/Aperiodic/widebanddip.maa` | A | 34 |  |  |
| `TX_ANT/2CQ15L.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/2CQ20L.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/2CQ30L.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/2CQ40L.maa` | B | 8 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/2E17.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3-YAGI-10.maa` | A | 3 |  |  |
| `TX_ANT/3-YAGI-12.maa` | A | 3 |  |  |
| `TX_ANT/3-YAGI-15.maa` | A | 4 |  |  |
| `TX_ANT/3-YAGI-17.maa` | A | 4 |  |  |
| `TX_ANT/3-YAGI-20.maa` | A | 4 |  |  |
| `TX_ANT/3CQ10.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ10L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ10X.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ12L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ15.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ15L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ15X.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ17.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ17L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ20L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/3CQ6L.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/40m-loaded-vert.maa` | A | 10 |  |  |
| `TX_ANT/40m-unloaded-vert.maa` | B | 5 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/43ft-vert.maa` | B | 2 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ10.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ10L.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ10X.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ12L.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ15.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ15L.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/4CQ17L.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/5B-ROTDIP.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/6CQ6L.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/80m-loaded-vert.maa` | A | 22 |  |  |
| `TX_ANT/80m-unloaded-vert.maa` | B | 17 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/D4030.maa` | B | 10 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/D403017.maa` | B | 11 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/D80CW-40-30.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/DK7ZB-loop.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/HB9CV-10.maa` | B | 13 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/MB-2-YAGI-B-tower.maa` | A | 21 |  |  |
| `TX_ANT/MB-2-YAGI-B.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/MB-2-YAGI.maa` | A | 18 |  |  |
| `TX_ANT/OB11-3-T.maa` | A | 24 |  |  |
| `TX_ANT/OB11-3.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/OWA10.maa` | B | 3 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/TA7OM.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/YD1SDL.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |
| `TX_ANT/nested_loop.maa` | A | 25 |  |  |
| `TX_ANT/yagi-12-17.maa` | B | 9 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/2CQ15.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ10-CW.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ10.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ1015.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ10X.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ12-P1015.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ12.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ15-CW.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ15-P1217.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ15.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ17.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/3CQ20-CW.maa` | B | 12 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/CCCCCCCCCCCCC.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/CQ-PB-75.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/CQ2-3.maa` | A | 25 |  |  |
| `Latest/CQ2-4.maa` | A | 33 |  |  |
| `Latest/CQ23-4.maa` | B | 36 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/CQ6.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/DB1510.maa` | B | 16 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/MB-ing.maa` | A | 25 |  |  |
| `Latest/PB2017151210.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/QB17151210.maa` | B | 32 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/SP.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/TB.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/TB151210.maa` | B | 24 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/X.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/X2.maa` | B | 40 |  | Per-section headers; wire count inside `***Wires***` block |
| `Latest/multiband.maa` | B | 19 |  | Per-section headers; wire count inside `***Wires***` block |

How OpenNEC handles each variant
---------------------------------

The `read_deck_maa()` function in `mma-support.c` currently targets Variant A
and will successfully parse most Variant B files because it already skips
`***…***` header lines.  However the wire count in Variant B is the first
integer in the section, not a combined `nw nl ns` triplet, so the importer may
mis-read `ns` and `nl` as zero.  A follow-up improvement to the importer should
detect which variant a file uses and adjust the count-parsing accordingly.

Grammar revision
----------------

The "Minimal grammar" section of `MMA file format.md` should be updated to reflect both variants:

```
<MMA-A>  ::= <title>
             <frequency>
             <nw> SP <nl> SP <ns>          // combined counts
             {<wire>}^nw
             [<source-section>]
             [<load-section>]
             [<extra-sections>]*

<MMA-B>  ::= <title>
             [* NEWLINE]                   // optional single-asterisk line
             <frequency>
             "***Wires***" NEWLINE
             <nw> NEWLINE
             {<wire>}^nw
             [<source-section-B>]
             [<load-section-B>]
             [<extra-sections>]*

<source-section>   ::= "***Source***" NEWLINE <ns> ["," <0>] NEWLINE {<src-A>}^ns
<source-section-B> ::= "***Source***" NEWLINE <ns> "," <0> NEWLINE {<src-B>}^ns
<src-A>            ::= <wire-tag> SP <seg> SP <mag> SP <phase>
<src-B>            ::= "w" <tag> "c" "," <phase> "," <mag>  // polar, wire-tag label

<extra-sections>   ::= "***Segmentation***" NEWLINE <seg-params>
                     | "***G/H/M/R/AzEl/X***" NEWLINE <ground-params>
                     | "###Comment###" [SP <text> | NEWLINE <text>]
```

