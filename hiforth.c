#include <setjmp.h>
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
   printf("halt reached!\n");
   longjmp(c->trampoline, 1);
}

/*
 * Duplicate the top of the data stack.
 */
void dup(struct ctx* c) {
   memcpy(c->dsi + WORD_SIZE, c->dsi, WORD_SIZE);   // Duplicate
   c->dsi += WORD_SIZE;                             // Bump the data stack
}

/*
 * Put a constant on the data stack.
 */
void lit(struct ctx* c) {
   c->dsi += WORD_SIZE;                   // Bump the data stack
   memcpy(c->dsi, c->w + 1, WORD_SIZE);   // Copy literal
}

/*
 * Sum two integers from the data stack
 */
void add(struct ctx* c) {
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

int main() {
   unsigned char memory[4096] = {0};
   long int memip = 0;
   unsigned char data_stack[128] = {0};
   unsigned char return_stack[128] = {0};


   intptr_t *i1 = malloc(WORD_SIZE * 2);
   memset(i1, 0, WORD_SIZE * 2);
   word p = &lit;
   memcpy(i1, &p, WORD_SIZE);
   i1[1] = 3;
   memcpy(memory + memip, &i1, WORD_SIZE);
   memip += WORD_SIZE;

   intptr_t *i2 = malloc(WORD_SIZE);
   p = &dup;
   memcpy(i2, &p, WORD_SIZE);
   memcpy(memory + memip, &i2, WORD_SIZE);
   memip += WORD_SIZE;

   intptr_t *i3 = malloc(WORD_SIZE);
   p = &mul;
   memcpy(i3, &p, WORD_SIZE);
   memcpy(memory + memip, &i3, WORD_SIZE);
   memip += WORD_SIZE;

   intptr_t *i4 = malloc(WORD_SIZE);
   p = &print_top;
   memcpy(i4, &p, WORD_SIZE);
   memcpy(memory + memip, &i4, WORD_SIZE);
   memip += WORD_SIZE;

   intptr_t *i5 = malloc(WORD_SIZE);
   p = &halt;
   memcpy(i5, &p, WORD_SIZE);
   memcpy(memory + memip, &i5, WORD_SIZE);
   memip += WORD_SIZE;

   struct ctx c = {0};
   c.ip = memory;
   c.dsi = data_stack;
   c.rsi = return_stack;

   trampoline(&c);
}

/*
   // word p = &halt;
   // memcpy(memory + 8, &p, sizeof(word));

   // intptr_t *i = (intptr_t*) (memory+8);
   // memcpy(memory, &i, sizeof(intptr_t));
*/