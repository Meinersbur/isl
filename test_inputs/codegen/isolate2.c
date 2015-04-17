for (int c0 = 0, c0_end = 99; c0 <= c0_end; c0 += 1) {
  if (c0 >= 4 && c0 <= 6) {
    for (int c1 = 0, c1_end = 99; c1 <= c1_end; c1 += 1)
      A(c0, c1);
  } else if (c0 >= 7) {
    for (int c1 = 0, c1_end = 99; c1 <= c1_end; c1 += 1)
      A(c0, c1);
  } else
    for (int c1 = 0, c1_end = 99; c1 <= c1_end; c1 += 1)
      A(c0, c1);
}
