for (int c0 = 0, c0_end = 4; c0 <= c0_end; c0 += 1) {
  if (c0 == 0) {
    S_0(0, 4);
  } else {
    S_0(2 * c0 - 1, 1);
    if (c0 == 4) {
      for (int c6 = 3, c6_end = 5; c6 <= c6_end; c6 += 1)
        S_0(7, c6);
    } else
      for (int c4 = 2 * c0 - 1, c4_end = 2 * c0; c4 <= c4_end; c4 += 1)
        for (int c6 = -2 * c0 + c4 + 4, c6_end = 2 * c0 - c4 + 4; c6 <= c6_end; c6 += 1)
          S_0(c4, c6);
  }
  for (int c4 = max(0, 2 * c0 - 1), c4_end = min(7, 2 * c0); c4 <= c4_end; c4 += 1)
    for (int c6 = -2 * c0 + c4 + 8, c6_end = 8; c6 <= c6_end; c6 += 1)
      S_0(c4, c6);
  if (c0 >= 1 && c0 <= 3) {
    for (int c2 = 0, c2_end = 1; c2 <= c2_end; c2 += 1)
      for (int c4 = 2 * c0 - 1, c4_end = 2 * c0; c4 <= c4_end; c4 += 1)
        for (int c6 = 2 * c0 + 4 * c2 - c4 + 1, c6_end = -2 * c0 + 4 * c2 + c4 + 3; c6 <= c6_end; c6 += 1)
          S_0(c4, c6);
  } else if (c0 == 4) {
    for (int c2 = 0, c2_end = 1; c2 <= c2_end; c2 += 1)
      S_0(7, 4 * c2 + 2);
  } else
    for (int c2 = 0, c2_end = 1; c2 <= c2_end; c2 += 1)
      for (int c6 = 4 * c2 + 1, c6_end = 4 * c2 + 3; c6 <= c6_end; c6 += 1)
        S_0(0, c6);
}
