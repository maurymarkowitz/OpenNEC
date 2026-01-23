/******************************************************************************
 * control.c
 *
 * Control card processing for OpenNEC. This module processes the control
 * cards (FR, LD, GN, EX, NT, TL, XQ, GD, RP, NX, PT, KH, NE, NH, PQ, EK, 
 * CP, PL, EN) that configure the calculation parameters in the context.
 *
 * This is extracted from the old main.c card input loop (lines 365-940).
 *
 *****************************************************************************/

#include "opennec.h"

/******************************************************************************
 * nec_calculation_defaults()
 *
 * Initialize calculation defaults that depend on geometry being calculated first.
 * This should be called after calculate_geometry() and before process_control_cards().
 *
 * Sets geometry-dependent values and resets all calculation parameters to their
 * initial state to support multiple calculation runs.
 *
 * @param ctx     The NEC context to initialize
 * @return        0 on success, -1 on error (no geometry)
 */
int nec_calculation_defaults(nec_context_t *ctx)
{
    // Validate that geometry has been calculated
    if (ctx->geometry.np <= 0) {
        return -1;
    }
    
    // Set geometry-dependent matrix parameters
    ctx->netcx.npeq = ctx->geometry.np + 2 * ctx->geometry.mp;
    
    // Matrix parameters (from oldmain.c lines 289-292)
    if (ctx->matpar.imat == 0) {
        ctx->netcx.neq = ctx->geometry.n + 2 * ctx->geometry.m;
        ctx->netcx.neq2 = 0;
    }
    
    // Reset all calculation defaults to initial values
    // These are reset for each run to support multiple calculations
    ctx->fpat.ixtyp = 0;
    ctx->fpat.near = -1;
    ctx->zload.nload = 0;
    ctx->netcx.nonet = 0;
    ctx->plot.iplp1 = 0;
    ctx->plot.iplp2 = 0;
    ctx->plot.iplp3 = 0;
    ctx->plot.iplp4 = 0;
    ctx->yparm.ncoup = 0;
    ctx->yparm.icoup = 0;
    ctx->gnd.iperf = 0;
    ctx->gnd.nradl = 0;
    ctx->dataj.rkh = 1.0;  // Default matrix integration limit
    ctx->dataj.iexk = 0;   // Extended thin-wire kernel off by default
    ctx->gnd.ifar = -1;
    
    // Note: The following old main.c local variables are not stored in ctx
    // as they were only used for local flow control:
    //   igo    - execution flow control flag
    //   nfrq   - frequency loop counter
    //   rkh    - wave number parameter (k*h)
    //   iexk   - extended thin-wire kernel flag
    //   iped   - impedance print flag
    //   iptflg - pattern output control flag
    //   iptflq - pattern output control flag
    //   mpcnt  - command card counter
    
    return 0;
}

/******************************************************************************
 * process_control_cards()
 *
 * Process control cards from the deck to set up calculation parameters in ctx.
 * This should be called after calculate_geometry() and nec_calculation_defaults().
 * 
 * Iterates over all cards after the GE (geometry end) card and processes
 * the control cards (FR, LD, GN, EX, NT, TL, etc.) to configure the context
 * for the calculation.
 *
 * @param ctx     The NEC context to configure
 * @param deck    The deck containing the control cards
 * @return        0 on success, -1 on error
 */
int process_control_cards(nec_context_t *ctx, deck_t *deck)
{
    // Validate inputs
    if (ctx == NULL || deck == NULL) {
        return -1;
    }
    
    if (deck->geometry_end < 0) {
        add_error(ctx, &ctx->errors, "No GE (geometry end) card found", FATAL);
        return -1;
    }
    
    // Start processing cards after the GE card
    int start_idx = deck->geometry_end + 1;
    
    for (int card_idx = start_idx; card_idx < deck->num_cards; card_idx++) {
        card_t *card = &deck->cards[card_idx];
        
        // Skip ignored, comment, or empty cards
        if (card->ignore || is_comment(card)) {
            continue;
        }
        
        // Get the card code
        char *code = card->card_code;
        
        // Get field values for convenience
        int i1 = card->iv[1], i2 = card->iv[2], i3 = card->iv[3], i4 = card->iv[4];
        double f1 = card->fv[1], f2 = card->fv[2], f3 = card->fv[3];
        double f4 = card->fv[4], f5 = card->fv[5], f6 = card->fv[6];
        
        // Process based on card type
        if (strcmp(code, "FR") == 0) {
            // FR card - Frequency specification
            // Format: FR ifrq nfrq - - fmhz delfrq - -
            ctx->save.ifrq = i1;
            ctx->save.nfrq = (i2 == 0) ? 1 : i2;
            ctx->save.fmhz = f1;
            ctx->save.delfrq = f2;
        }
        else if (strcmp(code, "LD") == 0) {
            // LD card - Loading
            if (i1 == -1) {
                continue; // Skip this card
            }
            
            // Reallocate loading buffers
            ctx->zload.nload++;
            size_t mreq = (size_t)ctx->zload.nload * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->zload.ldtyp, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtag, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtagf, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtagt, mreq);
            
            mreq = (size_t)ctx->zload.nload * sizeof(double);
            mem_realloc(ctx, (void **)&ctx->zload.zlr, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.zli, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.zlc, mreq);
            
            int idx = ctx->zload.nload - 1;
            ctx->zload.ldtyp[idx] = i1;
            ctx->zload.ldtag[idx] = i2;
            ctx->zload.ldtagf[idx] = (i4 == 0) ? i3 : i3;
            ctx->zload.ldtagt[idx] = (i4 == 0) ? i3 : i4;
            
            if (ctx->zload.ldtagt[idx] < ctx->zload.ldtagf[idx]) {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "DATA FAULT ON LOADING CARD No: %d: ITAG "
                    "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
                    ctx->zload.nload, i3, i4);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            
            ctx->zload.zlr[idx] = f1;
            ctx->zload.zli[idx] = f2;
            ctx->zload.zlc[idx] = f3;
        }
        else if (strcmp(code, "GN") == 0) {
            // GN card - Ground parameters
            if (i1 == -1) {
                ctx->gnd.ksymp = 1;
                ctx->gnd.nradl = 0;
                ctx->gnd.iperf = 0;
                continue;
            }
            
            ctx->gnd.iperf = i1;
            ctx->gnd.nradl = i2;
            ctx->gnd.ksymp = 2;
            ctx->save.epsr = f1;
            ctx->save.sig = f2;
            
            if (ctx->gnd.nradl != 0) {
                if (ctx->gnd.iperf == 2) {
                    add_error(ctx, &ctx->errors,
                        "RADIAL WIRE G.S. APPROXIMATION MAY "
                        "NOT BE USED WITH SOMMERFELD GROUND OPTION", FATAL);
                    return -1;
                }
                ctx->save.scrwlt = f3;
                ctx->save.scrwrt = f4;
                continue;
            }
            
            ctx->fpat.epsr2 = f3;
            ctx->fpat.sig2 = f4;
            ctx->fpat.clt = f5;
            ctx->fpat.cht = f6;
        }
        else if (strcmp(code, "EX") == 0) {
            // EX card - Excitation
            ctx->fpat.ixtyp = i1;
            ctx->netcx.masym = i4 / 10;
            
            // For voltage source types (0 and 5)
            if (i1 == 0 || i1 == 5) {
                ctx->netcx.ntsol = 0;
                
                if (i1 == 5) {
                    // Incident plane wave or elementary current source
                    ctx->vsorc.nvqd++;
                    size_t mreq = (size_t)ctx->vsorc.nvqd * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.ivqd, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.iqds, mreq);
                    
                    mreq = (size_t)ctx->vsorc.nvqd * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vqd, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vqds, mreq);
                    
                    int idx = ctx->vsorc.nvqd - 1;
                    ctx->vsorc.ivqd[idx] = segment_number(ctx, i2, i3);
                    ctx->vsorc.vqd[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.vqd[idx]) < 1.e-20) {
                        ctx->vsorc.vqd[idx] = CPLX_10;
                    }
                } else {
                    // Applied voltage source
                    ctx->vsorc.nsant++;
                    size_t mreq = (size_t)ctx->vsorc.nsant * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.isant, mreq);
                    
                    mreq = (size_t)ctx->vsorc.nsant * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vsant, mreq);
                    
                    int idx = ctx->vsorc.nsant - 1;
                    ctx->vsorc.isant[idx] = segment_number(ctx, i2, i3);
                    ctx->vsorc.vsant[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.vsant[idx]) < 1.e-20) {
                        ctx->vsorc.vsant[idx] = CPLX_10;
                    }
                }
            } else {
                // Far field pattern for receiving antenna
                ctx->fpat.xpr6 = f6;
                ctx->vsorc.nsant = 0;
                ctx->vsorc.nvqd = 0;
            }
        }
        else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
            // NT/TL cards - Network parameters
            if (i2 == -1) {
                continue;
            }
            
            // Reallocate network buffers
            ctx->netcx.nonet++;
            size_t mreq = (size_t)ctx->netcx.nonet * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->netcx.ntyp, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.iseg1, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.iseg2, mreq);
            
            mreq = (size_t)ctx->netcx.nonet * sizeof(double);
            mem_realloc(ctx, (void **)&ctx->netcx.x11r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x11i, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x12r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x12i, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x22r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x22i, mreq);
            
            int idx = ctx->netcx.nonet - 1;
            if (strcmp(code, "NT") == 0) {
                ctx->netcx.ntyp[idx] = 1;
            } else {
                ctx->netcx.ntyp[idx] = 2;
            }
            
            ctx->netcx.iseg1[idx] = segment_number(ctx, i1, i2);
            ctx->netcx.iseg2[idx] = segment_number(ctx, i3, i4);
            ctx->netcx.x11r[idx] = f1;
            ctx->netcx.x11i[idx] = f2;
            ctx->netcx.x12r[idx] = f3;
            ctx->netcx.x12i[idx] = f4;
            ctx->netcx.x22r[idx] = f5;
            ctx->netcx.x22i[idx] = f6;
            
            // Check for transmission line with impedance
            if ((ctx->netcx.ntyp[idx] == 2) && (f1 <= 0.0)) {
                ctx->netcx.ntyp[idx] = 3;
                ctx->netcx.x11r[idx] = -f1;
            }
        }
        else if (strcmp(code, "XQ") == 0) {
            // XQ card - Execute (calculate with radiated fields)
            if (i1 != 0) {
                // XQ just signals execute - radiation pattern setup comes from RP card
                // Don't set ctx->gnd.ifar = 0 here
                ctx->fpat.rfld = 0.0;
                ctx->fpat.ipd = 0;
                ctx->fpat.iavp = 0;
                ctx->fpat.inor = 0;
                ctx->fpat.iax = 0;
                ctx->fpat.nth = 91;
                ctx->fpat.nph = 1;
                ctx->fpat.thets = 0.0;
                ctx->fpat.phis = 0.0;
                ctx->fpat.dth = 1.0;
                ctx->fpat.dph = 0.0;
                
                if (i1 == 2) {
                    ctx->fpat.phis = 90.0;
                }
                if (i1 == 3) {
                    ctx->fpat.nph = 2;
                    ctx->fpat.dph = 90.0;
                }
            }
        }
        else if (strcmp(code, "GD") == 0) {
            // GD card - Ground representation (for patterns)
            ctx->fpat.epsr2 = f1;
            ctx->fpat.sig2 = f2;
            ctx->fpat.clt = f3;
            ctx->fpat.cht = f4;
        }
        else if (strcmp(code, "RP") == 0) {
            // RP card - Radiation pattern parameters
            ctx->gnd.ifar = i1;
            ctx->fpat.nth = (i2 == 0) ? 1 : i2;
            ctx->fpat.nph = (i3 == 0) ? 1 : i3;
            
            ctx->fpat.ipd = i4 / 10;
            ctx->fpat.iavp = i4 - ctx->fpat.ipd * 10;
            ctx->fpat.inor = ctx->fpat.ipd / 10;
            ctx->fpat.ipd = ctx->fpat.ipd - ctx->fpat.inor * 10;
            ctx->fpat.iax = ctx->fpat.inor / 10;
            ctx->fpat.inor = ctx->fpat.inor - ctx->fpat.iax * 10;
            
            if (ctx->fpat.iax != 0) ctx->fpat.iax = 1;
            if (ctx->fpat.ipd != 0) ctx->fpat.ipd = 1;
            if ((ctx->fpat.nth < 2) || (ctx->fpat.nph < 2) || (ctx->gnd.ifar == 1)) {
                ctx->fpat.iavp = 0;
            }
            
            ctx->fpat.thets = f1;
            ctx->fpat.phis = f2;
            ctx->fpat.dth = f3;
            ctx->fpat.dph = f4;
            ctx->fpat.rfld = f5;
            ctx->fpat.gnor = f6;
        }
        else if (strcmp(code, "NX") == 0) {
            // NX card - Next job (not typically used in single-run mode)
            // For now, just note it and continue
            continue;
        }
        else if (strcmp(code, "KH") == 0) {
            // KH card - Matrix integration limit
            // This affects matrix computation accuracy
            // Sets rkh which controls approximate vs. exact integration threshold
            ctx->dataj.rkh = f1;
            continue;
        }
        else if (strcmp(code, "NE") == 0 || strcmp(code, "NH") == 0) {
            // NE/NH cards - Near field calculation
            ctx->fpat.nfeh = (strcmp(code, "NH") == 0) ? 1 : 0;
            ctx->fpat.near = i1;
            ctx->fpat.nrx = i2;
            ctx->fpat.nry = i3;
            ctx->fpat.nrz = i4;
            ctx->fpat.xnr = f1;
            ctx->fpat.ynr = f2;
            ctx->fpat.znr = f3;
            ctx->fpat.dxnr = f4;
            ctx->fpat.dynr = f5;
            ctx->fpat.dznr = f6;
        }
        else if (strcmp(code, "PT") == 0) {
            // PT card - Print control for current
            // Note: iptflg, iptag, iptagf, iptagt were local variables
            // These controlled printing, not calculation
            continue;
        }
        else if (strcmp(code, "PQ") == 0) {
            // PQ card - Print control for charge
            // Note: iptflq, iptaq, iptaqf, iptaqt were local variables
            continue;
        }
        else if (strcmp(code, "EK") == 0) {
            // EK card - Extended thin-wire kernel option
            // Note: iexk was a local variable, controlled kernel selection
            continue;
        }
        else if (strcmp(code, "CP") == 0) {
            // CP card - Maximum coupling between antennas
            if (i2 == 0) {
                continue;
            }
            
            ctx->yparm.icoup = 0;
            
            // First antenna
            ctx->yparm.ncoup++;
            size_t mreq = (size_t)ctx->yparm.ncoup * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->yparm.nctag, mreq);
            mem_realloc(ctx, (void **)&ctx->yparm.ncseg, mreq);
            ctx->yparm.nctag[ctx->yparm.ncoup - 1] = i1;
            ctx->yparm.ncseg[ctx->yparm.ncoup - 1] = i2;
            
            // Second antenna (if specified)
            if (i4 != 0) {
                ctx->yparm.ncoup++;
                mreq = (size_t)ctx->yparm.ncoup * sizeof(int);
                mem_realloc(ctx, (void **)&ctx->yparm.nctag, mreq);
                mem_realloc(ctx, (void **)&ctx->yparm.ncseg, mreq);
                ctx->yparm.nctag[ctx->yparm.ncoup - 1] = i3;
                ctx->yparm.ncseg[ctx->yparm.ncoup - 1] = i4;
            }
        }
        else if (strcmp(code, "PL") == 0) {
            // PL card - Plot flags
            ctx->plot.iplp1 = i1;
            ctx->plot.iplp2 = i2;
            ctx->plot.iplp3 = i3;
            ctx->plot.iplp4 = i4;
        }
        else if (strcmp(code, "EN") == 0) {
            // EN card - End of deck
            break;
        }
        else if (strcmp(code, "WG") == 0) {
            // WG card - Not supported
            add_error(ctx, &ctx->errors, "WG CARD NOT SUPPORTED", FATAL);
            return -1;
        }
    }
    
    return 0;
}

/******************************************************************************
 * execute_frequency_loop()
 *
 * Execute the main frequency loop calculations. This is the core computation
 * that performs matrix fill/factor, network calculations, and field calculations
 * for each frequency point.
 * 
 * This replaces the old main.c frequency do loop (lines 945-1862).
 * Output formatting has been factored out to output.c functions.
 *
 * @param ctx     The NEC context with all calculation parameters
 * @param nfrq    Number of frequency points to calculate
 * @param ifrq    Frequency step type (0=linear, 1=multiplicative)
 * @param delfrq  Frequency step size
 * @return        0 on success, -1 on error
 */
int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq)
{
    if (ctx == NULL) {
        return -1;
    }
    
    // Validate geometry exists
    if (ctx->netcx.neq == 0 || ctx->netcx.npeq == 0) {
        add_error(ctx, &ctx->errors, "Geometry not initialized before frequency loop", FATAL);
        return -1;
    }
    
    if (ctx->geometry.n > 0 && (ctx->geometry.icon1 == NULL || ctx->geometry.icon2 == NULL)) {
        add_error(ctx, &ctx->errors, "Geometry connection data not allocated", FATAL);
        return -1;
    }
    
    // Allocate memory for interaction matrix and IP array
    size_t iresrv = ctx->netcx.neq * (ctx->netcx.neq + 2);
    size_t mreq = iresrv * sizeof(complex double);
    complex double *cm = NULL;
    mem_alloc(ctx, (void **)&cm, mreq);
    
    mreq = ctx->netcx.neq * sizeof(int);
    mem_alloc(ctx, (void **)&ctx->save.ip, mreq);
    
    // Allocate symmetry array
    ctx->smat.nop = ctx->netcx.neq / ctx->netcx.npeq;
    mreq = (size_t)(ctx->smat.nop * ctx->smat.nop) * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->smat.ssx, mreq);
    
    // Allocate current array
    mreq = (size_t)ctx->geometry.np3m * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->crnt.cur, mreq);
    
    // Allocate current basis function coefficient arrays
    mreq = (size_t)ctx->geometry.npm * sizeof(double);
    mem_alloc(ctx, (void **)&ctx->crnt.air, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.aii, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.bir, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.bii, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.cir, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.cii, mreq);
    
    // Save unscaled geometry for frequency scaling
    double *xtemp = NULL, *ytemp = NULL, *ztemp = NULL;
    double *sitemp = NULL, *bitemp = NULL;
    
    if (ctx->geometry.n > 0) {
        mreq = (ctx->geometry.n + ctx->geometry.m) * sizeof(double);
        mem_alloc(ctx, (void **)&xtemp, mreq);
        mem_alloc(ctx, (void **)&ytemp, mreq);
        mem_alloc(ctx, (void **)&ztemp, mreq);
        mem_alloc(ctx, (void **)&sitemp, mreq);
        mem_alloc(ctx, (void **)&bitemp, mreq);
        
        // Save wire geometry
        for (int i = 0; i < ctx->geometry.n; i++) {
            xtemp[i] = ctx->geometry.x[i];
            ytemp[i] = ctx->geometry.y[i];
            ztemp[i] = ctx->geometry.z[i];
            sitemp[i] = ctx->geometry.si[i];
            bitemp[i] = ctx->geometry.bi[i];
        }
        
        // Save patch geometry
        if (ctx->geometry.m > 0) {
            for (int i = 0; i < ctx->geometry.m; i++) {
                int j = i + ctx->geometry.n;
                xtemp[j] = ctx->geometry.px[i];
                ytemp[j] = ctx->geometry.py[i];
                ztemp[j] = ctx->geometry.pz[i];
                bitemp[j] = ctx->geometry.pbi[i];
            }
        }
    }
    
    // Perform fblock matrix setup if needed
    if (ctx->matpar.imat == 0) {
        fblock(ctx, ctx->netcx.npeq, ctx->netcx.neq, iresrv, ctx->geometry.ipsym);
    }
    
    // Frequency loop
    for (int mhz = 1; mhz <= nfrq; mhz++) {
        // Update frequency
        if (mhz > 1) {
            if (ifrq == 1) {
                ctx->save.fmhz *= delfrq;
            } else {
                ctx->save.fmhz += delfrq;
            }
        }
        
        // Calculate wavelength and frequency ratio
        double fr = ctx->save.fmhz / CVEL;
        ctx->geometry.wlam = CVEL / ctx->save.fmhz;
        
        // Scale geometry to current frequency
        if (ctx->geometry.n > 0) {
            for (int i = 0; i < ctx->geometry.n; i++) {
                ctx->geometry.x[i] = xtemp[i] * fr;
                ctx->geometry.y[i] = ytemp[i] * fr;
                ctx->geometry.z[i] = ztemp[i] * fr;
                ctx->geometry.si[i] = sitemp[i] * fr;
                ctx->geometry.bi[i] = bitemp[i] * fr;
            }
        }
        
        if (ctx->geometry.m > 0) {
            double fr2 = fr * fr;
            for (int i = 0; i < ctx->geometry.m; i++) {
                int j = i + ctx->geometry.n;
                ctx->geometry.px[i] = xtemp[j] * fr;
                ctx->geometry.py[i] = ytemp[j] * fr;
                ctx->geometry.pz[i] = ztemp[j] * fr;
                ctx->geometry.pbi[i] = bitemp[j] * fr2;
            }
        }
        
        // Apply loading to structure
        if (ctx->zload.nload > 0) {
            int *ldtyp = ctx->zload.ldtyp;
            int *ldtag = ctx->zload.ldtag;
            int *ldtagf = ctx->zload.ldtagf;
            int *ldtagt = ctx->zload.ldtagt;
            double *zlr = ctx->zload.zlr;
            double *zli = ctx->zload.zli;
            double *zlc = ctx->zload.zlc;
            
            if (load(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc) != 0)
                return -1;
        }
        
        // Set up ground parameters
        if (ctx->gnd.ksymp != 1) {
            ctx->gnd.frati = CPLX_10;
            
            if (ctx->gnd.iperf != 1) {
                double sig = ctx->save.sig;
                if (sig < 0.0) {
                    sig = -sig / (59.96 * ctx->geometry.wlam);
                    ctx->save.sig = sig;
                }
                
                complex double epsc = ctx->save.epsr - I * sig * ctx->geometry.wlam * 59.96;
                ctx->gnd.zrati = 1.0 / csqrt(epsc);
                ctx->gwav.u = ctx->gnd.zrati;
                ctx->gwav.u2 = ctx->gwav.u * ctx->gwav.u;
                
                // Handle radial wire ground screen
                if (ctx->gnd.nradl != 0) {
                    ctx->gnd.scrwl = ctx->save.scrwlt / ctx->geometry.wlam;
                    ctx->gnd.scrwr = ctx->save.scrwrt / ctx->geometry.wlam;
                    ctx->gnd.t1 = CPLX_01 * 2367.067 / (double)ctx->gnd.nradl;
                    ctx->gnd.t2 = ctx->gnd.scrwr * (double)ctx->gnd.nradl;
                }
                
                // Use Sommerfeld ground solution if requested
                if (ctx->gnd.iperf == 2) {
                    somnec(ctx, ctx->save.epsr, ctx->save.sig, ctx->save.fmhz);
                    ctx->gnd.frati = (epsc - 1.0) / (epsc + 1.0);
                }
            }
        }
        
        // Fill and factor primary interaction matrix
        double tim1, tim2;
        secnds(ctx, &tim1);
        cmset(ctx, ctx->netcx.neq, cm, ctx->dataj.rkh, ctx->dataj.iexk);
        secnds(ctx, &tim2);
        ctx->mat_fill_time = tim2 - tim1;
        
        factrs(ctx, ctx->netcx.npeq, ctx->netcx.neq, cm, ctx->save.ip);
        secnds(ctx, &tim1);
        ctx->mat_factor_time = tim1 - tim2;
        
        // Reset solution counter
        ctx->netcx.ntsol = 0;
        ctx->netcx.nprint = 0;
        
        // Set up excitation and solve
        // For voltage source excitation (most common case)
        if (ctx->fpat.ixtyp == 0 || ctx->fpat.ixtyp == 5) {
            // Fill right-hand side matrix (excitation)
            etmns(ctx, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, ctx->fpat.ixtyp, ctx->crnt.cur);
            
            // Solve with network
            network(ctx, cm, ctx->save.ip, ctx->crnt.cur);
            ctx->netcx.ntsol = 1;
            
            // Calculate power loss in structure
            ctx->fpat.ploss = 0.0;
            if (ctx->geometry.n > 0) {
                for (int i = 0; i < ctx->geometry.n; i++) {
                    complex double curi = ctx->crnt.cur[i] * ctx->geometry.wlam;
                    double cmag = cabs(curi);
                    
                    if (ctx->zload.nload > 0 && fabs(creal(ctx->zload.zarray[i])) >= 1.e-20) {
                        ctx->fpat.ploss += 0.5 * cmag * cmag * 
                                          creal(ctx->zload.zarray[i]) * ctx->geometry.si[i];
                    }
                }
            }
            
            // Handle coupling calculations if requested
            if (ctx->yparm.ncoup > 0) {
                couple(ctx, ctx->crnt.cur, ctx->geometry.wlam);
            }
            
            // Near field calculation if requested
            if (ctx->fpat.near != -1) {
                nfpat(ctx);
                if (mhz == nfrq) {
                    ctx->fpat.near = -1;
                }
            }
            
            // Store data for radiation pattern output (calculation happens in output.c)
            if (ctx->gnd.ifar != -1) {
                ctx->fpat.pinr = ctx->netcx.pin;
                ctx->fpat.pnlr = ctx->netcx.pnls;
                rdpat(ctx);
            }
        }
    }
    
    // Free temporary arrays
    mem_free(ctx, (void **)&cm);
    if (xtemp != NULL) {
        mem_free(ctx, (void **)&xtemp);
        mem_free(ctx, (void **)&ytemp);
        mem_free(ctx, (void **)&ztemp);
        mem_free(ctx, (void **)&sitemp);
        mem_free(ctx, (void **)&bitemp);
    }
    
    return 0;
}

