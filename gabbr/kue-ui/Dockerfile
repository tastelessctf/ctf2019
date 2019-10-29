FROM node:10
RUN npm install -g kue
ENV REDIS_HOST redis
ENV REDIS_PORT 6379
ENV REDIS "redis://${REDIS_HOST}:${REDIS_PORT}"
ENV PORT 3000
ENTRYPOINT ""/usr/local/bin/kue-dashboard "-p" ${PORT} "-r" ${REDIS}