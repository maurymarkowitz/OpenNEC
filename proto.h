/*
 * Function prototypes for nec2c.c
 */

/*------------------------------------------------------------------------*/
/* main.c */
int main(int argc, char **argv);
void null_pointers(void);
void prnt(int in1, int in2, int in3, double fl1, double fl2, double fl3, double fl4, double fl5, double fl6, char *ia, int ichar);
/* input.c */
void read_deck(Deck *deck, FILE *pfile);
int read_line(char *buff, FILE *pfile);
void parse_deck(Deck *deck, Errors *errors);
void parse_comment_card(Card *card, Errors *errors);
void parse_geometry_or_control_card(Card *card, Errors *errors);
void parse_onec_card(Card *card, Errors *errors);
void parse_key_values(Card *card, Errors *errors);
/* output.c */
void write_deck_onec(Deck *deck, FILE *pfile);
void write_nec_output(Deck *deck, FILE *pfile);
void write_header(Deck *deck, FILE *pfile);
void write_structure(Deck *deck, FILE *pfile);
void write_segments(Deck *deck, FILE *pfile);
void write_patches(Deck * deck, FILE *pfile);
/* deck.c */
void update_deck_values(Deck *deck);
void update_card_values(Card *card);
int isComment(Card *card);
int isGeometry(Card *card);
int isControl(Card *card);
int isExtension(Card *card);
int min_int_fields(Card *card);
int max_int_fields(Card *card);
int min_flt_fields(Card *card);
int max_flt_fields(Card *card);
void add_key_value(Card *card, KeyValue *list, char *key, char *value, char separator);
/* test.c */
void test_deck_structure(Deck *deck, Errors *errors);
void test_duplicate_tags(Deck *deck, Errors *errors);
/* misc.c */
void add_error(Errors *errors, char *message, int severity);
int strendswith(const char *str, const char *suffix);
char* substr(char* dest, char *src, int start, int len);
char* trim_start(char* dest);
char* trim_end(char* dest);
void abort_on_error(int why);
void secnds(double *x);
int stop(int flag);
void mem_alloc(void **ptr, size_t req);
void mem_realloc(void **ptr, size_t req);
void mem_free(void **ptr);
/* calculations.c */
void cabc(complex double *curx);
void couple(complex double *cur, double wlam);
void load(int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt, double *zlr, double *zli, double *zlc);
void gf(double zk, double *co, double *si);
double db10(double x);
double db20(double x);
void intrp(double x, double y, complex double *f1, complex double *f2, complex double *f3, complex double *f4);
void intx(double el1, double el2, double b, int ij, double *sgr, double *sgi);
int min(int a, int b);
void test(double f1r, double f2r, double *tr, double f1i, double f2i, double *ti, double dmin);
void sbf(int i, int is, double *aa, double *bb, double *cc);
void tbf(int i, int icap);
void trio(int j);
void zint(double sigl, double rolam, complex double *zt);
double cang(complex double z);
/* fields.c */
void efld(double xi, double yi, double zi, double ai, int ij);
void eksc(double s, double z, double rh, double xk, int ij, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void ekscx(double bx, double s, double z, double rhx, double xk, int ij, int inx1, int inx2, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void gh(double zk, double *hr, double *hi);
void gwave(complex double *erv, complex double *ezv, complex double *erh, complex double *ezh, complex double *eph);
void gx(double zz, double rh, double xk, complex double *gz, complex double *gzp);
void gxx(double zz, double rh, double a, double a2, double xk, int ira, complex double *g1, complex double *g1p, complex double *g2, complex double *g2p, complex double *g3, complex double *gzp);
void hfk(double el1, double el2, double rhk, double zpkx, double *sgr, double *sgi);
void hintg(double xi, double yi, double zi);
void hsfld(double xi, double yi, double zi, double ai);
void hsflx(double s, double rh, double zpx, complex double *hpk, complex double *hps, complex double *hpc);
void nefld(double xob, double yob, double zob, complex double *ex, complex double *ey, complex double *ez);
void nfpat(void);
void nhfld(double xob, double yob, double zob, complex double *hx, complex double *hy, complex double *hz);
void pcint(double xi, double yi, double zi, double cabi, double sabi, double salpi, complex double *e);
void unere(double xob, double yob, double zob);
/* geometry.c */
int segment_number(int tag, int m);
void calculate_geometry(Deck *deck, Errors *errors);
void finish_geometry(void);
void connect_segments(int ignd);
void wire(int card_num, int tag_num, int segs, double xw1, double yw1, double zw1, double xw2, double yw2, double zsw2, double rad, double rdel, double rrad);
void arc(int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad);
void helix(int card_num, int tag_num, int segs, double s, double hl, double a1, double b1, double a2, double b2, double rad);
void patch(int card_num, int nx, int ny, double ax1, double ay1, double az1, double ax2, double ay2, double az2, double ax3, double ay3, double az3, double ax4, double ay4, double az4);
void calculate_patch(int nx, int ny);
void reproduce(double rox, double roy, double roz, double xs, double ys, double zs, int its, int nrpt, int itgi);
void reflect(int card_num, int tag_increment, int ix, int iy, int iz);
void rotate(int card_num, int tag_increment, int num_copies);
void scale(double xw1);
void qdsrc(int is, complex double v, complex double *e);
/* ground.c */
void rom2(double a, double b, complex double *sum, double dmin);
void sflds(double t, complex double *e);
/* matrix.c */
void cmset(int nrow, complex double *cm, double rkhx, int iexkx);
void cmss(int j1, int j2, int im1, int im2, complex double *cm, int nrow, int itrp);
void cmsw(int j1, int j2, int i1, int i2, complex double *cm, complex double *cw, int ncw, int nrow, int itrp);
void cmws(int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int itrp);
void cmww(int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int nw, int itrp);
void etmns(double p1, double p2, double p3, double p4, double p5, double p6, int ipr, complex double *e);
void factr(int n, complex double *a, int *ip, int ndim);
void factrs(int np, int nrow, complex double *a, int *ip);
void fblock(int nrow, int ncol, int imax, int ipsym);
void solve(int n, complex double *a, int *ip, complex double *b, int ndim);
void solves(complex double *a, int *ip, complex double *b, int neq, int nrh, int np, int n, int mp, int m);
/* network.c */
void network(complex double *cm, int *ip, complex double *einc);
/* radiation.c */
void ffld(double thet, double phi, complex double *eth, complex double *eph);
void fflds(double rox, double roy, double roz, complex double *scur, complex double *ex, complex double *ey, complex double *ez);
void gfld(double rho, double phi, double rz, complex double *eth, complex double *epi, complex double *erd, complex double ux, int ksymp);
void rdpat(void);
/* somnec.c */
void somnec(double epr, double sig, double fmhz);
void bessel(complex double z, complex double *j0, complex double *j0p);
void evlua(complex double *erv, complex double *ezv, complex double *erh, complex double *eph);
void fbar(complex double p, complex double *r);
void gshank(complex double start, complex double dela, complex double *sum, int nans, complex double *seed, int ibk, complex double bk, complex double delb);
void hankel(complex double z, complex double *h0, complex double *h0p);
void lambda(double t, complex double *xlam, complex double *dxlam);
void rom1(int n, complex double *sum, int nx);
void saoa(double t, complex double *ans);
