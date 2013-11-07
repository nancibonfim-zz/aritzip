#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

typedef unsigned short int code_t;

unsigned long *calculate_frequencies(FILE*);
unsigned long *extracts_frequencies(FILE*);
void mount_table(unsigned long*);
void write_frequencies(unsigned long *);
void compress(FILE*);
void decompress(FILE*);
int in(FILE*);
static inline void out(int bit);
void (*process_file)(FILE*);
void error(char*);
void info(void);

#define EOS 0
#define LEN_IND 257
#define MAX 0XFFFF
#define TOTAL table[indice[EOS]]

unsigned int table[LEN_IND+1]; /* intervals associated for each symbol */
unsigned int indice[LEN_IND+1]; /* mapping of each symbol to a indice of the table */
unsigned int symbol[LEN_IND+1];
FILE *output;

void mount_uniforme_iso_table(FILE *file) {
  int i;
  table[0] = 0;
  symbol[0] = -1;
  for (i = 1; i < 257; i++) {
    indice[i] = i;
    symbol[i] = i-1;
    table[i] = table[i-1] + 4;
  }
  indice[EOS] = i;
  table[indice[EOS]] = table[i-1] + 1;
  symbol[indice[EOS]] = EOS;
}

int is_char(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

void read(FILE *file) {
  int c;
  while ((c = fgetc(file)) != EOF) {
    do {
      printf("%d", c & 1);
    } while ((c >>= 1) != 0);
    printf("\n");
  }
}

unsigned long *calculate_frequencies(FILE *file) {
  unsigned long total, *frequency, bigger, escala;
  long beginning_file;
  int c;

  frequency = (unsigned long*) calloc(LEN_IND + 1, sizeof(unsigned long));
  begining_file = ftell(file);
  while ((c = fgetc(file)) != EOF)
    if (frequency[c] != ULONG_MAX) {
      frequency[c]++;
    }
  if (ftell(file) == beginning_file) {
    free(frequency);		
    fclose(file);
    fclose(output);
    exit(EXIT_SUCCESS);
  }
  fseek(file, beginning_file, 0);

  bigger = 0;
  for (c = 0 ; c < LEN_IND ; c++)
    if (frequency[c] > bigger)
      bigger = frequency[c];
  escala = 1 + bigger/LEN_IND;
  total = 1;

  for (c = 0; c < LEN_IND ; c++)
    if (frequency[c] != 0)
      {
        total += frequency[c];
        if (frequency[c] <= escala) frequency[c] = 1;
        else frequency[c] = frequency[c] / escala;
      }
  if (total > 32511)
    escala = 2;
  else if ( total > 16383 )
    escala = 1;
  else
    return frequency;

  for (c = 0 ; c < LEN_IND ; c++)
    if (frequency[c] != 0)
      {
        frequency[c] >>= escala;
        if (frequency[c] == 0) frequency[c] = 1;
      }
  return frequency;
}

/* mount_table: creates a table of intervals */
void mount_table(unsigned long *frequency) {
  unsigned int *ordering, *previous, n;
  int c;

  ordering = (unsigned int*) calloc(LEN_IND+1, sizeof(unsigned int));
  previous = (unsigned int*) calloc(LEN_IND+1, sizeof(unsigned int));
  for (c = LEN_IND-1 ; c >= 0 ; c--)
    if (frequency[c]) {
      previous[c + 1] = ordering[frequency[c]];
      ordering[frequency[c]] = c + 1;
    }
  
  for (c = LEN_IND-1, n = 0; c > 0; c--)
    while(ordering[c] != 0) {
      n++;
      symbol[n] = ordering[c] - 1;
      table[n] = table[n-1] + c;
      indice[ordering[c]] = n;
      ordering[c] = previous[ordering[c]];
    }
  
  indice[EOS] = n + 1;
  table[indice[EOS]] = table[n] + 1;
  symbol[indice[EOS]] = '$';
  
  free(frequency);
  free(ordering);
  free(previous);
}

void write_frequencies(unsigned long *frequency) {
  int i, n;
  for (i = 0; i < LEN_IND; i++)
    if (frequency[i] != 0)
      fputc(frequency[i], output);
    else {
      fputc(0, output);
      for(n = 0; (i + 1 < LEN_IND) && (frequency[i+1] == 0); i++)
        n++;
      fputc(n, output);
    }
}

unsigned long *extracts_frequencies(FILE *file) {
  int c, i;
  unsigned long *frequency;

  frequency = (unsigned long*) calloc(LEN_IND+1, sizeof(unsigned long));
  for (i = 0; i < LEN_IND; i++)
    if ((c = fgetc(file)) != 0) {
      frequency[i] = c;
    }
    else {
      i += fgetc(file);
    }
  return frequency;
}

void info(void) {
  int i;
  for(i = 1; table[i] && i < LEN_IND; i++)
    printf("c=%c indice[%d] = %d t[%d] = %d\n",
           is_char(symbol[i])?symbol[i]:'#', i, indice[symbol[i]], 
           indice[symbol[i]], table[indice[symbol[i]]]);
  printf("TOTAL = %d\n", TOTAL);
}

/* aritzip main: compress ou decompress files */
int main(int argc, char **argv) {
  FILE *file;
  char uso[128];
  char use_std = 0;
  unsigned long *frequencies;

  if ( (argc < 3) || (argv[1][1] != '\0') ) {
      sprintf(uso, "To compress: %s c file_fonte file_destino\n \
				To decompress: %s d file_fonte file_destino",	argv[0], argv[0]);
      error(uso);
  }
  switch (argv[1][0]) {
  case 's':
    use_std = 1;
  case 'c':
  case 'C':
    process_file = compress;
    break;
  case 'd':
  case 'D':
    process_file = decompress;
    break;
  case 'l':
    process_file = le;
    break;
  default:
    error(uso);
  }
  
  file = fopen(argv[2], "rb");
  output = stdout;

  if (process_file == compress) {
    frequencies = calculate_frequencies(file);
    write_frequencies(frequencies);
    mount_table(frequencies);
  }
  else if (process_file == decompress)
    mount_table(extracts_frequencies(file));
  process_file(file);
  fclose(file);
  fclose(output);
  return 0;
}


/* compress: compress the files */
void compress(FILE *file) { 
  unsigned long interval;
  unsigned long underflow;
  code_t H, L;
  unsigned int c;
  int char_read;
  L = 0;
  H = MAX;
  underflow = 0;
  do {
    char_read = fgetc(file);
    c = char_read + 1;
    interval = (unsigned long) (H - L) + 1;
    H = L + (code_t) ((table[ indice[c] ] * interval)/TOTAL - 1);
    L += (code_t) ((interval*table[ indice[c] - 1 ])/TOTAL);
    
    for (;;) {
      if ((H & 0x8000) == (L & 0x8000)) {
        out(H & 0x8000);
        for( ; underflow > 0; underflow--)
          out(~H & 0x8000);
      }
      else if ((L & 0x4000) && (~H & 0x4000)) {
        underflow ++;
        L &= 0x3FFF;
        H |= 0x4000;
      }
      else  break;
      L <<= 1;
      H <<= 1;
      H |= 1;
    }
  } while (char_read != EOF);
  /* end of codification */
  
  /* FLUSH to the output:
   * puts the final bits of the interval and of the output's 
   buffer in the compressed file */
  underflow++;
  out(L & 0x4000);
  while (underflow-- > 0)
    out(~L & 0x4000);
  for(c = 0; c < 7; c++) 
    out(0);
}

static inline void out(int bit) { 
  static int buffer = 0;
  static int bits_faltantes = 8;
  
  buffer >>= 1;
  if(bit)  buffer |= 0x80;
  if (--bits_faltantes == 0) {
    putc(buffer, output);
    bits_faltantes = 8;
  }
}

/* decompress the file name_of_file.az to name_of_file */
void decompress(FILE *file) {
  unsigned long interval;
  code_t H, L, freq, value;
  int c, i;
  unsigned int ind;

  L = 0;
  H = MAX;
  value = 0;

  for (i = 0; i < 16; i++) {
    int char_read = in(file);
    value = 2 * value + char_read;
  }

  for (;;) {
    interval = (unsigned long) (H - L) + 1;
    freq = (code_t) ((((unsigned long) (value - L) + 1) * TOTAL - 1) / interval);
    for (ind = 1; freq >= table[ind]; ind++);
    if(ind == indice[EOS]) break;
    H = L + (code_t) ((interval * table[ind]) / TOTAL - 1);
    L += (code_t) ((interval * table[ind - 1]) / TOTAL);
    
    for (;;) {
      if ((H & 0x8000) == (L & 0x8000));
      else if ((L & 0x4000) && (~H & 0x4000)) {
        value ^= 0x4000;
        L &= 0x3fff;
        H |= 0x4000;
      }
      else
        break;
      L <<= 1;
      H <<= 1;
      H |= 1;
      value <<= 1;
      value += in(file);
    }
    fputc(symbol[ind], output);
  }
}

int in(FILE *file) {
  static int buffer = 0;
  static int remaining_bits = 0;
  int ret;

  if (remaining_bits == 0) {
    buffer = fgetc(file);
    remaining_bits = 8;
  }
  ret	= buffer & 1;
  buffer >>= 1;
  remaining_bits --;
  return ret;
}

void error(char *msg) {
  exit(1);
}
