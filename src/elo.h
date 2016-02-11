#ifndef ELO_H
#define ELO_H

#include "player.h"

#define DEFAULT_ELO 1500

/*
 * Update elo's point of each rankable player.
 */
void update_elos(struct player *players, unsigned length);

#endif /* ELO_H */
