// $Id$

#include "dfa-interpreter-min.h"

#include "jlocale.h"
#include "jrx-intern.h"
#include "nfa.h"
#include "util.h"

int jrx_match_state_advance_min(jrx_match_state* ms, jrx_char cp, jrx_assertion assertions) {
    jrx_dfa_state* state = dfa_get_state(ms->dfa, ms->state);

    if ( ! state )
        return 0;

    if ( ms->dfa->options & JRX_OPTION_DEBUG )
        fprintf(stderr, "> in state #%u with input symbol %u and assertions %u ", ms->state, cp, assertions);

    vec_for_each(dfa_transition, state->trans, trans) {
        jrx_ccl* ccl = vec_ccl_get(ms->dfa->ccls->ccls, trans.ccl);

        if ( ! ccl_match(ccl, cp, ms->offset == 0 ? &ms->previous : 0, assertions) )
            // Doesn't match.
            continue;

        ++ms->offset;

        // Found transition.
        jrx_dfa_state_id succ_id = trans.succ;
        jrx_dfa_state* succ_state = dfa_get_state(ms->dfa, succ_id);

        ms->state = succ_id;
        ms->previous = cp;

        if ( ms->dfa->options & JRX_OPTION_DEBUG )
            fprintf(stderr, "-> found transition, new state is #%d", succ_id);

        if ( succ_state->accepts ) {
            jrx_accept_id aid = vec_dfa_accept_get(succ_state->accepts, 0).aid;

            if ( ms->dfa->options & JRX_OPTION_DEBUG )
                fprintf(stderr, " (accepting with ID %d)\n", aid);

            // Accepting.
            return aid;
        }

        if ( ms->dfa->options & JRX_OPTION_DEBUG )
            fputs("\n", stderr);

        // Partial match.
        return -1;
    }

    if ( ms->dfa->options & JRX_OPTION_DEBUG )
        fputs("-> no transition possible", stderr);

    // Matching failed. Check if the start state is already an accepting one.
    if ( state->accepts ) {
        jrx_accept_id aid = vec_dfa_accept_get(state->accepts, 0).aid;

        if ( ms->dfa->options & JRX_OPTION_DEBUG )
            fprintf(stderr, " (accepting with ID %d)\n", aid);

        ms->state = -1; // Jam it.

        // Accepting.
        return aid;
    }

    return 0;
}

void jrx_match_state_copy(const jrx_match_state* from, jrx_match_state* to) {
    if ( from->cflags & REG_STD_MATCHER )
        jrx_internal_error("jrx_match_state_copy() used with state from standard matcher; that's not supported");

    to->offset = from->offset;
    to->begin = from->begin;
    to->dfa = from->dfa;
    to->state = from->state;
    to->previous = from->previous;
    to->cflags = from->cflags;
    // Skip fields only used by the full matcher.
    to->acc = from->acc;
}
