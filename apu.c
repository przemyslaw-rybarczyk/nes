#include "global.h"
#include "apu.h"

float duty_cycles[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0},
};

void apu_envelope_update(APU_Envelope *envelope, u8 volume, bool loop) {
    if (envelope->start) {
        envelope->start = false;
        envelope->decay = 15;
        envelope->divider = volume;
    } else {
        if (envelope->divider == 0) {
            envelope->divider = volume;
            if (envelope->decay == 0) {
                if (loop)
                    envelope->decay = 15;
            } else {
                (envelope->decay)--;
            }
        } else {
            (envelope->divider)--;
        }
    }
}

void apu_quarter_frame(System *sys) {
    apu_envelope_update(&sys->pulse1_envelope, sys->pulse1_volume, sys->pulse1_halt);
    apu_envelope_update(&sys->pulse2_envelope, sys->pulse2_volume, sys->pulse2_halt);
}

void apu_apply_sweep(APU_Sweep *sweep, u16 *period) {
    if (sweep->enable && sweep->divider == 0 && *period >= 8 && sweep->target < 0x800)
        *period = sweep->target;
    if (sweep->divider == 0 || sweep->reload) {
        sweep->divider = sweep->period;
        sweep->reload = false;
    } else {
        sweep->divider--;
    }
}

void apu_half_frame(System *sys) {
    if (!sys->pulse1_halt && sys->pulse1_length_counter > 0)
        sys->pulse1_length_counter--;
    apu_apply_sweep(&sys->pulse1_sweep, &sys->pulse1_period);
    if (!sys->pulse2_halt && sys->pulse2_length_counter > 0)
        sys->pulse2_length_counter--;
    apu_apply_sweep(&sys->pulse2_sweep, &sys->pulse2_period);
}

void apu_calculate_sweep_target(APU_Sweep *sweep, u16 period, u16 i) {
    u16 change = period >> sweep->shift;
    if (sweep->negate)
        change = -change - i;
    sweep->target = period + change;
}

bool apu_timer_tick(u16 *timer, u16 period) {
    if (*timer == 0) {
        *timer = period;
        return true;
    } else {
        (*timer)--;
        return false;
    }
}

float apu_step(System *sys) {
    apu_calculate_sweep_target(&sys->pulse1_sweep, sys->pulse1_period, 1);
    float pulse1 = sys->pulse1_enable && sys->pulse1_length_counter > 0 && sys->pulse1_period >= 8 && sys->pulse1_sweep.target < 0x800
        ? (sys->pulse1_const_volume ? sys->pulse1_volume : sys->pulse1_envelope.decay)
            * duty_cycles[sys->pulse1_duty][sys->pulse1_sequencer]
        : 0;
    if ((sys->APU_cycle_counter & 1) == 0 && apu_timer_tick(&sys->pulse1_timer, sys->pulse1_period))
        sys->pulse1_sequencer = (sys->pulse1_sequencer - 1) & 7;

    apu_calculate_sweep_target(&sys->pulse2_sweep, sys->pulse2_period, 0);
    float pulse2 = sys->pulse2_enable && sys->pulse2_length_counter > 0 && sys->pulse2_period >= 8 && sys->pulse2_sweep.target < 0x800
        ? (sys->pulse2_const_volume ? sys->pulse2_volume : sys->pulse2_envelope.decay)
            * duty_cycles[sys->pulse2_duty][sys->pulse2_sequencer]
        : 0;
    if ((sys->APU_cycle_counter & 1) == 0 && apu_timer_tick(&sys->pulse2_timer, sys->pulse2_period))
        sys->pulse2_sequencer = (sys->pulse2_sequencer - 1) & 7;

    switch (sys->APU_sequencer_mode) {
    case 0:
        switch (sys->APU_cycle_counter) {
        case 0:
            break;
        case 7457:
            apu_quarter_frame(sys);
            break;
        case 14193:
            apu_quarter_frame(sys);
            apu_half_frame(sys);
            break;
        case 22371:
            apu_quarter_frame(sys);
            break;
        case 29828:
            break;
        case 29829:
            apu_quarter_frame(sys);
            apu_half_frame(sys);
            break;
        }
        break;
    case 1:
        switch (sys->APU_cycle_counter) {
        case 0:
            break;
        case 7457:
            apu_quarter_frame(sys);
            break;
        case 14193:
            apu_quarter_frame(sys);
            apu_half_frame(sys);
            break;
        case 22371:
            apu_quarter_frame(sys);
            break;
        case 37281:
            apu_quarter_frame(sys);
            apu_half_frame(sys);
            break;
        }
        break;
    }
    sys->APU_cycle_counter++;
    if (sys->APU_cycle_counter > (sys->APU_sequencer_mode ? 37281 : 29829))
        sys->APU_cycle_counter = 0;
    return (pulse1 + pulse2) / 2;
}
