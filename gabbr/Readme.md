https://gabbr.hitme.tasteless.eu/

Author: plonk

---

changes:
- this repository contains all relevant files to setup the gabbr challenge locally.
- recaptcha verification is removed/disabled.
- caddy for https is removed.
- worker is running in host network mode to bypass wss and allow access to localhost (e.g. for sample exploiting)

local setup:
```
docker-compose up --build
```

UI: http://localhost:8080
Kue-UI: http://localhost:8081
Pushgateway: http://localhost:9091
