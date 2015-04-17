if ((length - 1) % 16 <= 14)
  for (int c0 = 0, c0_end = 1; c0 <= c0_end; c0 += 1)
    for (int c5 = 0, c5_end = 31; c5 <= c5_end; c5 += 1)
      for (int c6 = max(0, 2 * ((length - 1) % 16) + 2 * c5 - 60), c6_end = 30; c6 <= c6_end; c6 += 1) {
        if (length + c5 >= ((length - 1) % 32) + 2 && (length - 1) % 32 >= c5 && 2 * ((length - 1) % 32) + c6 >= 2 * c5 && 2 * c5 + 30 >= 2 * ((length - 1) % 32) + c6 && 2 * ((length - 1) % 32) + c6 == 2 * ((length - 1) % 16) + 2 * c5 && (2 * c5 - c6) % 32 == 0)
          S_3(c0, 0, (c6 / 2) - ((length - 1) % 16) + length - 1);
        if (length <= 16 && length >= c5 + 1 && c6 >= 1 && length >= c6)
          S_0(c0, c5, c6 - 1);
      }
