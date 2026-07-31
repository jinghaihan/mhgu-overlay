# Size presets and safety

## How presets work

Choose **Size lock** before entering a quest. The choice is saved, but the
overlay does not modify a quest definition or save file in advance. It waits
for a large monster's runtime object, validates that object, writes the
selected crown threshold, and reads the value back immediately.

The selector has four modes:

| Mode | Result |
| --- | --- |
| Off | Does not request any size writes |
| Mini | Uses that monster's small gold crown threshold |
| Silver | Uses that monster's large silver crown threshold |
| Gold | Uses that monster's large gold crown threshold |

MHGU does not have a small silver crown category. Fixed-size monsters are not
written in any mode.

## Validation rules

- Off is the default and free-form multipliers are not accepted.
- The adapter requires the supported MHGU title and build ID.
- Every target is resolved independently for the detected monster.
- Kiranico's Mini and Gold thresholds define the conservative per-monster
  write range.
- Both the portable core and Switch adapter reject values outside that range.
- The adapter rechecks monster identity and current-map state just before a
  write.
- Only the size multiplier field is written.
- Every successful write must pass an immediate read-back check.

Legal ranges are global per monster, not separated by map or quest. A chosen
threshold can therefore be applied in a quest that would not normally roll
that crown. MH Crown is used only as an independent data cross-check; a
disagreement never widens a Kiranico-derived range.

## Testing status

Automated tests cover preset selection, range checks, identity validation,
and read-back behavior. A successful CI build proves compilation and host-test
success only.

Real-hardware testing is still required for firmware behavior, model scale,
hitboxes, crown registration, quest records, special-event monsters, and save
effects. Back up your save before testing and do not assume the feature is
safe for online or competitive use.
