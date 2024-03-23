#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#define WORD_SIZE sizeof(intptr_t)

struct ctx;

typedef void (*word)(struct ctx* c);

struct ctx {
   unsigned char *ip;           // instruction pointer
   unsigned char *dsi;          // data stack index
   unsigned char *rsi;          // return stack index
   intptr_t      *w;            // work register
   jmp_buf        trampoline;
};

/*
 * Trampoline executor to avoid using the native stack.
 */
void trampoline(struct ctx* c) {
   if (setjmp(c->trampoline)) {
      return;
   }
   while(1) {
      memcpy(&c->w, c->ip, WORD_SIZE);   // Store first indirection in work register
      c->ip += WORD_SIZE;                // Advance ip
      ((word) *(c->w))(c);               // Call second indirection
   }
}

/*
 * Stops the execution of the provided context.
 */
void halt(struct ctx* c) {
   printf("called halt\n");
   longjmp(c->trampoline, 1);
}

/*
 * Duplicate the top of the data stack.
 */
void dup(struct ctx* c) {
   printf("called dup\n");
   memcpy(c->dsi + WORD_SIZE, c->dsi, WORD_SIZE);   // Duplicate
   c->dsi += WORD_SIZE;                             // Bump the data stack
}

/*
 * Put a constant on the data stack.
 */
void lit(struct ctx* c) {
   printf("called lit\n");
   c->dsi += WORD_SIZE;                   // Bump the data stack
   memcpy(c->dsi, c->w + 1, WORD_SIZE);   // Copy literal
}

/*
 * Sum two integers from the data stack
 */
void add(struct ctx* c) {
   printf("called add\n");
   intptr_t a, b;
   memcpy(&a, c->dsi, WORD_SIZE);
   memcpy(&b, c->dsi - WORD_SIZE, WORD_SIZE);

   a += b;
   c->dsi -= WORD_SIZE;
   memcpy(c->dsi, &a, WORD_SIZE);
}

/*
 * Multiply two integers from the data stack
 */
void mul(struct ctx* c) {
   printf("called mul\n");
   intptr_t a, b;
   memcpy(&a, c->dsi, WORD_SIZE);
   memcpy(&b, c->dsi - WORD_SIZE, WORD_SIZE);

   a *= b;
   c->dsi -= WORD_SIZE;
   memcpy(c->dsi, &a, WORD_SIZE);
}

/*
 * Print the top of the stack
 */
void print_top(struct ctx* c) {
   intptr_t a;
   memcpy(&a, c->dsi, WORD_SIZE);
   printf("top of stack: %lu\n", a);
}

// /*
//  * Starting handler for non-machine words.
//  */
// void enter(struct ctx* c) {
//    c->rsi += WORD_SIZE;                 // Bump the return stack
//    memcpy(c->rsi, &c->ip, WORD_SIZE);   // Store the current ip on it

//    c->ip = c->w + WORD_SIZE;            // Update the ip with the next word
// }

// /*
//  * Ending handler for non-machine words.
//  */
// void leave(struct ctx* c) {
//    c->rsi -= WORD_SIZE;                 // Pop the return stack
//    memcpy(&c->ip, c->rsi, WORD_SIZE);   // Restore the ip
// }

void place(unsigned char **mem, word wrd, int vc, ...) {

   intptr_t *def = malloc(WORD_SIZE * (1 + vc));
   memset(def, 0, WORD_SIZE * (1 + vc));
   memcpy(def, &wrd, WORD_SIZE);

   va_list args;
   va_start(args, vc);
   int c = 1;
   while(c <= vc) {
      def[c] = va_arg(args, intptr_t);
      c++;
   }
   va_end(args);

   memcpy(*mem, &def, WORD_SIZE);
   *mem += WORD_SIZE;   // Note: WORD_SIZE here, this is the thread
}

int main() {
   unsigned char memory[4096] = {0};
   unsigned char data_stack[128] = {0};
   unsigned char return_stack[128] = {0};

   unsigned char *mem = memory;

   place(&mem, lit, 1, 2);
   place(&mem, dup, 0);
   place(&mem, add, 0);
   place(&mem, print_top, 0);
   place(&mem, halt, 0);

   struct ctx c = {0};
   c.ip = memory;
   c.dsi = data_stack;
   c.rsi = return_stack;

   trampoline(&c);
}
