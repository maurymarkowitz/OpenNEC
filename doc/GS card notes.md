GS card parsing note
--------------------

Some NEC decks place unit tokens (e.g., "in", "ft") separated by spaces in the GS card
(`GS I1 I2 F1`). The general-purpose preprocessing step that inserts implicit
multiplication (so `135 ft` -> `135*ft`) can merge a unit token onto the previous
integer field (e.g., `0 in` -> `0*in`), which causes the float field to be empty and
can produce a zero scale factor unexpectedly.

To handle this oddity, OpenNEC special-cases the `GS` card during geometry processing:
- If the evaluated scale is zero, OpenNEC attempts to recover the scale by extracting
  the raw third token from the un-preprocessed card text and evaluating that token on
  its own (so `in` -> `0.0254`).

If you maintain decks intended for use with OpenNEC, prefer putting unit suffixes
directly adjacent to numeric values (e.g., `135ft`) or using a tab between fields to
avoid implicit multiplication across fields.
