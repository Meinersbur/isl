for (int c0 = 0, c0_end = 100; c0 <= c0_end; c0 += 2) {
  for (int c3 = 0, c3_end = 100; c3 <= c3_end; c3 += 1)
    A(c0, c3);
  for (int c2 = 0, c2_end = 100; c2 <= c2_end; c2 += 1)
    B(c0, c2);
}
