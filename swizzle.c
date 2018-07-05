// swizzle.c

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/syscall.h>
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
  switch(pc[1]) {
  case '=':
    sp[1] = sp[1] == *sp;
    sp++;
    pc++;
    break;
  case '$':
    *(char *)val[pc[2]] = (char)*sp++;
    pc+=2;
    break;
  case '^':
    *(size_t *)val[pc[2]] = *sp++;
    pc+=2;
    break;
  case '&':
    sp = (size_t *)*sp;
    pc++;
    break;
  default:
    val[pc[1]] = *sp++;
    pc++;
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
  *sp = ((size_t (*)())val[*pc])(*sp, sp[1], sp[2], sp[3], sp[4], sp[5]);
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
  iset['&'] = iaddr;
  iset['^'] = iptr;
  iset['$'] = icptr;
  iset['"'] = istring;
  iset[';'] = ipop;
  iset['='] = ieq;
  iset['!'] = inot;
  iset['%'] = imod;
  iset['<'] = ilt;
  iset['>'] = igt;
  iset['+'] = iadd;
  iset['-'] = isub;
  iset['*'] = imul;
  iset['/'] = idiv;
  iset['?'] = icond;
  iset[':'] = ifwd;
  iset['@'] = iback;
  iset['{'] = ifun;
  iset['}'] = iret;
  iset['\\']= iremap;

  iset['P'] = isys; val['P'] = (size_t)printf;
  iset['D'] = isys; val['D'] = (size_t)dlsym;
  iset['S'] = isys; val['S'] = (size_t)syscall;
  iset['E'] = isys; val['E'] = (size_t)exit;
  iset[0]   = isys; val[0]   = (size_t)exit;
  
  for(pc = program; *pc; pc++) {
#ifdef DEBUG
//    printf("%d: ", (int)syscall(SYS_getpid));
    printf("[%c] ", *pc);
    printf("stack %d: [%p, %p, %p, %p]", sp - &stack[STACK_SZ], (size_t)*sp, (size_t)sp[1], (size_t)sp[2], (size_t)sp[3]);
    printf("\n"); fflush(stdout);
#endif
    iset[*pc]();
  }
}
