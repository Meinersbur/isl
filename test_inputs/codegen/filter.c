if (n >= m + 1) {
  for (int c0 = 0, c0_end = n; c0 < c0_end; c0 += 1)
    for (int c2 = 0, c2_end = n; c2 < c2_end; c2 += 1)
      A(c0, c2);
} else
  for (int c0 = 0, c0_end = n; c0 < c0_end; c0 += 1)
    for (int c2 = 0, c2_end = n; c2 < c2_end; c2 += 1)
      A(c0, c2);
