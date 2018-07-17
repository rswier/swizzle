// swizzle.c

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dlfcn.h>

#define STACK_SZ 256

char *program, *pc;

void (*iset[256])();
size_t val[256] = {0};

size_t stack[STACK_SZ+10] = {0};
size_t *sp = &stack[STACK_SZ];

size_t rstack[STACK_SZ+10] = {0};
size_t *rs = &rstack[STACK_SZ];

void inop() {
}
void ivar() {
  *--sp = val[*pc];
}
void idigit() {
  size_t i = *pc - '0';
  while (isdigit(pc[1]))
    i = i*10 + *++pc - '0';
  *--sp = i;
}
void istring() {
  for (*--sp = (size_t)++pc; *pc; pc++) {
    if (*pc == '"') {
      *pc = 0;
      break;
    }
  }
}
void ipop() {
  sp++;
}
void ieq() {
  switch(*++pc) {
  case '=':
    sp[1] = sp[1] == *sp;
    sp++;
    break;
  case '$':
    *(char *)*sp = (char)sp[1];
    sp+=2;
    break;
  case '^':
    *(size_t *)*sp = sp[1];
    sp+=2;
    break;
  case '&':
    sp = (size_t *)*sp;
    break;
  default:
    val[*pc] = *sp++;
  }
}
void inot() {
  if (pc[1] == '=') {
    sp[1] = sp[1] != *sp;
    pc++;
    sp++;
  } else {
    *sp = !*sp;
  }
}
void igt() {
  if (pc[1] == '=') {
    sp[1] = sp[1] >= *sp;
    pc++;
  } else {
    sp[1] = sp[1] > *sp;
  }
  sp++;
}
void ilt() {
  if (pc[1] == '=') {
    sp[1] = sp[1] <= *sp;
    pc++;
  } else {
    sp[1] = sp[1] < *sp;
  }
  sp++;
}
void iadd() {
  if (pc[1] == '+') {
    val[pc[-1]]++;
    pc++;
  } else {
    sp[1] += *sp;
    sp++;
  }
}
void isub() {
  if (pc[1] == '-') {
    val[pc[-1]]--;
    pc++;
  } else {
    sp[1] -= *sp;
    sp++;
  }
}
void imul() {
  sp[1] *= *sp;
  sp++;
}
void idiv() {
  sp[1] /= *sp;
  sp++;
}
void imod() {
  sp[1] %= *sp;
  sp++;
}
void ifwd() {
  int level = 0;
  for (;;) {
    switch (*++pc) {
    case '(': level++; continue;
    case ':': if (!level) return; continue;
    case ')': if (!level) return; level--;
    }
  }
}
void iback() {
  int level = 0;
  for (;;) {
    switch (*--pc) {
    case ')': level++; continue;
    case '(': if (!level) return; level--;
    }
  }
}
void icond() {
  if (!*sp++)
    ifwd();
}
void icall() {
  *--rs = val['r'];
  val['r'] = (size_t)pc;
  pc = (char *)val[*pc];
}
void iptr() {
  *sp = *(size_t *)*sp;
}
void icptr() {
  *sp = (size_t)*(char *)*sp;
}
void iaddr() {
  --sp;
  *sp = (size_t)(sp+1);
}
void ifun() {
  iset[*++pc] = &icall;
  val[*pc] = (size_t)pc;
  while (*pc && *pc != '}')
    pc++;
}
void iret() {
  pc = (char *)val['r'];
  val['r'] = *rs++;
}
void iremap() {
  iset[pc[1]] = iset[pc[2]];
  val[pc[1]] = val[pc[2]];
  pc += 2;
}
void isys() {
  *sp = ((size_t (*)())val[*pc])(*sp, sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7]);
}

int main(int argc, char *argv[])
{
  int i, fd;
  struct stat st;

  if (argc != 2) {
    printf("usage: %s <program>\n", argv[0]);
    exit(1);
  }
  if (stat(argv[1], &st)) {
    printf("file not found\n");
    exit(1);
  }

  program = malloc(st.st_size + 1);
  fd = open(argv[1], O_RDONLY);
  i = read(fd, program, st.st_size);
  program[i] = 0;

  for (i=0;   i<256;  i++) iset[i] = inop;
  for (i='a'; i<='z'; i++) iset[i] = ivar;
  for (i='0'; i<='9'; i++) iset[i] = idigit;
  iset['"'] = istring;
  iset[';'] = ipop;
  iset['='] = ieq;
  iset['!'] = inot;
  iset['>'] = igt;
  iset['<'] = ilt;
  iset['+'] = iadd;
  iset['-'] = isub;
  iset['*'] = imul;
  iset['/'] = idiv;
  iset['%'] = imod;
  iset[':'] = ifwd;
  iset['@'] = iback;
  iset['?'] = icond;
  iset['^'] = iptr;
  iset['$'] = icptr;
  iset['&'] = iaddr;
  iset['{'] = ifun;
  iset['}'] = iret;
  iset['\\']= iremap;

  iset['P'] = isys; val['P'] = (size_t)printf;
  iset['D'] = isys; val['D'] = (size_t)dlsym;
  iset['E'] = isys; val['E'] = (size_t)exit;
  iset[0]   = isys; val[0]   = (size_t)exit;
  
  for(pc = program; *pc; pc++) {
#ifdef DEBUG
//    printf("pc: %p", (void *)pc);
    printf("[%c] ", *pc);
    printf("stack %ld: [%p, %p, %p, %p]", sp - &stack[STACK_SZ], (void *)*sp, (void *)sp[1], (void *)sp[2], (void *)sp[3]);
    printf("\n"); fflush(stdout);
    if (sp - &stack[STACK_SZ] > 0) exit(9);
#endif
    iset[*pc]();
  }
}
