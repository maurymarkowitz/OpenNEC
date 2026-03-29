GS card parsing note
--------------------

4nec2 allows the unit type to be specified on a per-field basis, for instance, `0.1cm`. This is converted to NEC-standard meters at runtime, using a series of conversion constants. That is, the "cm" is a constant with the value 0.01, so the field value `0.1cm` is converted to "0.1 x 0.01", producing the internal value of 0.001 meters, which is then used in any following calculations. 

Many 4nec2 decks contain spaces between the numerical value and the unit type. OpenNEC attempts to split fields using a flexible system that looks for spaces, tabs, commas and other separators. Normally this would result in something like `0.1 cm` being interpreted as two fields, 0.1 and cm. There is special code to find these sorts of entries and remove the extra spaces so they are merged back into one field.

However, this runs into another oddity found in some 4nec2 decks, which have the bare unit type in their `GS` cards, like `GS 0 0 ft`. The "merging" code would normally turn this into `GS 0 0ft`, so the required F1 field disappears, and I2 becomes "0 x in", both of which cause the calculation to fail.

To address this, there is special code in the `GS` card handler to look for this sort of entry and correctly interpret it as `1 x ft` when the scale is interpreted to zero.

Generally, if you want your decks to be robust in both 4nec2 and OpenNEC, you should put unit suffixes directly adjacent to numeric values, e.g., `135ft`,  or use a tab between fields to avoid implicit multiplication across fields.
