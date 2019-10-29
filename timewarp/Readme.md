!!! THIS CHALLENGE IS OVER !!! you can not solve it anymore.

however, if you sit on a flag, feel free to submit it :)

!!! challenge will shutdown at approx 01:30 UTC !!!

If you haven't solved it until then, we don't think you'll be able to do it.

http://hitme.tasteless.eu:10101

Author: ccm

---

this challenge created tokens with a validity of 5 seconds, and Ansi C date formatting.
Ansi C does not include any timezone/offset/location information so, during the DST time shift
tokens issues before 03:00am were valid for another hour as the clock jumped back to 02:00.

the solution was to get a token between 02:00 and 03:00, wait until 03:00 for the clock
to jump back (for Europe/Berlin timezone) and then simply reload the page again.
