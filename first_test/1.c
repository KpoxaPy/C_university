int count (char * str)
{
  if (*str == ' ')
    return ++count(++str);
  else if (*str == '\0')
    return 0;
  else
    return count(++str);
}
