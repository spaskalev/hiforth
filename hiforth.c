struct ctx;

typedef void (*machine_word)(struct ctx* c);

struct ctx {
   int         *memory;
   int          ip;           // instruction pointer

   int         *return_stack;
   int          rsi;          // return stack index

   int          is_running;
   machine_word next;
};

/*
 * Trampoline executor to avoid using the native stack.
 */
void trampoline(struct ctx* c) {
   while(c->is_running) c->next(c);
}

/*
 * Stops the execution of the provided context.
 */
void halt(struct ctx* c) {
   c->is_running = 0;
}

/*
 * Handler for non-machine words
 */
void enter(struct ctx* c) {
   c->return_stack[c->rsi++] = c->ip;  // save current ip
   c->ip++;                            // calculate new ip
}

int main() {
   int memory[4096] = {0};
   int retstack[128] = {0};

   struct ctx c = {memory, 0, retstack, 0, 1, &halt};

   trampoline(&c);
}