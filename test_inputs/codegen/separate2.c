if ((length - 1) % 16 <= 14)
  for (int c0 = 0; c0 <= 1; c0 += 1)
    for (int c5 = 0; c5 <= 31; c5 += 1)
      for (int c6 = 0; c6 <= 30; c6 += 1) {
        if ((2 * c5 - c6 + 31) % 32 == 31 && 2 * ((length - 1) % 16) + 2 * c5 == 2 * ((length - 1) % 32) + c6 && c6 + 62 >= 2 * ((length - 1) % 16) + 2 * c5 && 2 * length + c6 >= 2 * ((length - 1) % 16) + 4 && 2 * ((length - 1) % 16) >= c6 && 2 * ((length - 1) % 16) + 2 * c5 >= c6)
          S_3(c0, 0, (-(2 * ((length - 1) % 16)) + 2 * length + c6 - 2) / 2);
        if (length <= 16 && length >= c5 + 1 && c6 >= 1 && length >= c6)
          S_0(c0, c5, c6 - 1);
      }
