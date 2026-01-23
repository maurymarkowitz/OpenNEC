/*
 * Function prototypes for onec
 */

#include "types.h"

/*------------------------------------------------------------------------*/
/* main.c */
int main(int argc, char **argv);
void prnt(nec_context_t *ctx, int in1, int in2, int in3, double fl1, double fl2, double fl3, double fl4, double fl5, double fl6, char *ia, int ichar);
/* input.c */
void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile);
int read_line(nec_context_t *ctx, char *buff, FILE *pfile);
void parse_deck(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
void parse_comment_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
void parse_geometry_or_control_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
void parse_onec_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
void parse_key_values(nec_context_t *ctx, card_t *card, errors_list_t *errors);
/* control.c */
int nec_run_simulation(nec_context_t *ctx, deck_t *deck);
int nec_calculation_defaults(nec_context_t *ctx);
int process_control_cards(nec_context_t *ctx, deck_t *deck);
int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq);
/* geometry.c */
void calculate_geometry(nec_context_t *ctx, deck_t *deck, errors_list_t *errors, outputs_list_t *outputs);
/* output.c */
void write_deck_onec(nec_context_t *ctx, deck_t *deck, FILE *pfile);
void write_nec_output(nec_context_t *ctx, deck_t *deck, FILE *pfile);
void write_header(nec_context_t *ctx, deck_t *deck, FILE *pfile);
int write_structure(nec_context_t *ctx, deck_t *deck, FILE *pfile);
int write_segments(nec_context_t *ctx, deck_t *deck, FILE *pfile);
void write_patches(nec_context_t *ctx, deck_t * deck, FILE *pfile);
void write_input_cards(FILE *file, deck_t *deck);
void write_frequency_data(FILE *file, nec_context_t *ctx);
void write_loading_data(FILE *file, nec_context_t *ctx);
void write_environment_data(FILE *file, nec_context_t *ctx);
void write_matrix_timing(FILE *file, nec_context_t *ctx);
void write_network_data(FILE *file, nec_context_t *ctx);
void write_matrix_asymmetry(FILE *file, nec_context_t *ctx);
void write_network_excitation(FILE *file, nec_context_t *ctx);
void write_antenna_input_parameters(FILE *file, nec_context_t *ctx);
void write_currents(FILE *file, nec_context_t *ctx);
void write_power_budget(FILE *file, nec_context_t *ctx);
void write_radiation_pattern_header(FILE *file, nec_context_t *ctx);
void write_radiation_pattern_data(FILE *file, nec_context_t *ctx);
void write_average_power_gain(FILE *file, nec_context_t *ctx);
void write_normalized_gain(FILE *file, nec_context_t *ctx);
void write_footer(FILE *file, nec_context_t *ctx, deck_t *deck);
/* deck.c */
// these methods work on updating the processed values in the deck and cards
void update_deck_values(deck_t *deck);
void update_card_values(card_t *card);
void add_key_value(const card_t *card, key_value_t *list, char *key, char *value, char separator);
// these methods work on identifying card types and their field counts,
// they are read-only and do not need to be thread safe
int is_comment(const card_t *card);
int is_geometry(const card_t *card);
int is_control(const card_t *card);
int is_extension(const card_t *card);
int min_int_fields(const card_t *card);
int max_int_fields(const card_t *card);
int min_flt_fields(const card_t *card);
int max_flt_fields(const card_t *card);
/* test.c */
// various tests on the deck structure
void test_deck_structure(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
void test_duplicate_tags(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
void test_card_inputs(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
/* misc.c */
// all sorts of bits and pieces
void add_error(nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
void add_message(nec_context_t *ctx, outputs_list_t *outputs, char *message);
int str_ends_with(nec_context_t *ctx, const char *str, const char *suffix);
char* substr(nec_context_t *ctx, char* dest, char *src, int start, int len);
char* trim_start(nec_context_t *ctx, char* dest);
char* trim_end(nec_context_t *ctx, char* dest);
void abort_on_error(nec_context_t *ctx, int why);
void secnds(nec_context_t *ctx, double *x);
int stop(nec_context_t *ctx, int flag);  // Only called from main.c - errors centralized to ctx->errors
int mem_alloc(nec_context_t *ctx, void **ptr, size_t req);
int mem_realloc(nec_context_t *ctx, void **ptr, size_t req);
void mem_free(nec_context_t *ctx, void **ptr);
/* calculations.c */
void cabc(nec_context_t *ctx, complex double *curx);
void couple(nec_context_t *ctx, complex double *cur, double wlam);
int load(nec_context_t *ctx, int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt, double *zlr, double *zli, double *zlc);
void gf(nec_context_t *ctx, double zk, double *co, double *si);
double db10(nec_context_t *ctx, double x);
double db20(nec_context_t *ctx, double x);
void intrp(nec_context_t *ctx, double x, double y, complex double *f1, complex double *f2, complex double *f3, complex double *f4);
void intx(nec_context_t *ctx, double el1, double el2, double b, int ij, double *sgr, double *sgi);
int min(nec_context_t *ctx, int a, int b);
void test(nec_context_t *ctx, double f1r, double f2r, double *tr, double f1i, double f2i, double *ti, double dmin);
int sbf(nec_context_t *ctx, int i, int is, double *aa, double *bb, double *cc);
int tbf(nec_context_t *ctx, int i, int icap);
int trio(nec_context_t *ctx, int j);
void zint(nec_context_t *ctx, double sigl, double rolam, complex double *zt);
double cang(nec_context_t *ctx, complex double z);
/* fields.c */
void efld(nec_context_t *ctx, double xi, double yi, double zi, double ai, int ij);
void eksc(nec_context_t *ctx, double s, double z, double rh, double xk, int ij, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void ekscx(nec_context_t *ctx, double bx, double s, double z, double rhx, double xk, int ij, int inx1, int inx2, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void gh(nec_context_t *ctx, double zk, double *hr, double *hi);
void gwave(nec_context_t *ctx, complex double *erv, complex double *ezv, complex double *erh, complex double *ezh, complex double *eph);
void gx(nec_context_t *ctx, double zz, double rh, double xk, complex double *gz, complex double *gzp);
void gxx(nec_context_t *ctx, double zz, double rh, double a, double a2, double xk, int ira, complex double *g1, complex double *g1p, complex double *g2, complex double *g2p, complex double *g3, complex double *gzp);
void hfk(nec_context_t *ctx, double el1, double el2, double rhk, double zpkx, double *sgr, double *sgi);
void hintg(nec_context_t *ctx, double xi, double yi, double zi);
void hsfld(nec_context_t *ctx, double xi, double yi, double zi, double ai);
void hsflx(nec_context_t *ctx, double s, double rh, double zpx, complex double *hpk, complex double *hps, complex double *hpc);
void nefld(nec_context_t *ctx, double xob, double yob, double zob, complex double *ex, complex double *ey, complex double *ez);
void nfpat(nec_context_t *ctx);
void nhfld(nec_context_t *ctx, double xob, double yob, double zob, complex double *hx, complex double *hy, complex double *hz);
void pcint(nec_context_t *ctx, double xi, double yi, double zi, double cabi, double sabi, double salpi, complex double *e);
void unere(nec_context_t *ctx, double xob, double yob, double zob);
/* geometry.c */
int segment_number(nec_context_t *ctx, int tag, int m);
void calculate_geometry(nec_context_t *ctx, deck_t *deck, errors_list_t *errors, outputs_list_t *outputs);
void finish_geometry(nec_context_t *ctx);
int connect_segments(nec_context_t *ctx, int ignd, outputs_list_t *outputs);
void wire(nec_context_t *ctx, int card_num, int tag_num, int segs, double xw1, double yw1, double zw1, double xw2, double yw2, double zsw2, double rad, double rdel, double rrad);
void arc(nec_context_t *ctx, int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad);
void helix(nec_context_t *ctx, int card_num, int tag_num, int segs, double s, double hl, double a1, double b1, double a2, double b2, double rad, outputs_list_t *outputs);
void patch(nec_context_t *ctx, int card_num, int nx, int ny, double ax1, double ay1, double az1, double ax2, double ay2, double az2, double ax3, double ay3, double az3, double ax4, double ay4, double az4);
void calculate_patch(nec_context_t *ctx, int nx, int ny);
void reproduce(nec_context_t *ctx, double rox, double roy, double roz, double xs, double ys, double zs, int its, int nrpt, int itgi);
void reflect(nec_context_t *ctx, int card_num, int tag_increment, int ix, int iy, int iz);
void rotate(nec_context_t *ctx, int card_num, int tag_increment, int num_copies);
void scale(nec_context_t *ctx, double xw1);
void qdsrc(nec_context_t *ctx, int is, complex double v, complex double *e);
/* ground.c */
int rom2(nec_context_t *ctx, double a, double b, complex double *sum, double dmin);
void sflds(nec_context_t *ctx, double t, complex double *e);
/* matrix.c */
void cmset(nec_context_t *ctx, int nrow, complex double *cm, double rkhx, int iexkx);
void cmss(nec_context_t *ctx, int j1, int j2, int im1, int im2, complex double *cm, int nrow, int itrp);
void cmsw(nec_context_t *ctx, int j1, int j2, int i1, int i2, complex double *cm, complex double *cw, int ncw, int nrow, int itrp);
void cmws(nec_context_t *ctx, int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int itrp);
void cmww(nec_context_t *ctx, int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int nw, int itrp);
void etmns(nec_context_t *ctx, double p1, double p2, double p3, double p4, double p5, double p6, int ipr, complex double *e);
void factr(nec_context_t *ctx, int n, complex double *a, int *ip, int ndim);
void factrs(nec_context_t *ctx, int np, int nrow, complex double *a, int *ip);
int fblock(nec_context_t *ctx, int nrow, int ncol, int imax, int ipsym);
void solve(nec_context_t *ctx, int n, complex double *a, int *ip, complex double *b, int ndim);
void solves(nec_context_t *ctx, complex double *a, int *ip, complex double *b, int neq, int nrh, int np, int n, int mp, int m);
/* network.c */
void network(nec_context_t *ctx, complex double *cm, int *ip, complex double *einc);
/* radiation.c */
void ffld(nec_context_t *ctx, double thet, double phi, complex double *eth, complex double *eph);
void fflds(nec_context_t *ctx, double rox, double roy, double roz, complex double *scur, complex double *ex, complex double *ey, complex double *ez);
void gfld(nec_context_t *ctx, double rho, double phi, double rz, complex double *eth, complex double *epi, complex double *erd, complex double ux, int ksymp);
void rdpat(nec_context_t *ctx);
/* somnec.c */
void somnec(nec_context_t *ctx, double epr, double sig, double fmhz);
void bessel(nec_context_t *ctx, complex double z, complex double *j0, complex double *j0p);
void evlua(nec_context_t *ctx, complex double *erv, complex double *ezv, complex double *erh, complex double *eph);
void fbar(nec_context_t *ctx, complex double p, complex double *r);
int gshank(nec_context_t *ctx, complex double start, complex double dela, complex double *sum, int nans, complex double *seed, int ibk, complex double bk, complex double delb);
int hankel(nec_context_t *ctx, complex double z, complex double *h0, complex double *h0p);
void lambda(nec_context_t *ctx, double t, complex double *xlam, complex double *dxlam);
void rom1(nec_context_t *ctx, int n, complex double *sum, int nx);
void saoa(nec_context_t *ctx, double t, complex double *ans);
/* control.c */
int nec_calculation_defaults(nec_context_t *ctx);
int process_control_cards(nec_context_t *ctx, deck_t *deck);
int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq);
