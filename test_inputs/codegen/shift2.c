for (int c0 = 0; c0 <= 1; c0 += 1) {
  for (int c1 = 0; c1 < length - 1; c1 += 32) {
    for (int c2 = c1; c2 < length; c2 += 32) {
      if (c1 == 0)
        for (int c3 = 0; c3 <= min(length, 2 * c2 - 32); c3 += 32)
          for (int c5 = 0; c5 <= min(31, length - c2 - 1); c5 += 1)
            for (int c6 = max(-c3 + 1, 0); c6 <= min(31, length - c3); c6 += 1)
              S_0(c0, c2 + c5, c3 + c6 - 1);
      for (int c3 = max(2 * c1, 2 * c2); c3 <= min(2 * c2 + 62, 2 * length - 2); c3 += 32)
        for (int c4 = 0; c4 <= min(min(length - c1 - 2, -c1 + c3 / 2 + 14), 31); c4 += 1) {
          if (c4 == 0 && c3 == 0 && c2 == 0 && c1 == 0) {
            for (int c6 = 1; c6 <= min(31, length); c6 += 1)
              S_0(c0, 0, c6 - 1);
          } else if (c4 == 0 && c3 == 2 * c2 + 32 && c1 == 0)
            for (int c5 = 0; c5 <= 15; c5 += 1)
              for (int c6 = 0; c6 <= min(31, length - 2 * c2 - 32); c6 += 1)
                S_0(c0, c2 + c5, 2 * c2 + c6 + 31);
          for (int c5 = max((-2 * c2 + c3) / 2, c1 - c2 + c4 + 1); c5 <= min(-c2 + c3 / 2 + 15, length - c2 - 1); c5 += 1) {
            if (c4 == 0 && c1 == 0)
              for (int c6 = max(-c3 + 1, 0); c6 <= min(2 * c2 - c3 + 2 * c5 - 1, length - c3); c6 += 1)
                S_0(c0, c2 + c5, c3 + c6 - 1);
            S_3(c0, c1 + c4, c2 + c5);
            if (c4 == 0 && c1 == 0 && length >= 2 * c2 + 2 * c5)
              S_0(c0, c2 + c5, 2 * c2 + 2 * c5 - 1);
            if (c4 == 0 && c1 == 0)
              for (int c6 = 2 * c2 - c3 + 2 * c5 + 1; c6 <= min(length - c3, 31); c6 += 1)
                S_0(c0, c2 + c5, c3 + c6 - 1);
          }
          if (c4 == 0 && c1 == 0 && c3 + 30 >= 2 * length)
            S_4(c0);
          if (c4 == 0 && c3 == 2 * c2 && c1 == 0)
            for (int c5 = 16; c5 <= min(length - c2 - 1, 31); c5 += 1)
              for (int c6 = max(0, -2 * c2 + 1); c6 <= min(31, length - 2 * c2); c6 += 1)
                S_0(c0, c2 + c5, 2 * c2 + c6 - 1);
        }
      if (32 * floord(length - 16, 32) + 16 == length && c2 + 16 == length && c1 == 0)
        S_4(c0);
      if (c1 == 0)
        for (int c3 = 2 * c2 + 64; c3 <= length; c3 += 32)
          for (int c5 = 0; c5 <= 31; c5 += 1)
            for (int c6 = 0; c6 <= min(31, length - c3); c6 += 1)
              S_0(c0, c2 + c5, c3 + c6 - 1);
    }
    if (length % 32 == 0 && c1 == 0)
      S_4(c0);
  }
  if (length <= 1)
    for (int c5 = 0; c5 <= length; c5 += 1)
      if (c5 == length) {
        S_4(c0);
      } else
        S_0(c0, 0, 0);
}
