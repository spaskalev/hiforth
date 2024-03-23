#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

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
      memcpy(&c->w, c->ip, sizeof(c->w));   // Store first indirection in work register
      ((word) *(c->w))(c);                  // Call second indirection
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
 * Starting handler for non-machine words.
 */
void enter(struct ctx* c) {
   c->rsi += WORD_SIZE;                 // Bump the return stack
   memcpy(c->rsi, &c->ip, WORD_SIZE);   // Store the current ip on it

   c->w += WORD_SIZE;                   // Bump the work register
   memcpy(c->ip, &c->w, WORD_SIZE);     // Update the ip with the next word
}

int main() {
   unsigned char memory[4096] = {0};
   unsigned char data_stack[128] = {0};
   unsigned char return_stack[128] = {0};

   word p = &halt;
   memcpy(memory + 8, &p, sizeof(word));

   intptr_t *i = (intptr_t*) (memory+8);
   memcpy(memory, &i, sizeof(intptr_t));

   struct ctx c = {0};
   c.ip = memory;
   c.dsi = data_stack;
   c.rsi = return_stack;

   trampoline(&c);
}
