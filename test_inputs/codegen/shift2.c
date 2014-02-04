for (int c0 = 0; c0 <= 1; c0 += 1) {
  for (int c2 = 0; c2 < length; c2 += 32) {
    for (int c3 = 0; c3 <= length; c3 += 32) {
      for (int c5 = 0; c5 <= min(31, length - c2 - 1); c5 += 1) {
        for (int c6 = max(0, -c3 + 1); c6 <= min(min(31, length - c3), 2 * c2 - c3 + 2 * c5 - 1); c6 += 1)
          S_0(c0, c2 + c5, c3 + c6 - 1);
        if (c3 + 30 >= 2 * c2 + 2 * c5 && 2 * c2 + 2 * c5 >= c3 && c2 + c5 >= 1) {
          S_3(c0, 0, c2 + c5);
          if (length >= 2 * c2 + 2 * c5)
            S_0(c0, c2 + c5, 2 * c2 + 2 * c5 - 1);
        }
        for (int c6 = max(0, 2 * c2 - c3 + 2 * c5 + 1); c6 <= min(31, length - c3); c6 += 1)
          S_0(c0, c2 + c5, c3 + c6 - 1);
      }
      if (c3 == 0 && c2 == 0 && length <= 15)
        S_4(c0);
      if (2 * c2 + 32 >= c3 && c3 >= 2 * c2)
        for (int c4 = 1; c4 <= min(min(31, length - 2), (c3 / 2) + 14); c4 += 1)
          for (int c5 = max((c3 / 2) - c2, -c2 + c4 + 1); c5 <= min(length - c2 - 1, (c3 / 2) - c2 + 15); c5 += 1)
            S_3(c0, c4, c2 + c5);
    }
    for (int c3 = max(2 * c2, -(length % 32) + length + 32); c3 <= min(2 * length - 2, 2 * c2 + 62); c3 += 32)
      for (int c4 = 0; c4 <= min(31, length - 2); c4 += 1) {
        for (int c5 = max((c3 / 2) - c2, -c2 + c4 + 1); c5 <= min(length - c2 - 1, (c3 / 2) - c2 + 15); c5 += 1)
          S_3(c0, c4, c2 + c5);
        if (c4 == 0 && c3 + 30 >= 2 * length)
          S_4(c0);
      }
    if ((length + 16) % 32 == 0 && c2 + 16 == length)
      S_4(c0);
  }
  if (length % 32 == 0 && length >= 32)
    S_4(c0);
  if (length == 0)
    S_4(c0);
  for (int c1 = 32; c1 < length - 1; c1 += 32)
    for (int c2 = c1; c2 < length; c2 += 32)
      for (int c3 = max(c1, c2); c3 <= min(length - 1, c2 + 31); c3 += 16)
        for (int c4 = 0; c4 <= min(min(31, length - c1 - 2), -c1 + c3 + 14); c4 += 1)
          for (int c5 = max(-c2 + c3, c1 - c2 + c4 + 1); c5 <= min(length - c2 - 1, -c2 + c3 + 15); c5 += 1)
            S_3(c0, c1 + c4, c2 + c5);
}
