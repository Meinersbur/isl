if (n % 2 == 0)
  for (int c0 = (n / 2) + 2 * floord(-n - 1, 4) + 2, c0_end = 100; c0 <= c0_end; c0 += 2)
    S(c0);
