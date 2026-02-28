# Error message inventory

This document lists every call site of `add_error()` found in the tree and the exact call expression (file:line + code). For calls where the message is a literal string, the literal is shown; for calls using a variable (e.g. `msg`, `err_msg`, `l_msg`) the variable name is shown and will need further lookup to extract the formatted string.

Note: many call sites use `add_error(ctx, errors, msg, <severity>)` where `msg` is composed earlier in the function — those are marked `VARIABLE` and should be expanded in a follow-up step.

---

/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:170: add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:176: add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:199: add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:212: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:225: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:231: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:243: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:268: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:287: add_error(ctx, errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:320: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:336: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:356: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:366: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:371: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:376: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:381: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:386: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:391: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:396: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:401: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:411: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:417: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:422: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:432: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:438: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:443: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:448: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:468: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:473: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:478: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:486: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:496: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:519: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:530: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:541: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:546: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:551: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:558: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:563: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:614: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:648: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:702: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:709: //      add_error(errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:716: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:723: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:733: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:743: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:752: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:772: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:779: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:791: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:804: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:814: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:819: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:824: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:830: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:851: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:856: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:878: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:883: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:900: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:905: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:926: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:931: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:965: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:970: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:984: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:991: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1028: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1033: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1039: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1065: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1070: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1088: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1104: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1109: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1163: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1174: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1179: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1184: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1189: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1195: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1200: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1205: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1210: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1216: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1223: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1307: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1346: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1373: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1401: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1406: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1411: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1421: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1429: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1434: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1483: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1521: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1531: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1542: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1548: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1554: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1565: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1573: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1582: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1589: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1594: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1604: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1610: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1622: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1627: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1635: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1640: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1650: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1655: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1661: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1666: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1672: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1677: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1686: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1691: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1696: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1705: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1710: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1715: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1762: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1777: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1804: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1856: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c:1885: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.h:19: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/ground.c:54: add_error(ctx, &ctx->errors, "ERROR - B LESS THAN A IN ROM2", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:169: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:187: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:297: add_error(ctx, errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:358: add_error(ctx,errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:374: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:425: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:431: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:504: add_error(ctx, errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:516: add_error(ctx, errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:534: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:540: add_error(ctx, errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:565: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:641: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:675: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:712: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:720: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:837: add_error(ctx, &ctx->geometry.errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1035: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1039: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1205: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1641: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1710: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1766: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1820: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1872: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1933: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:1986: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c:2289: add_error(ctx, &ctx->geometry.errors, msg, 1);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/somnec.c:616: add_error(ctx, &ctx->errors, "No convergence in gshank()", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/somnec.c:663: add_error(ctx, &ctx->errors, "Hankel function invalid for z=0", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:124: add_error(ctx, &ctx->errors, "Failed to initialize calculation defaults (no valid geometry)", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:304: add_error(ctx, &ctx->errors, nx_geom_errors.errors[i].message,
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:309: add_error(ctx, &ctx->errors,
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:613: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:648: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:673: add_error(ctx, &ctx->errors,
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:694: add_error(ctx, &ctx->errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:718: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:741: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:795: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:802: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:932: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:950: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:997: add_error(ctx, &ctx->errors, "Geometry not initialized before frequency loop", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c:1002: add_error(ctx, &ctx->errors, "Geometry connection data not allocated", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c:933: //   add_error(NULL, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c:1183: add_error(ctx, errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c:1293: add_error(ctx, errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c:1434: * @param ctx      The context (for add_error)
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c:1497: add_error(ctx, errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:92: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:101: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:116: add_error(ctx, &ctx->errors, msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:236: add_error(ctx, &ctx->errors, msg, WARNING);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:336: add_error(ctx, errors, msg, 0);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:537: add_error(ctx, errors, msg, PROBLEM);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:1145: add_error(ctx, &ctx->errors, "Memory allocation failed for 'invisible' key/value", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c:1152: add_error(ctx, &ctx->errors, "Memory allocation failed for 'invisible' strings", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/main.c:87: * TODO: Make this static once all calculation files use add_error() instead
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:309: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:402: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:913: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:965: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:1173: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c:1420: add_error(ctx, &ctx->errors, err_msg, FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.c:19: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity)
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.c:250: add_error(ctx, (errors_list_t*)&ctx->errors, "Memory allocation failed", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.c:262: add_error(ctx, (errors_list_t*)&ctx->errors, "Memory reallocation failed", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/output.c:1195: add_error(ctx, &ctx->errors,
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/output.c:1421: add_error(ctx, &ctx->errors, "ERROR: IPSYM=0 IN CONECT()", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/output.c:1520: add_error(ctx, &ctx->errors, "SEGMENT DATA ERROR", FATAL);
/Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/matrix.c:1284: add_error(ctx, &ctx->errors, err_msg, FATAL);
TOTAL 204

---
End of inventory.

