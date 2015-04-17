for (int c0 = 0, c0_end = ni; c0 < c0_end; c0 += 1)
  for (int c1 = 0, c1_end = nj; c1 < c1_end; c1 += 1) {
    S_2(c0, c1);
    for (int c2 = 0, c2_end = nk; c2 < c2_end; c2 += 1)
      S_4(c0, c1, c2);
  }
