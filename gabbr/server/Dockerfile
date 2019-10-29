FROM node:10

WORKDIR /app
ADD package.json ./
RUN npm install
ADD index.ts tsconfig.json ./
ADD views/home.handlebars views/home.handlebars

RUN npm run build
ENV REDIS_HOST redis
ENV REDIS_PORT 6379
ENV REDIS "redis://${REDIS_HOST}:${REDIS_PORT}"
CMD [ "npm", "start" ]
ENV PORT 1337
EXPOSE 1337