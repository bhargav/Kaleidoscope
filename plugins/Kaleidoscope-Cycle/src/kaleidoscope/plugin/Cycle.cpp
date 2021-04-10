/* -*- mode: c++ -*-
 * Kaleidoscope-Cycle -- Key sequence cycling dead key for Kaleidoscope.
 * Copyright (C) 2016, 2017, 2018, 2021  Keyboard.io, Inc
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "kaleidoscope/Runtime.h"
#include <Kaleidoscope-Cycle.h>
#include <Kaleidoscope-FocusSerial.h>
#include "kaleidoscope/keyswitch_state.h"
#include "kaleidoscope/key_events.h"

namespace kaleidoscope {
namespace plugin {
// --- state ---
Key Cycle::last_non_cycle_key_;
KeyAddr Cycle::cycle_key_addr_{KeyAddr::invalid_state};
uint8_t Cycle::current_modifier_flags_;
uint8_t Cycle::cycle_count_;

// --- helpers ---

#define isCycle(k) (k.getRaw() == kaleidoscope::ranges::CYCLE)

// --- api ---

void Cycle::replace(Key key) {
  if (cycle_key_addr_ == KeyAddr{KeyAddr::invalid_state})
    return;
  Runtime.handleKeyEvent(KeyEvent{cycle_key_addr_, IS_PRESSED | INJECTED, Key_Backspace});
  Runtime.handleKeyEvent(KeyEvent{cycle_key_addr_, WAS_PRESSED | INJECTED, Key_Backspace});
  Runtime.handleKeyEvent(KeyEvent{cycle_key_addr_, IS_PRESSED | INJECTED, key});
  Runtime.handleKeyEvent(KeyEvent{cycle_key_addr_, WAS_PRESSED | INJECTED, key});
}

void Cycle::replace(uint8_t cycle_size, const Key cycle_steps[]) {
  uint8_t idx = cycle_count_ % cycle_size;
  replace(cycle_steps[idx].readFromProgmem());
}

// --- hooks ---

EventHandlerResult Cycle::onNameQuery() {
  return ::Focus.sendName(F("Cycle"));
}

EventHandlerResult Cycle::onKeyEvent(KeyEvent &event) {
  if (event.state & INJECTED)
    return EventHandlerResult::OK;

  if (!isCycle(event.key)) {
    if (keyToggledOn(event.state)) {
      current_modifier_flags_ |= toModFlag(event.key.getKeyCode());
      last_non_cycle_key_.setKeyCode(event.key.getKeyCode());
      last_non_cycle_key_.setFlags(current_modifier_flags_);
      cycle_count_ = 0;
    }
    if (keyToggledOff(event.state)) {
      current_modifier_flags_ &= ~toModFlag(event.key.getKeyCode());
    }
    return EventHandlerResult::OK;
  }

  if (!keyToggledOff(event.state)) {
    return EventHandlerResult::EVENT_CONSUMED;
  }

  ++cycle_count_;
  cycle_key_addr_ = event.addr;
  cycleAction(last_non_cycle_key_, cycle_count_);
  return EventHandlerResult::EVENT_CONSUMED;
}

uint8_t Cycle::toModFlag(uint8_t keyCode) {
  switch (keyCode) {
  case Key_LeftShift.getKeyCode():
  case Key_RightShift.getKeyCode():
    return SHIFT_HELD;
  case Key_LeftAlt.getKeyCode():
    return LALT_HELD;
  case Key_RightAlt.getKeyCode():
    return RALT_HELD;
  case Key_LeftControl.getKeyCode():
  case Key_RightControl.getKeyCode():
    return CTRL_HELD;
  case Key_LeftGui.getKeyCode():
  case Key_RightGui.getKeyCode():
    return GUI_HELD;
  default:
    return 0;
  }
}

}
}

__attribute__((weak))
void cycleAction(Key previous_key, uint8_t cycle_count) {
}

kaleidoscope::plugin::Cycle Cycle;
