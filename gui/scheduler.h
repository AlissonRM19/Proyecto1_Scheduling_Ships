#pragma once

/* Advances the simulation by dt seconds.
   In connected mode only animates the active ship; in standalone mode
   also handles dispatch, direction switching, and slice expiry. */
void schedulerUpdate(float dt);
