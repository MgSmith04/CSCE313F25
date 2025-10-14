#include <threading.h>
#include <stdio.h>

void t_init()
{
        // TODO
        // initialize arrays & indices for managing worker contexts
        // what does that mean?
        current_context_idx = 0;

        // printf("intializing...\n"); // DEBUG LINE
        struct worker_context contexts[NUM_CTX];
        for (uint8_t i = 0; i < NUM_CTX; i++) {
                contexts[i].state = INVALID;
        }

        // initialize it with getcontext
        contexts[current_context_idx].state = VALID;
        ucontext_t* ucp = &contexts[current_context_idx].context;
        getcontext(ucp); // this should be the main thread

        // make sure to allocate stack
        ucp->uc_stack.ss_sp = (char *) malloc(STK_SZ);
        ucp->uc_stack.ss_size = STK_SZ;
        ucp->uc_stack.ss_flags = 0;
        ucp->uc_link = NULL;
        // makecontext(ucp, (int ) &main, 0);


        // getcontext(&contexts[current_context_idx].context);
        // printf("context_idx: ");
        // printf("%d", current_context_idx);
        // printf("\n");
        // for (uint8_t i = 0; i < NUM_CTX; i ++) {
        //         printf("index: ");
        //         printf("%d", i);
        //         printf(" state: ");
        //         printf("%d", contexts[i].state);
        //         printf("\n");
        // }
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // TODO
        // find first empty context in the contexts array
        // printf("\ncreating...\n"); // DEBUG LINE
        uint8_t next_context_idx = find_next_free();
        if (next_context_idx > NUM_CTX) {
                return -1;
        }
        // printf("next_context_idx: ");
        // printf("%d", next_context_idx);
        // printf("\n");

        
        // DEBUG LINES
        // for (uint8_t i = 0; i < NUM_CTX; i ++) {
        //         printf("index: ");
        //         printf("%d", i);
        //         printf(" state: ");
        //         printf("%d", contexts[i].state);
        //         printf("\n");
        // }

        // initialize it with getcontext
        contexts[next_context_idx].state = VALID;
        ucontext_t* ucp = &contexts[next_context_idx].context;
        getcontext(ucp);

        // make sure to allocate stack
        ucp->uc_stack.ss_sp = (char *) malloc(STK_SZ);
        ucp->uc_stack.ss_size = STK_SZ;
        ucp->uc_stack.ss_flags = 0;
        ucp->uc_link = NULL;

        // use makecontext to modify it so that it calls foo with its arguments
        makecontext(ucp, (void (*)()) foo, 2, arg1, arg2);
        
        // current_context_idx = next_context_idx;
        // printf("created\n");

        // DEBUG LINES
        // for (uint8_t i = 0; i < NUM_CTX; i ++) {
        //         printf("index: ");
        //         printf("%d", i);
        //         printf(" state: ");
        //         printf("%d", contexts[i].state);
        //         printf("\n");
        // }
        
        return 0;
        
        // return 1; // if it reaches the end of the array, all slots must be filled
        
        // next = find_next_free();
        // if(next == curr) return ERROR; // no free slots
        // use makecontext to create a frame for foo(arg1, arg2)
        // return SUCCESS;
}

int32_t t_yield()
{
        /*
        * In this function you can first update the current context (whose index is stored in current_context_idx) 
        * by taking a fresh snapshot using getcontext. 
        * Then you can search for a VALID context entry in the contexts array and use swapcontext to switch to it. 
        * After this you can then compute the number of contexts in the VALID state and return the count. 
        * Note that since the switch happens before you compute the number of contexts in the VALID state, 
        * when the original context is resumed, the calculation of this count will be done at that point. 
        */
        // TODO
        // next = find_next_valid();
        uint8_t next_context_idx = find_next_valid();

        // printf("\nyielding...\n"); // DEBUG LINE
        // printf("next_context_idx: ");
        // printf("%d", next_context_idx);
        // printf("\n");
        // the caller uses the current slot in the proc table
        
        // if(next == curr) return ERROR; // only caller remains
        if (next_context_idx == current_context_idx) {
                return 0;
        }
        if (next_context_idx > NUM_CTX) {
                return -1;
        }
        //TODO
        // atomically:
        // save caller's context in current;
        ucontext_t* ucp = &contexts[current_context_idx].context;
        getcontext(ucp);
        ucontext_t* oucp = &contexts[next_context_idx].context;
        // getcontext(oucp);

        

        // DEBUG LINES     
        // printf("swapping: "); // DEBUG LINE
        // printf("%d", current_context_idx);
        // printf(" to ");
        // printf("%d", next_context_idx);
        // printf("\n");

        current_context_idx = next_context_idx;
        swapcontext(ucp, oucp); // they just yield forever?

        // restore context from next;
        // swapcontext(oucp, ucp);
        // return SUCCESS;

        
        // uint8_t validCount = 0;
        // for (uint8_t i = 0; i < NUM_CTX; i++) {
        //         if (contexts[i].state == VALID) {
        //                 validCount++;
        //         }
        // }

        uint8_t validCount = 0;
        for (uint8_t i = 0; i < NUM_CTX; i++) {
                if (contexts[i].state == VALID) {
                        validCount++;
                }
        }
        // printf("validcount: ");
        // printf("%d", validCount);
        // printf("\n");
        return validCount;
}

void t_finish()
{
        // TODO
        // printf("finishing...\n"); //DEBUG LINE
        contexts[current_context_idx].state = DONE;
        // free the entry in slot current;
        ucontext_t* ucp = &contexts[current_context_idx].context;
        getcontext(ucp);
        free(ucp->uc_stack.ss_sp);
        // // there's at least one for the main thread
        // next = find_next_valid(); 
        uint8_t next_context_idx = find_next_free();
        // use setcontext to set context with proctable[next]
        setcontext(&contexts[next_context_idx].context);

}

// returns index of next free space in contexts array, returns 255 if none found
uint8_t find_next_free() {
        uint8_t next = 0;
        while (next < NUM_CTX) {
                if (next == current_context_idx) {
                        next++;
                        continue;
                }
                if (contexts[next].state == INVALID) {
                        return next;
                }
                next++;
        }
        return 255; 
}

// returns the index of the next filled space in contexts array, returns 255 if error
uint8_t find_next_valid() {
        // start at current index
        uint8_t next = 0;
        while (next < NUM_CTX) {
                //  && ((current_context_idx - 1) == next)
                if ((contexts[next].state == VALID) && (current_context_idx != next)) {
                        return next;
                }
                // else if ((current_context_idx == 0) && (next == 0)) {
                //         return next; // at index 0, is main thread, don't swap
                // }
                next++;
        }
        return current_context_idx; // error
}
