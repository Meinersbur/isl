for (int c0 = 0, c0_end = 128; c0 <= c0_end; c0 += 1) {
  if (c0 <= 127) {
    if (c0 == 0) {
      for (int c3 = 0, c3_end = 1; c3 <= c3_end; c3 += 1)
        for (int c5 = c3 + 58, c5_end = -c3 + 61; c5 <= c5_end; c5 += 1)
          S_0(c3, c5);
    } else
      for (int c2 = 1, c2_end = 2; c2 <= c2_end; c2 += 1)
        for (int c3 = max(4 * c0 - 2, 4 * c0 + 6 * c2 - 12), c3_end = min(4 * c0 + 1, 4 * c0 + 6 * c2 - 7); c3 <= c3_end; c3 += 1)
          for (int c5 = max(4 * c0 - c3 + 57, -4 * c0 + c3 + 58), c5_end = min(4 * c0 - c3 + 61, -4 * c0 + c3 + 62); c5 <= c5_end; c5 += 1)
            S_0(c3, c5);
    for (int c2 = 1, c2_end = 2; c2 <= c2_end; c2 += 1)
      for (int c3 = max(4 * c0, 4 * c0 + 6 * c2 - 10), c3_end = min(4 * c0 + 3, 4 * c0 + 6 * c2 - 5); c3 <= c3_end; c3 += 1)
        for (int c5 = max(-4 * c0 + c3 + 59, 4 * c0 - c3 + 62), c5_end = min(-4 * c0 + c3 + 63, 4 * c0 - c3 + 66); c5 <= c5_end; c5 += 1)
          S_0(c3, c5);
  } else
    for (int c3 = 510, c3_end = 511; c3 <= c3_end; c3 += 1)
      for (int c5 = -c3 + 569, c5_end = c3 - 449; c5 < c5_end; c5 += 1)
        S_0(c3, c5);
}
