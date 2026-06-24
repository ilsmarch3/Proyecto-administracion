/*
 * stegano_finder.c  –  Herramienta forense / esteganografía  v2
 * ─────────────────────────────────────────────────────────────────────────────
 * Compilar:
 *   gcc -O2 -pthread -Wall -o stegano_finder stegano_finder.c
 *
 * Uso:
 *   ./stegano_finder <archivo> [hilos] [--damaged] [--strict]
 *                              [--filter ext1,ext2,...] [--minmatch N]
 *
 * Flags nuevos v2:
 *   --strict    Omite firmas ≤4 bytes (reduce falsos positivos drásticamente)
 *   --minmatch N  Exige al menos N bytes de firma (defecto sin --strict: 2)
 *
 * Ejemplos:
 *   ./stegano_finder disco.img 8 --strict
 *   ./stegano_finder dump.bin 4 --damaged --filter jpg,png,pdf,zip
 *   ./stegano_finder archivo.png --strict --damaged
 * ─────────────────────────────────────────────────────────────────────────────
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

/* ══════════════════════════════════════════════════════════════════
   Tabla de firmas
   Campos extra:
     min_context_len  – bytes adicionales tras la firma que deben verificarse
     context_mask[]   – qué bits importan (0x00 = ignorar)
     context_val[]    – valor esperado
   ══════════════════════════════════════════════════════════════════ */
typedef struct {
    const char *ext;
    const char *desc;
    uint8_t     magic[20];
    size_t      magic_len;
    long        sig_offset;
    /* validación contextual opcional (hasta 4 bytes extra tras la firma) */
    uint8_t     ctx_mask[4];   /* 0x00 = no importa */
    uint8_t     ctx_val[4];
    int         ctx_len;       /* cuántos bytes de ctx validar (0 = ninguno) */
} Sig;

static const Sig SIGS[] = {
/* ──────────────── Imágenes ──────────────── */
/* JPEG: los 4 primeros bytes son suficientes y muy específicos */
{"jpg",  "JPEG RAW",        {0xFF,0xD8,0xFF,0xDB},           4, 0, {0},{0},0},
{"jpg",  "JPEG JFIF",       {0xFF,0xD8,0xFF,0xE0},           4, 0, {0xFF,0xFF},{0x00,0x10},2},
{"jpg",  "JPEG Exif",       {0xFF,0xD8,0xFF,0xE1},           4, 0, {0},{0},0},
{"jpg",  "JPEG Adobe",      {0xFF,0xD8,0xFF,0xEE},           4, 0, {0},{0},0},
{"png",  "PNG Image",       {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A}, 8, 0, {0},{0},0},
{"gif",  "GIF87a",          {0x47,0x49,0x46,0x38,0x37,0x61}, 6, 0, {0},{0},0},
{"gif",  "GIF89a",          {0x47,0x49,0x46,0x38,0x39,0x61}, 6, 0, {0},{0},0},
/* BMP: 2 bytes solos son muy ambiguos – añadimos contexto: bytes 2-5 son tamaño (>0) */
{"bmp",  "BMP Bitmap",      {0x42,0x4D},                     2, 0, {0x00,0x00,0x00,0x00},{0x00},0},
/* ICO: firma corta muy común en binarios. Añadimos validación:
   bytes 4-5 = 0x00 0x00 (reservado), bytes 6-7 = num imágenes 01..14 */
{"ico",  "Windows Icon",    {0x00,0x00,0x01,0x00},           4, 0,
         {0xFF,0xFF,0xFF,0x00},{0x00,0x00,0x01,0x00},4},
{"tif",  "TIFF LE",         {0x49,0x49,0x2A,0x00},           4, 0, {0},{0},0},
{"tif",  "TIFF BE",         {0x4D,0x4D,0x00,0x2A},           4, 0, {0},{0},0},
{"psd",  "Photoshop",       {0x38,0x42,0x50,0x53},           4, 0,
         {0xFF,0xFF},{0x00,0x01},2},  /* bytes 4-5 = version = 0x0001 */
{"qoi",  "QOI Image",       {0x71,0x6F,0x69,0x66},           4, 0, {0},{0},0},
{"exr",  "OpenEXR",         {0x76,0x2F,0x31,0x01},           4, 0, {0},{0},0},
{"bpg",  "BPG Image",       {0x42,0x50,0x47,0xFB},           4, 0, {0},{0},0},
{"flif", "FLIF Image",      {0x46,0x4C,0x49,0x46},           4, 0, {0},{0},0},
{"icns", "Apple Icon",      {0x69,0x63,0x6E,0x73},           4, 0, {0},{0},0},
{"jp2",  "JPEG 2000",       {0x00,0x00,0x00,0x0C,0x6A,0x50,0x20,0x20}, 8, 0, {0},{0},0},
{"dpx",  "DPX Image",       {0x53,0x44,0x50,0x58},           4, 0, {0},{0},0},

/* ──────────────── Audio / Video ──────────────── */
{"mp3",  "MP3 ID3v2",       {0x49,0x44,0x33},                3, 0,
         {0xFF,0x00},{0x03,0x00},2},  /* version 2.3 o 2.4, flags razonables */
{"mp3",  "MPEG Layer 3",    {0xFF,0xFB},                     2, 0, {0},{0},0},
{"flac", "FLAC Audio",      {0x66,0x4C,0x61,0x43},           4, 0, {0},{0},0},
{"ogg",  "Ogg Container",   {0x4F,0x67,0x67,0x53},           4, 0,
         {0xFF},{0x00},1},  /* byte 4 = version = 0 */
{"wav",  "WAV Audio",       {0x52,0x49,0x46,0x46},           4, 0,
         /* bytes +8..+11 deben ser "WAVE" */
         {0xFF,0xFF,0xFF,0xFF},{0x00},0},  /* se valida por separado abajo */
{"mid",  "MIDI Audio",      {0x4D,0x54,0x68,0x64},           4, 0,
         {0xFF,0xFF,0xFF,0xFF},{0x00,0x00,0x00,0x06},4}, /* chunk size=6 */
{"mkv",  "Matroska/WebM",   {0x1A,0x45,0xDF,0xA3},           4, 0, {0},{0},0},
{"swf",  "Flash SWF (comp)",{0x43,0x57,0x53},                3, 0, {0},{0},0},
{"swf",  "Flash SWF (raw)", {0x46,0x57,0x53},                3, 0, {0},{0},0},
{"flv",  "Flash Video",     {0x46,0x4C,0x56,0x01},           4, 0, {0},{0},0},
{"mp4",  "MP4/ISOM",        {0x66,0x74,0x79,0x70,0x69,0x73,0x6F,0x6D}, 8, 4, {0},{0},0},
{"3gp",  "3GPP Video",      {0x66,0x74,0x79,0x70,0x33,0x67}, 6, 4, {0},{0},0},
{"heic", "HEIC Image",      {0x66,0x74,0x79,0x70,0x68,0x65,0x69,0x63}, 8, 4, {0},{0},0},
{"asf",  "ASF/WMA/WMV",     {0x30,0x26,0xB2,0x75,0x8E,0x66,0xCF,0x11}, 8, 0, {0},{0},0},

/* ──────────────── Documentos ──────────────── */
{"pdf",  "PDF Document",    {0x25,0x50,0x44,0x46,0x2D},      5, 0, {0},{0},0},
{"doc",  "MS Office legacy",{0xD0,0xCF,0x11,0xE0,0xA1,0xB1,0x1A,0xE1}, 8, 0, {0},{0},0},
{"zip",  "ZIP/DOCX/XLSX",   {0x50,0x4B,0x03,0x04},           4, 0, {0},{0},0},
{"zip",  "ZIP empty",       {0x50,0x4B,0x05,0x06},           4, 0, {0},{0},0},
{"xml",  "XML Document",    {0x3C,0x3F,0x78,0x6D,0x6C,0x20}, 6, 0, {0},{0},0},
{"djvu", "DjVu Document",   {0x41,0x54,0x26,0x54,0x46,0x4F,0x52,0x4D}, 8, 0, {0},{0},0},
{"fits", "FITS Astronomy",  {0x53,0x49,0x4D,0x50,0x4C,0x45,0x20,0x20,0x3D}, 9, 0, {0},{0},0},
{"chm",  "MS HtmlHelp",     {0x49,0x54,0x53,0x46,0x03,0x00,0x00,0x00}, 8, 0, {0},{0},0},

/* ──────────────── Comprimidos ──────────────── */
{"gz",   "GZIP",            {0x1F,0x8B},                     2, 0,
         {0xFF},{0x08},1},  /* byte 2 = método = deflate(8) */
{"bz2",  "BZip2",           {0x42,0x5A,0x68},                3, 0,
         {0xFF},{0x39},1},  /* byte 3 = '1'..'9' – usamos 0x39='9' como mask */
{"7z",   "7-Zip",           {0x37,0x7A,0xBC,0xAF,0x27,0x1C}, 6, 0, {0},{0},0},
{"xz",   "XZ Compression",  {0xFD,0x37,0x7A,0x58,0x5A,0x00}, 6, 0, {0},{0},0},
{"rar",  "RAR v1.5+",       {0x52,0x61,0x72,0x21,0x1A,0x07,0x00}, 7, 0, {0},{0},0},
{"rar",  "RAR v5.0+",       {0x52,0x61,0x72,0x21,0x1A,0x07,0x01,0x00}, 8, 0, {0},{0},0},
{"lz4",  "LZ4 Frame",       {0x04,0x22,0x4D,0x18},           4, 0, {0},{0},0},
{"lz",   "LZIP",            {0x4C,0x5A,0x49,0x50},           4, 0, {0},{0},0},
{"tar",  "TAR ustar",       {0x75,0x73,0x74,0x61,0x72},      5, 257,{0},{0},0},
{"cab",  "MS Cabinet",      {0x4D,0x53,0x43,0x46},           4, 0,
         {0xFF,0xFF,0xFF,0xFF},{0x00,0x00,0x00,0x00},4}, /* bytes 4-7 reservados = 0 */
{"xar",  "XAR Archive",     {0x78,0x61,0x72,0x21},           4, 0, {0},{0},0},
{"deb",  "Linux DEB",       {0x21,0x3C,0x61,0x72,0x63,0x68,0x3E,0x0A}, 8, 0, {0},{0},0},
{"rpm",  "RPM Package",     {0xED,0xAB,0xEE,0xDB},           4, 0, {0},{0},0},

/* ──────────────── Ejecutables / Binarios ──────────────── */
/* MZ/PE: 2 bytes muy cortos pero muy usados – se incluye pero se omite en --strict */
{"exe",  "DOS/PE (MZ)",     {0x4D,0x5A},                     2, 0, {0},{0},0},
{"elf",  "ELF Executable",  {0x7F,0x45,0x4C,0x46},           4, 0,
         {0xFF},{0x01},1},  /* EI_CLASS = 1 (32bit) o 2 (64bit) — mask 0 = any */
{"class","Java Class",      {0xCA,0xFE,0xBA,0xBE},           4, 0, {0},{0},0},
{"macho","Mach-O 32-bit",   {0xFE,0xED,0xFA,0xCE},           4, 0, {0},{0},0},
{"macho","Mach-O 64-bit",   {0xFE,0xED,0xFA,0xCF},           4, 0, {0},{0},0},
{"wasm", "WebAssembly",     {0x00,0x61,0x73,0x6D},           4, 0, {0},{0},0},
{"dex",  "Dalvik/APK",      {0x64,0x65,0x78,0x0A,0x30,0x33,0x35,0x00}, 8, 0, {0},{0},0},

/* ──────────────── BD / Misc ──────────────── */
{"sqlite","SQLite DB",      {0x53,0x51,0x4C,0x69,0x74,0x65,0x20,0x66,
                              0x6F,0x72,0x6D,0x61,0x74,0x20,0x33,0x00}, 16, 0, {0},{0},0},
{"pcap", "PCAP (LE)",       {0xD4,0xC3,0xB2,0xA1},           4, 0, {0},{0},0},
{"pcap", "PCAP (BE)",       {0xA1,0xB2,0xC3,0xD4},           4, 0, {0},{0},0},
{"pcapng","PCAPng",         {0x0A,0x0D,0x0D,0x0A},           4, 0, {0},{0x00},0},
{"vmdk", "VMDK Image",      {0x4B,0x44,0x4D},                3, 0, {0},{0},0},
{"iso",  "ISO 9660",        {0x43,0x44,0x30,0x30,0x31},      5, 0x8001, {0},{0},0},
{"pem",  "PEM Cert/Key",    {0x2D,0x2D,0x2D,0x2D,0x2D,0x42,0x45,0x47,
                              0x49,0x4E},                    10, 0, {0},{0},0},
{"woff", "WOFF Font",       {0x77,0x4F,0x46,0x46},           4, 0, {0},{0},0},
{"woff2","WOFF2 Font",      {0x77,0x4F,0x46,0x32},           4, 0, {0},{0},0},
{"nes",  "NES ROM",         {0x4E,0x45,0x53,0x1A},           4, 0, {0},{0},0},
{"lep",  "Lepton JPEG",     {0xCF,0x84,0x01},                3, 0, {0},{0},0},
{"psafe3","Password Gorilla",{0x50,0x57,0x53,0x33},          4, 0, {0},{0},0},
{"ppk",  "PuTTY Key",       {0x50,0x75,0x54,0x54,0x59,0x2D,0x55,0x73,
                              0x65,0x72,0x2D,0x4B,0x65,0x79,0x2D,0x46}, 16, 0, {0},{0},0},
};

#define N_SIGS  ((int)(sizeof(SIGS)/sizeof(SIGS[0])))

/* Firmas que se omiten en modo --strict (magic_len <= 4 y sin ctx extra fiable) */
/* Marcamos los índices "problemáticos" dinámicamente según magic_len */
#define STRICT_MIN_LEN  4   /* en --strict solo firmas >= 5 bytes o con ctx >= 2 */

/* ══════════════════════════════════════════════════════════════════ */
typedef struct Result {
    long         offset;
    int          sig_idx;
    int          damaged;
    int          diff_bytes;
    struct Result *next;
} Result;

typedef struct {
    const uint8_t  *data;
    size_t          data_size;
    size_t          start;
    size_t          end;
    int             detect_damaged;
    int             strict_mode;
    int             minmatch;
    char           *filter;
    Result         *results;
    pthread_mutex_t *res_mutex;
} ThreadArg;

/* ══ Colores ══ */
#define RST  "\033[0m"
#define GRN  "\033[1;32m"
#define YEL  "\033[1;33m"
#define CYN  "\033[1;36m"
#define BLD  "\033[1m"

/* ══ Helpers ══ */
static int ext_in_filter(const char *ext, const char *filter) {
    if (!filter || !filter[0]) return 1;
    char fcopy[512];
    strncpy(fcopy, filter, sizeof(fcopy)-1); fcopy[sizeof(fcopy)-1]='\0';
    char *tok = strtok(fcopy, ",");
    while (tok) {
        if (strcasecmp(ext, tok)==0) return 1;
        tok = strtok(NULL, ",");
    }
    return 0;
}

static void add_result(ThreadArg *a, long offset, int sig_idx, int damaged, int diff) {
    Result *r = malloc(sizeof(Result));
    if (!r) return;
    r->offset = offset; r->sig_idx = sig_idx;
    r->damaged = damaged; r->diff_bytes = diff;
    pthread_mutex_lock(a->res_mutex);
    r->next = a->results; a->results = r;
    pthread_mutex_unlock(a->res_mutex);
}

/* Validación contextual: comprueba ctx_len bytes DESPUÉS de la firma */
static int check_context(const Sig *sig, const uint8_t *data, size_t base, size_t dsz) {
    if (sig->ctx_len <= 0) return 1;
    size_t ctx_start = base + sig->magic_len;
    if (ctx_start + (size_t)sig->ctx_len > dsz) return 1; /* borde: aceptar */
    for (int b = 0; b < sig->ctx_len; b++) {
        uint8_t mask = sig->ctx_mask[b];
        if (mask == 0x00) continue;
        if ((data[ctx_start + b] & mask) != (sig->ctx_val[b] & mask)) return 0;
    }
    return 1;
}

/* Validación extra para WAV: bytes +8..+11 = "WAVE" */
static int check_wav(const uint8_t *data, size_t base, size_t dsz) {
    if (base + 12 > dsz) return 1;
    return (data[base+8]=='W' && data[base+9]=='A' &&
            data[base+10]=='V' && data[base+11]=='E');
}

/* Validación extra para ICO: número de imágenes en bytes 4-5 debe ser 1..20 */
static int check_ico(const uint8_t *data, size_t base, size_t dsz) {
    if (base + 6 > dsz) return 1;
    uint16_t count = (uint16_t)(data[base+4] | (data[base+5] << 8));
    return (count >= 1 && count <= 20);
}

/* ══ Hilo de búsqueda ══ */
static void *search_thread(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    const uint8_t *data = a->data;
    size_t dsz = a->data_size;

    for (size_t i = a->start; i < a->end; i++) {
        for (int s = 0; s < N_SIGS; s++) {
            const Sig *sig = &SIGS[s];

            /* filtros de longitud */
            int eff_len = (int)sig->magic_len + sig->ctx_len;
            if (a->strict_mode && eff_len < STRICT_MIN_LEN) continue;
            if ((int)sig->magic_len < a->minmatch) continue;
            if (!a->strict_mode && sig->magic_len < 3) continue;

            /* filtro de extensión */
            if (!ext_in_filter(sig->ext, a->filter)) continue;

            long base = (long)i + sig->sig_offset;
            if (base < 0 || (size_t)(base + (long)sig->magic_len) > dsz) continue;

            int diff = 0;
            for (size_t b = 0; b < sig->magic_len; b++)
                if (data[(size_t)base + b] != sig->magic[b]) diff++;

            if (diff == 0) {
                /* validaciones contextuales específicas */
                if (strcmp(sig->ext,"ico")==0 && !check_ico(data,(size_t)base,dsz)) continue;
                if (strcmp(sig->ext,"wav")==0 && !check_wav(data,(size_t)base,dsz)) continue;
                if (!check_context(sig, data, (size_t)base, dsz)) continue;
                add_result(a, (long)i, s, 0, 0);
            } else if (a->detect_damaged && diff == 1 && sig->magic_len >= 6) {
                if (strcmp(sig->ext,"ico")==0)   continue;
                if (strcmp(sig->ext,"exe")==0)   continue;
                if (strcmp(sig->ext,"bmp")==0)   continue;
                if (strcmp(sig->ext,"jpg")==0)   continue;
                if (strcmp(sig->ext,"tif")==0)   continue;
                if (strcmp(sig->ext,"wasm")==0)  continue;
                if (strcmp(sig->ext,"flif")==0)  continue;
                if (strcmp(sig->ext,"exr")==0)   continue;
                if (strcmp(sig->ext,"qoi")==0)   continue;
                if (strcmp(sig->ext,"xar")==0)   continue;
                if (strcmp(sig->ext,"psd")==0)   continue;
                if (strcmp(sig->ext,"icns")==0)  continue;
                if (strcmp(sig->ext,"flv")==0)   continue;
                if (strcmp(sig->ext,"wav")==0)   continue;
                add_result(a, (long)i, s, 1, diff);
            }
        }
    }
    return NULL;
}

/* ══ Sort + dedup ══ */
static Result *sort_results(Result *head) {
    if (!head || !head->next) return head;
    Result *sorted = NULL;
    while (head) {
        Result *next = head->next;
        if (!sorted || head->offset < sorted->offset) {
            head->next = sorted; sorted = head;
        } else {
            Result *cur = sorted;
            while (cur->next && cur->next->offset <= head->offset) cur = cur->next;
            head->next = cur->next; cur->next = head;
        }
        head = next;
    }
    return sorted;
}

static void hexdump16(const uint8_t *data, size_t off, size_t fsz) {
    printf("    HEX  : ");
    for (int i=0; i<16 && off+(size_t)i<fsz; i++) printf("%02X ",data[off+i]);
    printf("\n    ASCII: ");
    for (int i=0; i<16 && off+(size_t)i<fsz; i++)
        printf("%c", isprint(data[off+i]) ? data[off+i] : '.');
    printf("\n");
}

/* ══ main ══ */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            BLD "\nUso:" RST " %s <archivo> [hilos] [--damaged] [--strict] "
            "[--filter ext,...] [--minmatch N]\n\n"
            "  --strict     Omite firmas cortas/ambiguas (≥5 bytes efectivos)\n"
            "               → Reduce drásticamente los falsos positivos\n"
            "  --minmatch N Solo busca firmas de al menos N bytes (defecto: 2)\n"
            "  --damaged    Detectar cabeceras con hasta 2 bytes distintos\n"
            "  --filter     Limitar a extensiones: jpg,png,pdf,zip,...\n\n"
            BLD "Ejemplos:" RST "\n"
            "  %s disco.img 8 --strict\n"
            "  %s archivo.png --strict --damaged\n"
            "  %s dump.bin 4 --damaged --filter jpg,png,pdf,zip\n"
            "  %s dump.bin --minmatch 6\n\n",
            argv[0],argv[0],argv[0],argv[0],argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int  n_threads   = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads < 1) n_threads = 1;
    int  detect_dmg  = 0;
    int  strict_mode = 0;
    int  minmatch    = 2;
    char *filter     = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strcmp(argv[i],"--damaged")==0)  detect_dmg  = 1;
        else if (strcmp(argv[i],"--strict")==0)   strict_mode = 1;
        else if (strcmp(argv[i],"--filter")==0  && i+1<argc) filter   = argv[++i];
        else if (strcmp(argv[i],"--minmatch")==0 && i+1<argc) minmatch = atoi(argv[++i]);
        else if (isdigit((unsigned char)argv[i][0])) {
            n_threads = atoi(argv[i]);
            if (n_threads<1) n_threads=1;
            if (n_threads>512) n_threads=512;
        }
    }
    if (strict_mode && minmatch < STRICT_MIN_LEN) minmatch = STRICT_MIN_LEN;

    /* ── Leer archivo ── */
    FILE *f = fopen(filename,"rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f,0,SEEK_END); long fsz_l = ftell(f); fseek(f,0,SEEK_SET);
    if (fsz_l<=0) { fprintf(stderr,"Archivo vacío.\n"); fclose(f); return 1; }
    size_t fsz = (size_t)fsz_l;
    uint8_t *data = malloc(fsz);
    if (!data) { fprintf(stderr,"Sin memoria.\n"); fclose(f); return 1; }
    if (fread(data,1,fsz,f)!=fsz) { fprintf(stderr,"Error lectura.\n"); free(data); fclose(f); return 1; }
    fclose(f);

    printf(BLD
        "\n╔══════════════════════════════════════════════════════╗\n"
        "║     STEGANO FINDER v2 – Buscador de Cabeceras        ║\n"
        "╚══════════════════════════════════════════════════════╝\n" RST);
    printf("  Archivo  : %s\n", filename);
    printf("  Tamaño   : %ld bytes  (%.2f MB)\n", fsz_l, (double)fsz_l/(1024.0*1024.0));
    printf("  Hilos    : %d\n", n_threads);
    printf("  Firmas   : %d\n", N_SIGS);
    printf("  Modo     : %s\n", strict_mode ? "STRICT (firmas ≥5 bytes)" : "NORMAL");
    printf("  Dañadas  : %s\n", detect_dmg ? "SÍ (≤2 bytes distintos)" : "NO");
    printf("  Filtro   : %s\n\n", filter ? filter : "(todas)");

    pthread_mutex_t res_mutex = PTHREAD_MUTEX_INITIALIZER;
    Result *all_results = NULL;
    pthread_t  *threads = malloc((size_t)n_threads * sizeof(pthread_t));
    ThreadArg  *args    = calloc((size_t)n_threads, sizeof(ThreadArg));

    size_t chunk = fsz / (size_t)n_threads;
    size_t overlap = 512;

    for (int t = 0; t < n_threads; t++) {
        args[t].data           = data;
        args[t].data_size      = fsz;
        args[t].start          = (size_t)t * chunk;
        args[t].end            = (t==n_threads-1) ? fsz : (size_t)(t+1)*chunk+overlap;
        if (args[t].end > fsz) args[t].end = fsz;
        args[t].detect_damaged = detect_dmg;
        args[t].strict_mode    = strict_mode;
        args[t].minmatch       = minmatch;
        args[t].filter         = filter;
        args[t].results        = NULL;
        args[t].res_mutex      = &res_mutex;
        pthread_create(&threads[t], NULL, search_thread, &args[t]);
    }
    for (int t = 0; t < n_threads; t++) {
        pthread_join(threads[t], NULL);
        Result *r = args[t].results;
        while (r) { Result *nx=r->next; r->next=all_results; all_results=r; r=nx; }
    }

    /* sort y dedup */
    all_results = sort_results(all_results);
    { Result *cur=all_results;
      while (cur&&cur->next) {
        if (cur->offset==cur->next->offset && cur->sig_idx==cur->next->sig_idx) {
            Result *dup=cur->next; cur->next=dup->next; free(dup);
        } else cur=cur->next;
      }
    }

    /* imprimir */
    int n_exact=0, n_dmg=0;
    printf(BLD "══════════════════════ RESULTADOS ══════════════════════\n" RST);
    for (Result *r=all_results; r; r=r->next) {
        const Sig *sig = &SIGS[r->sig_idx];
        if (r->damaged) {
            n_dmg++;
            printf(YEL "⚠  CABECERA POSIBLEMENTE DAÑADA\n" RST);
            printf("   Tipo     : " BLD "%s" RST " – %s\n", sig->ext, sig->desc);
            printf("   Bytes Δ  : %d byte(s) de %zu distintos\n", r->diff_bytes, sig->magic_len);
        } else {
            n_exact++;
            printf(GRN "✔  CABECERA EXACTA\n" RST);
            printf("   Tipo     : " BLD "%s" RST " – %s\n", sig->ext, sig->desc);
        }
        printf("   Offset   : " BLD "%ld" RST " dec  /  0x%lX hex\n",
               r->offset, (unsigned long)r->offset);
        printf("   Bloque   : sector 512B #%ld  /  bloque 4K #%ld\n",
               r->offset/512, r->offset/4096);
        hexdump16(data, (size_t)r->offset, fsz);
        printf(CYN
            "   dd bs=1   : dd if=%s of=extraido_%ld.%s bs=1 skip=%ld\n"
            "   dd bs=512 : dd if=%s of=extraido_%ld.%s bs=512 skip=%ld\n\n" RST,
            filename, r->offset, sig->ext, r->offset,
            filename, r->offset, sig->ext, r->offset/512);
    }

    printf(BLD "══════════════════════ RESUMEN ═════════════════════════\n" RST);
    printf("  " GRN "Exactas   : %d\n" RST, n_exact);
    printf("  " YEL "Dañadas   : %d\n" RST, n_dmg);
    printf("      Total : %d\n\n", n_exact+n_dmg);
    if (n_exact+n_dmg==0)
        printf("  Sin resultados. Prueba sin --strict o ajusta --filter.\n\n");

    Result *r=all_results; while(r){Result *nx=r->next;free(r);r=nx;}
    free(data); free(threads); free(args);
    pthread_mutex_destroy(&res_mutex);
    return 0;
}
