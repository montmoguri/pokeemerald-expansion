#ifndef GUARD_SWSH_HALL_OF_FAME_H
#define GUARD_SWSH_HALL_OF_FAME_H

#define SWSH_HALL_OF_FAME         TRUE

// mega and gmax forms
#define SWSH_HOF_SHOW_MEGA        TRUE                // show mega evolution forms if mon is holding mega stones
#define SWSH_HOF_SHOW_GMAX        TRUE                // show gigantamax form if mon has gmax factor

#define SWSH_HOF_FORM_MEGA        0
#define SWSH_HOF_FORM_GMAX        1
#define SWSH_HOF_FORM_PRIORITY    SWSH_HOF_FORM_MEGA  // form to show when a mon has both mega and gmax

void CB2_DoHallOfFameScreen_SwSh(void);
void CB2_DoHallOfFameScreenDontSaveData_SwSh(void);

#endif // GUARD_SWSH_HALL_OF_FAME_H
