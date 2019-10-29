FROM ubuntu

RUN apt-get update
RUN apt-get -y install firefox
RUN apt-get -y install nodejs
RUN apt-get -y install npm

WORKDIR /app
ADD package.json ./
RUN npm install
ADD worker.ts tsconfig.json ./

RUN npm run build
ENV REDIS_HOST redis
ENV REDIS_PORT 6379
ENV REDIS "redis://${REDIS_HOST}:${REDIS_PORT}"
CMD [ "npm", "start" ]