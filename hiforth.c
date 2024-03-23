#include <setjmp.h>
#include <stdint.h>
#include <string.h>

struct ctx;

typedef void (*machine_word)(struct ctx* c);

struct ctx {
   unsigned char    *memory;
   long int          ip;           // instruction pointer

   unsigned char    *return_stack;
   long int          rsi;          // return stack index

   jmp_buf      trampoline;
};

/*
 * Trampoline executor to avoid using the native stack.
 */
void trampoline(struct ctx* c) {
   if (setjmp(c->trampoline)) {
      return;
   }
   while(1) {
      intptr_t *f;
      memcpy(&f, c->memory + c->ip, sizeof(f));

      ((machine_word) *f)(c);
   }
}

/*
 * Stops the execution of the provided context.
 */
void halt(struct ctx* c) {
   longjmp(c->trampoline, 1);
}

/*
 * Handler for non-machine words
 */
void enter(struct ctx* c) {
   c->return_stack[c->rsi++] = c->ip;  // save current ip
   c->ip++;                            // calculate new ip
}

int main() {
   unsigned char memory[4096] = {0};
   unsigned char return_stack[128] = {0};

   machine_word p = &halt;
   memcpy(memory + 8, &p, sizeof(machine_word));

   intptr_t *i = (intptr_t*) (memory+8);
   memcpy(memory, &i, sizeof(intptr_t));

   struct ctx c = {0};
   c.memory = memory;
   c.return_stack = return_stack;

   trampoline(&c);
}