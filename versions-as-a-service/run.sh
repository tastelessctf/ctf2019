#!/bin/sh
docker run --rm -i --read-only --tmpfs /revs:rw,noexec,nosuid,size=11m -v $(pwd)/flag:/flag:ro --user 9000:9000 vaas
