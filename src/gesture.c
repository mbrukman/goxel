/* Goxel 3D voxels editor
 *
 * copyright (c) 2017 Guillaume Chereau <guillaume@noctua-software.com>
 *
 * Goxel is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.

 * Goxel is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.

 * You should have received a copy of the GNU General Public License along with
 * goxel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "goxel.h"

// Still experimental gestures manager.

// XXX: this value should be set depending on the screen resolution.
static float g_start_dist = 8;

static bool rect_contains(vec4_t rect, vec2_t pos)
{
    return pos.x >= rect.x && pos.x < rect.x + rect.z &&
           pos.y >= rect.y && pos.y < rect.y + rect.w;
}

static int update(gesture_t *gest, const inputs_t *inputs, int mask)
{
    const touch_t *ts = inputs->touches;

    if (gest->type == GESTURE_DRAG) {
        switch (gest->state) {
        case GESTURE_POSSIBLE:
            if (ts[0].down[gest->button]) {
                gest->start_pos = ts[0].pos;
                gest->pos = gest->start_pos;
                if (!rect_contains(gest->view, gest->pos)) {
                    gest->state = GESTURE_FAILED;
                    break;
                }
                gest->state = (mask & (GESTURE_CLICK | GESTURE_PINCH)) ?
                            GESTURE_RECOGNISED: GESTURE_BEGIN;
            }
            break;
        case GESTURE_RECOGNISED:
            if (vec2_dist(gest->start_pos, ts[0].pos) >= g_start_dist)
                gest->state = GESTURE_BEGIN;
            if (!ts[0].down[gest->button])
                gest->state = GESTURE_POSSIBLE;
            break;
        case GESTURE_BEGIN:
        case GESTURE_UPDATE:
            gest->pos = ts[0].pos;
            gest->state = GESTURE_UPDATE;
            if (!ts[0].down[gest->button])
                gest->state = GESTURE_END;
            break;
        }
    }

    if (gest->type == GESTURE_CLICK) {
        gest->pos = ts[0].pos;
        switch (gest->state) {
            case GESTURE_POSSIBLE:
            if (ts[0].down[gest->button]) {
                gest->start_pos = ts[0].pos;
                gest->state = GESTURE_RECOGNISED;
            }
            break;
            case GESTURE_RECOGNISED:
            if (!ts[0].down[gest->button]) {
                gest->state = GESTURE_TRIGGERED;
            }
            break;
        }
    }

    return 0;
}


int gesture_update(int nb, gesture_t *gestures[],
                   const inputs_t *inputs, const vec4_t *view, void *user)
{
    int i, j, mask = 0;
    bool allup = true;
    gesture_t *gest, *triggered = NULL;

    for (i = 0; !allup && i < ARRAY_SIZE(inputs->touches); i++) {
        for (j = 0; !allup && j < ARRAY_SIZE(inputs->touches[i].down); j++) {
            if (inputs->touches[i].down[i]) allup = false;
        }
    }

    for (i = 0; i < nb; i++) {
        gest = gestures[i];
        if (gest->state == GESTURE_POSSIBLE) mask |= gest->type;
    }

    for (i = 0; i < nb; i++) {
        gest = gestures[i];
        gest->view = *view;
        if ((gest->state == GESTURE_FAILED) && allup) {
            gest->state = GESTURE_POSSIBLE;
        }
        if (IS_IN(gest->state, GESTURE_END, GESTURE_TRIGGERED))
            gest->state = GESTURE_POSSIBLE;

        update(gest, inputs, mask);
        if (IS_IN(gest->state, GESTURE_BEGIN, GESTURE_UPDATE, GESTURE_END,
                               GESTURE_TRIGGERED)) {
            gest->callback(gest, user);
            triggered = gest;
            break;
        }
    }

    // If one gesture started, fail all the other gestures.
    if (triggered && triggered->state == GESTURE_BEGIN) {
        for (i = 0; i < nb; i++) {
            gest = gestures[i];
            if (gest != triggered)
                gest->state = GESTURE_FAILED;
        }
    }

    // Special case for the hover gesture.
    if (!triggered && allup) {
        for (i = 0; i < nb; i++) {
            gest = gestures[i];
            if (gest->type != GESTURE_HOVER) continue;
            gest->pos = inputs->touches[0].pos;
            gest->callback(gest, user);
            triggered = gest;
        }
    }

    return triggered ? 1 : 0;
}